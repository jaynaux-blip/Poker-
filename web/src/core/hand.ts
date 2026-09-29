import { Card, fullDeck } from './cards';
import { evaluateHand } from './evaluator';
import { Rng } from './rng';

/**
 * One hand of No-Limit Texas Hold'em.
 *
 * Implements: moving button with heads-up blind rules, big-blind ante (dead
 * money, posted after the blind when the big blind is short), minimum raise
 * sizing, incomplete all-in raises that do not reopen betting for players who
 * already acted, uncalled-bet returns, side pots, and odd chips going to the
 * first winner left of the button.
 *
 * The hand is a pure state machine: callers read `toAct` and
 * `legalActions()`, then call `act()`. Everything that happens is appended to
 * `events` so the UI can animate it and hand histories can be rebuilt.
 */

export type Street = 'preflop' | 'flop' | 'turn' | 'river' | 'showdown';

export interface SeatInput {
  seat: number; // physical seat number at the table, 0..8
  id: string;
  stack: number;
}

export interface HandConfig {
  players: SeatInput[];
  buttonSeat: number;
  smallBlind: number;
  bigBlind: number;
  ante: number; // big-blind ante, 0 for none
  rng: Rng;
  /** Optional fixed deck order for tests (top of deck first). */
  deck?: Card[];
}

export interface Seat {
  seat: number;
  id: string;
  startStack: number;
  stack: number;
  hole: Card[];
  streetBet: number;
  committed: number; // total put in this hand, including ante
  anteCommitted: number;
  folded: boolean;
  allIn: boolean;
  hasActed: boolean;
  raiseLocked: boolean;
}

export type PlayerActionType = 'fold' | 'check' | 'call' | 'bet' | 'raise';

export interface ActionRecord {
  street: Street;
  seat: number;
  type: PlayerActionType;
  added: number; // chips put in by this action
  to: number; // player's total street bet after the action
  allIn: boolean;
  potBefore: number;
  facing: number; // amount the player had to call before acting
}

export interface PotResult {
  amount: number;
  eligible: number[]; // seat numbers
  winners: number[];
  shares: number[]; // chips won per winner, same order as winners
}

export type HandEvent =
  | { t: 'ante'; seat: number; amount: number }
  | { t: 'blind'; seat: number; amount: number; kind: 'sb' | 'bb' }
  | { t: 'deal'; order: number[] }
  | ({ t: 'action' } & ActionRecord)
  | { t: 'street'; street: Street; cards: Card[]; board: Card[] }
  | { t: 'return'; seat: number; amount: number }
  | { t: 'reveal'; seats: number[] } // all-in: hands turned face up before the runout
  | { t: 'showdown'; hands: { seat: number; hole: Card[]; score: number }[] }
  | { t: 'award'; pot: PotResult; index: number; potCount: number }
  | { t: 'end' };

export interface LegalActions {
  canFold: boolean;
  canCheck: boolean;
  callAmount: number; // chips needed to call (capped by stack)
  canRaise: boolean;
  minRaiseTo: number; // total street bet for the smallest legal raise
  maxRaiseTo: number; // all-in total street bet
  isBet: boolean; // true when there is no bet yet this street
}

export type PlayerAction =
  | { type: 'fold' }
  | { type: 'check' }
  | { type: 'call' }
  | { type: 'raise'; to: number };

export class Hand {
  readonly seats: Seat[];
  readonly events: HandEvent[] = [];
  readonly history: ActionRecord[] = [];
  readonly board: Card[] = [];
  readonly smallBlind: number;
  readonly bigBlind: number;
  readonly ante: number;
  readonly buttonSeat: number;

  street: Street = 'preflop';
  toAct: number | null = null; // index into seats
  currentBet = 0;
  minRaiseInc = 0;
  lastAggressor: number | null = null; // index into seats
  complete = false;
  potResults: PotResult[] = [];

  private deck: Card[];
  private deckPos = 0;
  private buttonIdx: number;
  private sbIdx = -1;
  private bbIdx = -1;

  constructor(cfg: HandConfig) {
    if (cfg.players.length < 2) throw new Error('A hand needs at least two players');
    const sorted = [...cfg.players].sort((a, b) => a.seat - b.seat);
    this.seats = sorted.map((p) => ({
      seat: p.seat,
      id: p.id,
      startStack: p.stack,
      stack: p.stack,
      hole: [],
      streetBet: 0,
      committed: 0,
      anteCommitted: 0,
      folded: false,
      allIn: false,
      hasActed: false,
      raiseLocked: false,
    }));
    this.smallBlind = cfg.smallBlind;
    this.bigBlind = cfg.bigBlind;
    this.ante = cfg.ante;
    this.buttonSeat = cfg.buttonSeat;
    this.buttonIdx = this.seats.findIndex((s) => s.seat === cfg.buttonSeat);
    if (this.buttonIdx < 0) throw new Error('Button must be on an occupied seat');
    this.deck = cfg.deck ? [...cfg.deck] : cfg.rng.shuffle(fullDeck());
    this.start();
  }

  // ---------------------------------------------------------------- queries

  get pot(): number {
    let p = 0;
    for (const s of this.seats) p += s.committed;
    return p;
  }

  /** Chips in the middle from previous streets (excludes current street bets). */
  get potBeforeStreet(): number {
    let p = 0;
    for (const s of this.seats) p += s.committed - s.streetBet;
    return p;
  }

  seatByNumber(seat: number): Seat | undefined {
    return this.seats.find((s) => s.seat === seat);
  }

  get buttonIndex(): number {
    return this.buttonIdx;
  }

  get smallBlindIndex(): number {
    return this.sbIdx;
  }

  get bigBlindIndex(): number {
    return this.bbIdx;
  }

  get activeCount(): number {
    return this.seats.filter((s) => !s.folded).length;
  }

  /** Players who can still make betting decisions. */
  private get ableCount(): number {
    return this.seats.filter((s) => !s.folded && !s.allIn).length;
  }

  legalActions(): LegalActions {
    if (this.toAct === null) throw new Error('No player to act');
    const s = this.seats[this.toAct];
    const toCall = Math.max(0, this.currentBet - s.streetBet);
    const callAmount = Math.min(toCall, s.stack);
    const maxRaiseTo = s.streetBet + s.stack;
    const fullMin = this.currentBet + this.minRaiseInc;
    // A raise is possible if the player has chips beyond a call, isn't locked
    // by an incomplete raise, and someone else could still respond.
    const othersAble = this.seats.some((o, i) => i !== this.toAct && !o.folded && !o.allIn);
    const canRaise = !s.raiseLocked && maxRaiseTo > this.currentBet && othersAble;
    return {
      canFold: true,
      canCheck: toCall === 0,
      callAmount,
      canRaise,
      minRaiseTo: Math.min(fullMin, maxRaiseTo),
      maxRaiseTo,
      isBet: this.currentBet === 0,
    };
  }

  // ---------------------------------------------------------------- actions

  act(action: PlayerAction): void {
    if (this.complete || this.toAct === null) throw new Error('Hand is not waiting for an action');
    const idx = this.toAct;
    const s = this.seats[idx];
    const legal = this.legalActions();
    const potBefore = this.pot;
    const facing = legal.callAmount;
    let type: PlayerActionType;
    let added = 0;

    switch (action.type) {
      case 'fold':
        s.folded = true;
        type = 'fold';
        break;
      case 'check':
        if (!legal.canCheck) throw new Error('Cannot check facing a bet');
        type = 'check';
        break;
      case 'call':
        if (legal.callAmount === 0) {
          type = 'check';
          break;
        }
        added = this.commit(s, legal.callAmount);
        type = 'call';
        break;
      case 'raise': {
        if (!legal.canRaise) throw new Error('Raising is not allowed here');
        const to = Math.min(Math.floor(action.to), legal.maxRaiseTo);
        if (to <= this.currentBet) throw new Error('Raise must exceed the current bet');
        const isAllIn = to === legal.maxRaiseTo;
        if (to < legal.minRaiseTo && !isAllIn) throw new Error(`Minimum raise is to ${legal.minRaiseTo}`);
        type = this.currentBet === 0 ? 'bet' : 'raise';
        const raiseSize = to - this.currentBet;
        added = this.commit(s, to - s.streetBet);
        if (raiseSize >= this.minRaiseInc) {
          // Full raise: reopens the betting for everyone.
          this.minRaiseInc = raiseSize;
          for (let i = 0; i < this.seats.length; i++) {
            if (i === idx) continue;
            const o = this.seats[i];
            if (!o.folded && !o.allIn) {
              o.hasActed = false;
              o.raiseLocked = false;
            }
          }
        } else {
          // Incomplete all-in raise: players who already acted may only call or fold.
          for (let i = 0; i < this.seats.length; i++) {
            if (i === idx) continue;
            const o = this.seats[i];
            if (!o.folded && !o.allIn && o.hasActed) {
              o.hasActed = false;
              o.raiseLocked = true;
            }
          }
        }
        this.currentBet = to;
        this.lastAggressor = idx;
        break;
      }
    }

    s.hasActed = true;
    s.raiseLocked = false;
    const rec: ActionRecord = {
      street: this.street,
      seat: s.seat,
      type,
      added,
      to: s.streetBet,
      allIn: s.allIn,
      potBefore,
      facing,
    };
    this.history.push(rec);
    this.events.push({ t: 'action', ...rec });
    this.advance();
  }

  // ---------------------------------------------------------------- internals

  private commit(s: Seat, amount: number): number {
    const a = Math.min(amount, s.stack);
    s.stack -= a;
    s.streetBet += a;
    s.committed += a;
    if (s.stack === 0) s.allIn = true;
    return a;
  }

  private nextIdx(from: number): number {
    return (from + 1) % this.seats.length;
  }

  private start(): void {
    const n = this.seats.length;
    if (n === 2) {
      // Heads-up: the button posts the small blind and acts first preflop.
      this.sbIdx = this.buttonIdx;
      this.bbIdx = this.nextIdx(this.buttonIdx);
    } else {
      this.sbIdx = this.nextIdx(this.buttonIdx);
      this.bbIdx = this.nextIdx(this.sbIdx);
    }

    const sb = this.seats[this.sbIdx];
    const bb = this.seats[this.bbIdx];
    const sbAmt = this.commit(sb, this.smallBlind);
    this.events.push({ t: 'blind', seat: sb.seat, amount: sbAmt, kind: 'sb' });
    const bbAmt = this.commit(bb, this.bigBlind);
    this.events.push({ t: 'blind', seat: bb.seat, amount: bbAmt, kind: 'bb' });
    if (this.ante > 0 && bb.stack > 0) {
      // Big-blind ante is dead money: it never counts toward the BB's bet.
      const a = Math.min(this.ante, bb.stack);
      bb.stack -= a;
      bb.committed += a;
      bb.anteCommitted += a;
      if (bb.stack === 0) bb.allIn = true;
      this.events.push({ t: 'ante', seat: bb.seat, amount: a });
    }
    this.currentBet = this.bigBlind;
    this.minRaiseInc = this.bigBlind;

    // Deal two cards each, one at a time, starting left of the button.
    const order: number[] = [];
    for (let i = 1; i <= n; i++) order.push((this.buttonIdx + i) % n);
    for (let round = 0; round < 2; round++) for (const i of order) this.seats[i].hole.push(this.draw());
    this.events.push({ t: 'deal', order: order.map((i) => this.seats[i].seat) });

    // First to act preflop: left of the big blind.
    this.toAct = this.findNextToAct(this.bbIdx);
    if (this.toAct === null) this.finishStreet();
  }

  private draw(): Card {
    return this.deck[this.deckPos++];
  }

  /** Next seat (after `from`) that still needs to act this street. */
  private findNextToAct(from: number): number | null {
    const n = this.seats.length;
    for (let k = 1; k <= n; k++) {
      const i = (from + k) % n;
      const s = this.seats[i];
      if (s.folded || s.allIn) continue;
      if (!s.hasActed || s.streetBet < this.currentBet) return i;
    }
    return null;
  }

  private advance(): void {
    if (this.activeCount === 1) {
      this.returnUncalled();
      this.awardUncontested();
      return;
    }
    const next = this.findNextToAct(this.toAct!);
    if (next !== null) {
      this.toAct = next;
      return;
    }
    this.finishStreet();
  }

  private finishStreet(): void {
    this.toAct = null;
    this.returnUncalled();
    for (const s of this.seats) {
      s.streetBet = 0;
      s.hasActed = false;
      s.raiseLocked = false;
    }
    this.currentBet = 0;
    this.minRaiseInc = this.bigBlind;

    if (this.street === 'river') {
      this.showdown();
      return;
    }

    // If at most one player can still bet, the rest of the board just runs out.
    const runout = this.ableCount <= 1;
    if (runout && !this.events.some((e) => e.t === 'reveal')) {
      this.events.push({ t: 'reveal', seats: this.seats.filter((s) => !s.folded).map((s) => s.seat) });
    }

    this.dealNextStreet();
    if (runout) {
      this.finishStreet();
      return;
    }
    // Postflop, the first active player left of the button acts first.
    this.toAct = this.findNextToAct(this.buttonIdx);
    if (this.toAct === null) this.finishStreet();
  }

  private dealNextStreet(): void {
    this.draw(); // burn
    let cards: Card[];
    if (this.street === 'preflop') {
      cards = [this.draw(), this.draw(), this.draw()];
      this.street = 'flop';
    } else if (this.street === 'flop') {
      cards = [this.draw()];
      this.street = 'turn';
    } else {
      cards = [this.draw()];
      this.street = 'river';
    }
    this.board.push(...cards);
    this.events.push({ t: 'street', street: this.street, cards, board: [...this.board] });
  }

  private returnUncalled(): void {
    // The highest street bet beyond what anyone else matched goes back.
    let top = -1;
    let topBet = -1;
    let second = 0;
    for (let i = 0; i < this.seats.length; i++) {
      const b = this.seats[i].streetBet;
      if (b > topBet) {
        second = Math.max(second, topBet);
        topBet = b;
        top = i;
      } else if (b > second) second = b;
    }
    if (top >= 0 && topBet > second) {
      const s = this.seats[top];
      const back = topBet - second;
      s.stack += back;
      s.streetBet -= back;
      s.committed -= back;
      if (s.stack > 0) s.allIn = false;
      this.events.push({ t: 'return', seat: s.seat, amount: back });
    }
  }

  private awardUncontested(): void {
    this.toAct = null;
    const winner = this.seats.find((s) => !s.folded)!;
    const amount = this.pot;
    const pot: PotResult = { amount, eligible: [winner.seat], winners: [winner.seat], shares: [amount] };
    winner.stack += amount;
    this.potResults = [pot];
    this.events.push({ t: 'award', pot, index: 0, potCount: 1 });
    this.end();
  }

  /** Split committed chips into a main pot and side pots. */
  buildPots(): { amount: number; eligible: number[] }[] {
    const remaining = this.seats.map((s) => s.committed - s.anteCommitted);
    const antes = this.seats.reduce((a, s) => a + s.anteCommitted, 0);
    const pots: { amount: number; eligible: number[] }[] = [];
    for (;;) {
      let level = Infinity;
      for (let i = 0; i < this.seats.length; i++) {
        if (!this.seats[i].folded && remaining[i] > 0) level = Math.min(level, remaining[i]);
      }
      if (level === Infinity) break;
      let amount = 0;
      const eligible: number[] = [];
      for (let i = 0; i < this.seats.length; i++) {
        const take = Math.min(remaining[i], level);
        if (take > 0) {
          amount += take;
          remaining[i] -= take;
          if (!this.seats[i].folded) eligible.push(this.seats[i].seat);
        }
      }
      pots.push({ amount, eligible });
    }
    // Leftover chips from folded players (rare) join the last pot.
    const leftover = remaining.reduce((a, b) => a + b, 0);
    if (pots.length === 0) {
      pots.push({ amount: antes + leftover, eligible: this.seats.filter((s) => !s.folded).map((s) => s.seat) });
    } else {
      pots[0].amount += antes;
      pots[pots.length - 1].amount += leftover;
    }
    return pots;
  }

  private showdown(): void {
    this.street = 'showdown';
    this.toAct = null;
    const live = this.seats.filter((s) => !s.folded);
    const scores = new Map<number, number>();
    for (const s of live) scores.set(s.seat, evaluateHand(s.hole, this.board));
    this.events.push({
      t: 'showdown',
      hands: live.map((s) => ({ seat: s.seat, hole: [...s.hole], score: scores.get(s.seat)! })),
    });

    const pots = this.buildPots();
    const n = this.seats.length;
    // Seat order starting left of the button, for odd-chip distribution.
    const leftOfButton = (seat: number) => {
      const i = this.seats.findIndex((s) => s.seat === seat);
      return (i - this.buttonIdx - 1 + n) % n;
    };
    this.potResults = [];
    pots.forEach((p, index) => {
      let best = -1;
      for (const seat of p.eligible) best = Math.max(best, scores.get(seat)!);
      const winners = p.eligible.filter((seat) => scores.get(seat) === best).sort((a, b) => leftOfButton(a) - leftOfButton(b));
      const base = Math.floor(p.amount / winners.length);
      let odd = p.amount - base * winners.length;
      const shares = winners.map(() => base + (odd-- > 0 ? 1 : 0));
      winners.forEach((seat, k) => {
        this.seatByNumber(seat)!.stack += shares[k];
      });
      const result: PotResult = { amount: p.amount, eligible: p.eligible, winners, shares };
      this.potResults.push(result);
      this.events.push({ t: 'award', pot: result, index, potCount: pots.length });
    });
    this.end();
  }

  private end(): void {
    this.complete = true;
    this.toAct = null;
    this.events.push({ t: 'end' });
  }
}
