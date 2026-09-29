import { analyzeDecision, DecisionAnalysis, DecisionGrade, gradeDecision } from '../core/ai/grading';
import { pushRange } from '../core/ai/bot';
import { Archetype, sprintProfile } from '../core/ai/profiles';
import { makeView, playersBehind, positionOf, preflopSummary } from '../core/ai/view';
import { Card, cardToString } from '../core/cards';
import { equityKnownHands, handPercentile } from '../core/equity';
import { describe } from '../core/evaluator';
import { Hand, HandEvent, PlayerAction } from '../core/hand';
import { Rng } from '../core/rng';
import { HERO_ID, TEvent, Tournament } from '../core/tournament';
import { chips, money, ordinal } from '../client/canvasui';
import { ChatLine, RIVAL, RIVAL_LINES, brag, bustLine, idle, nice, salt } from './chat';
import { LOBBY, LobbyEvent } from './events';

/**
 * Night One game session: bankroll, lobby, and the moment-to-moment flow of
 * a tournament. Hands from the engine are replayed event by event with
 * realistic pacing so the client can animate them; the hero's decisions are
 * graded; tournament milestones trigger story beats.
 */

export type Pace = 'full' | 'smart' | 'sprint';
export type Screen = 'boot' | 'lobby' | 'table' | 'results';

export interface SeatVis {
  seat: number;
  id: string;
  name: string;
  isHero: boolean;
  isRival: boolean;
  stack: number;
  bet: number;
  hole: Card[]; // known to the viewer (hero, or revealed)
  hasCards: boolean;
  folded: boolean;
  allIn: boolean;
  lastAction: string;
  lastActionAt: number;
  acting: boolean;
  actStart: number;
  actEnd: number;
  equity: number | null;
  winner: boolean;
  handLabel: string;
  dealtAt: number;
  sittingOut: boolean;
}

export interface Flight {
  kind: 'chips' | 'card';
  from: { seat?: number; pot?: boolean; bet?: number; deck?: boolean };
  to: { seat?: number; pot?: boolean; bet?: number; muck?: boolean };
  amount: number;
  start: number;
  dur: number;
}

export interface HeroPrompt {
  analysis: DecisionAnalysis;
  openedAt: number;
  deadline: number;
  timeBankUntil: number;
  raiseTo: number;
  minRaise: number;
  maxRaise: number;
  toCall: number;
  canCheck: boolean;
  canRaise: boolean;
  isBet: boolean;
  pot: number;
  bigBlind: number;
  street: string;
}

export interface GradeBadge {
  grade: DecisionGrade;
  at: number;
}

export interface Results {
  eventName: string;
  place: number;
  entrants: number;
  prizeCents: number;
  buyInCents: number;
  hands: number;
  grades: DecisionGrade[];
  accuracy: number;
  biggestPot: number;
  won: boolean;
  bustHand: string;
}

export interface Hooks {
  sound(name: SoundName, opts?: { volume?: number; rate?: number }): void;
  text(from: string, body: string): void;
  heartbeat(on: boolean): void;
  addCan(): void;
  celebrate(): void;
}

export type SoundName =
  | 'chip' | 'chips' | 'card' | 'flip' | 'check' | 'fold' | 'turn' | 'win' | 'level' | 'bust' | 'click' | 'alert'
  | 'move' | 'allin' | 'bubble' | 'cash';

const SAVE_KEY = 'shortstack.nightone.v1';

interface SaveData {
  bankrollCents: number;
  heroName: string;
  results: { name: string; place: number; entrants: number; prize: number; accuracy: number }[];
  textsSeen: string[];
}

function load(): SaveData | null {
  try {
    const raw = localStorage.getItem(SAVE_KEY);
    return raw ? (JSON.parse(raw) as SaveData) : null;
  } catch {
    return null;
  }
}

export class Session {
  // ------------------------------------------------------------ persistent
  bankrollCents = 237;
  heroName = 'grinder_3c';
  history: SaveData['results'] = [];
  private textsSeen = new Set<string>();

  // ------------------------------------------------------------ ui state
  screen: Screen = 'boot';
  selected = 0;
  pace: Pace = 'smart';
  hud = true;
  rightTab: 'info' | 'chat' | 'payouts' = 'chat';
  confirmRegister = false;
  results: Results | null = null;
  resultsAt = 0;
  toast: { text: string; at: number } | null = null;
  banner: { title: string; sub: string; at: number; color: string } | null = null;

  // ------------------------------------------------------------ tournament
  lobbyEvent: LobbyEvent | null = null;
  t: Tournament | null = null;
  hand: Hand | null = null;
  tableId = 0;
  seats: (SeatVis | null)[] = [];
  buttonSeat = 0;
  board: Card[] = [];
  boardShownAt: number[] = [];
  pot = 0;
  flights: Flight[] = [];
  chat: ChatLine[] = [];
  prompt: HeroPrompt | null = null;
  badges: GradeBadge[] = [];
  grades: DecisionGrade[] = [];
  heroTilt = 0;
  sitOutNext = false;
  sittingOutThisHand = false;
  sprinting = false;
  sprintStopReason = '';
  handsPlayed = 0;
  biggestPot = 0;
  timeBank = 30;
  moving: { from: number; to: number; at: number } | null = null;
  autoFolded: { cards: Card[]; at: number } | null = null;
  lastLevelUpAt = -100;
  heroAllInReveal = false;

  private rng = new Rng(Date.now());
  private cursor = 0;
  private nextAt = 0;
  private botPending: { action: PlayerAction; at: number; seatIdx: number } | null = null;
  private handDone = false;
  private revealed = false;
  private heroFolded = false;
  private potBeforeAward = 0;
  private bustAt = 0;
  private bustInfo: { place: number; prize: number } | null = null;
  private lastIdleChat = 0;
  private rivalArrived = false;
  private cansShown = 1;
  private sprintMilestone: { bubble: boolean; final: boolean } = { bubble: false, final: false };
  now = 0;

  constructor(private hooks: Hooks) {
    const s = load();
    if (s) {
      this.bankrollCents = s.bankrollCents;
      this.heroName = s.heroName;
      this.history = s.results ?? [];
      this.textsSeen = new Set(s.textsSeen ?? []);
    }
  }

  save(): void {
    try {
      const data: SaveData = { bankrollCents: this.bankrollCents, heroName: this.heroName, results: this.history, textsSeen: [...this.textsSeen] };
      localStorage.setItem(SAVE_KEY, JSON.stringify(data));
    } catch {
      /* storage unavailable: progress lasts for this session only */
    }
  }

  resetSave(): void {
    this.bankrollCents = 237;
    this.history = [];
    this.textsSeen.clear();
    this.save();
  }

  // ------------------------------------------------------------ story

  private storyText(key: string, from: string, body: string, once = true): void {
    if (once && this.textsSeen.has(key)) return;
    this.textsSeen.add(key);
    this.hooks.text(from, body);
    this.save();
  }

  onBoot(): void {
    this.storyText('dee-intro', 'Dee', "Heard about the notice on your door. Tuesday game at the laundromat is still on if you need it. Don't play scared.", false);
  }

  // ------------------------------------------------------------ lobby

  canAfford(ev: LobbyEvent): boolean {
    return ev.spec !== null && this.bankrollCents >= ev.buyInCents;
  }

  register(index: number): void {
    const ev = LOBBY[index];
    if (!ev.spec || !this.canAfford(ev)) return;
    this.bankrollCents -= ev.buyInCents;
    this.save();
    this.lobbyEvent = ev;
    this.grades = [];
    this.badges = [];
    this.chat = [];
    this.handsPlayed = 0;
    this.biggestPot = 0;
    this.heroTilt = 0;
    this.results = null;
    this.bustInfo = null;
    this.rivalArrived = false;
    this.sprinting = false;
    this.timeBank = 30;
    const seed = `${ev.spec.id}:${Date.now()}`;
    const rival: { name: string; archetype: Archetype }[] = [{ name: RIVAL, archetype: 'crusher' }];
    this.t = new Tournament(ev.spec, this.heroName, seed, rival);
    this.screen = 'table';
    this.system(`Welcome to ${ev.spec.name}. ${chips(ev.spec.entrants)} players, ${this.t.paidPlaces} paid.`);
    this.system(`Blinds ${this.t.level.sb}/${this.t.level.bb}, ante ${this.t.level.ante}. Good luck!`);
    this.hooks.sound('alert');
    this.startNextHand();
  }

  // ------------------------------------------------------------ chat helpers

  private push(line: Omit<ChatLine, 'time'>): void {
    this.chat.push({ ...line, time: this.now });
    if (this.chat.length > 80) this.chat.splice(0, this.chat.length - 80);
  }
  dealer(text: string): void {
    this.push({ who: 'Dealer', text, kind: 'dealer' });
  }
  system(text: string): void {
    this.push({ who: '', text, kind: 'system' });
  }
  say(who: string, text: string): void {
    this.push({ who, text, kind: who === RIVAL ? 'rival' : 'player' });
  }

  // ------------------------------------------------------------ hand lifecycle

  private seatName(seat: number): string {
    return this.seats[seat]?.name ?? `Seat ${seat + 1}`;
  }

  private buildSeats(): void {
    const t = this.t!;
    const table = t.tables.get(this.tableId)!;
    this.seats = table.seats.map((id, seat) => {
      if (!id) return null;
      const p = t.players.get(id)!;
      return {
        seat, id, name: p.name, isHero: p.isHero, isRival: p.name === RIVAL, stack: p.stack, bet: 0, hole: [],
        hasCards: false, folded: false, allIn: false, lastAction: '', lastActionAt: 0, acting: false, actStart: 0,
        actEnd: 0, equity: null, winner: false, handLabel: '', dealtAt: 0, sittingOut: false,
      };
    });
  }

  startNextHand(): void {
    const t = this.t!;
    if (t.finished || t.hero.busted) return;
    const prevTable = this.tableId;

    // Story: seat the rival at the hero's table once things get serious.
    if (!this.rivalArrived && this.handsPlayed >= 14 && !t.inTheMoney) {
      const rival = t.players.get(`npc:${RIVAL}`);
      if (rival && !rival.busted) {
        t.moveToTable(rival.id, t.hero.tableId);
        this.rivalArrived = true;
      }
    }

    const { hand, events } = t.startTick();
    this.handleTourneyEvents(events);
    if (!hand) return;
    this.hand = hand;
    this.tableId = t.hero.tableId;
    if (prevTable !== 0 && prevTable !== this.tableId) {
      this.moving = { from: prevTable, to: this.tableId, at: this.now };
      this.system(`You have been moved to Table ${this.tableId}.`);
      this.hooks.sound('move');
    }
    this.buildSeats();
    if (this.rivalArrived && this.seats.some((s) => s?.isRival) && !this.chat.some((c) => c.kind === 'rival')) {
      this.say(RIVAL, this.rng.pick(RIVAL_LINES.arrive));
    }
    this.buttonSeat = hand.buttonSeat;
    this.board = [];
    this.boardShownAt = [];
    this.pot = 0;
    this.flights = [];
    this.cursor = 0;
    this.nextAt = this.now + (this.moving && this.moving.at === this.now ? 1.6 : 0.25);
    this.botPending = null;
    this.handDone = false;
    this.revealed = false;
    this.heroFolded = false;
    this.prompt = null;
    this.autoFolded = null;
    this.heroAllInReveal = false;
    this.sittingOutThisHand = this.sitOutNext;
    if (this.sitOutNext) this.heroTilt *= 0.5;
    this.sitOutNext = false;
    this.heroTilt *= 0.93;
    this.handsPlayed++;
    this.dealer(`Hand #${(t.tick + 1).toLocaleString()} · Blinds ${chips(t.level.sb)}/${chips(t.level.bb)}`);
  }

  /** Visual speed: once the hero is out of the hand, Smart pace fast-forwards. */
  private get speed(): number {
    if (this.pace === 'full') return 1;
    const heroOut = this.heroFolded || !this.seats.some((s) => s?.isHero && s.hasCards && !s.folded);
    return heroOut ? 0.22 : 1;
  }

  private heroSeatIdx(): number {
    return this.hand ? this.hand.seats.findIndex((s) => s.id === HERO_ID) : -1;
  }

  update(now: number): void {
    this.now = now;
    this.flights = this.flights.filter((f) => now < f.start + f.dur + 0.05);
    this.badges = this.badges.filter((b) => now < b.at + 3.2);
    if (this.toast && now > this.toast.at + 6) this.toast = null;
    if (this.banner && now > this.banner.at + 3.2) this.banner = null;
    if (this.moving && now > this.moving.at + 2.2) this.moving = null;

    if (this.screen !== 'table' || !this.t) return;
    if (this.sprinting) {
      this.sprintStep();
      return;
    }
    if (this.bustInfo) {
      if (now > this.bustAt) this.showResults();
      return;
    }
    const hand = this.hand;
    if (!hand) return;

    // Idle chatter.
    if (now - this.lastIdleChat > 50 && this.rng.chance(0.004)) {
      this.lastIdleChat = now;
      const others = this.seats.filter((s) => s && !s.isHero && !s.isRival) as SeatVis[];
      if (others.length) this.say(this.rng.pick(others).name, idle(this.rng));
    }

    // Hero timer.
    if (this.prompt) {
      const p = this.prompt;
      if (now > p.deadline) {
        if (p.timeBankUntil === 0 && this.timeBank >= 1) {
          // Time bank kicks in automatically.
          p.timeBankUntil = now + this.timeBank;
          this.system('Time bank activated.');
          this.hooks.sound('alert');
        } else if (p.timeBankUntil === 0 || now > p.timeBankUntil) {
          this.heroAct(p.canCheck ? { type: 'check' } : { type: 'fold' }, true);
        }
      }
      return;
    }

    let guard = 0;
    while (now >= this.nextAt && guard++ < 50) {
      if (this.cursor < hand.events.length) {
        const ev = hand.events[this.cursor++];
        this.nextAt = now + this.consume(ev) * this.speed;
        continue;
      }
      if (hand.complete) {
        if (!this.handDone) {
          this.handDone = true;
          this.nextAt = now + 1.4 * Math.max(0.5, this.speed);
          continue;
        }
        this.endHand();
        return;
      }
      const idx = hand.toAct!;
      const seatNo = hand.seats[idx].seat;
      const vis = this.seats[seatNo]!;
      if (hand.seats[idx].id === HERO_ID) {
        this.openHeroTurn();
        return;
      }
      if (!this.botPending) {
        const d = this.t.botDecision(hand, false);
        const think = (d.thinkMs / 1000) * (this.speed < 1 ? 0.12 : 0.55);
        this.botPending = { action: d.action, at: now + think, seatIdx: idx };
        vis.acting = true;
        vis.actStart = now;
        vis.actEnd = now + Math.max(think, 0.4);
        this.nextAt = now + think;
        continue;
      }
      vis.acting = false;
      const a = this.botPending.action;
      this.botPending = null;
      hand.act(a);
    }
  }

  // ------------------------------------------------------------ hero turn

  private shouldAutoFold(): boolean {
    if (this.sittingOutThisHand) return true;
    if (this.pace === 'full') return false;
    const hand = this.hand!;
    const idx = this.heroSeatIdx();
    if (hand.street !== 'preflop') return false;
    const view = makeView(hand, idx);
    const pct = handPercentile(view.hole[0], view.hole[1]);
    const sum = preflopSummary(view);
    const bb = hand.bigBlind;
    const stackBB = (view.seats[idx].stack + view.seats[idx].streetBet) / bb;
    if (view.canCheck) return false; // free option: always let the hero decide
    const pos = positionOf(view, idx);
    if (stackBB <= 15 && sum.raises === 0) return pct > pushRange(stackBB, playersBehind(view, idx)) * 1.4;
    if (sum.raises === 0) {
      const open: Record<string, number> = { UTG: 0.16, 'UTG+1': 0.18, MP: 0.2, LJ: 0.24, HJ: 0.28, CO: 0.36, BTN: 0.55, SB: 0.5, BB: 1 };
      return pct > (open[pos] ?? 0.3);
    }
    if (idx === hand.bigBlindIndex) return pct > 0.5;
    return pct > 0.22;
  }

  private openHeroTurn(): void {
    const hand = this.hand!;
    const idx = this.heroSeatIdx();
    const legal = hand.legalActions();
    if (this.sittingOutThisHand && legal.canCheck) {
      hand.act({ type: 'check' });
      this.nextAt = this.now + 0.15;
      return;
    }
    if (this.shouldAutoFold()) {
      const hole = [...hand.seats[idx].hole];
      this.autoFolded = { cards: hole, at: this.now };
      hand.act({ type: 'fold' });
      if (!this.sittingOutThisHand) this.dealer(`Auto-folded ${hole.map(cardToString).join(' ')}`);
      this.heroFolded = true;
      this.nextAt = this.now + 0.15;
      return;
    }
    const view = makeView(hand, idx);
    const analysis = analyzeDecision(view, this.rng, 1200);
    const bb = hand.bigBlind;
    let raiseTo: number;
    if (legal.isBet) raiseTo = Math.max(legal.minRaiseTo, Math.round((hand.pot * 0.5) / (bb / 2)) * (bb / 2));
    else if (hand.street === 'preflop') raiseTo = Math.max(legal.minRaiseTo, Math.round(hand.currentBet * 2.5));
    else raiseTo = Math.max(legal.minRaiseTo, Math.round(hand.currentBet * 3));
    raiseTo = Math.min(raiseTo, legal.maxRaiseTo);
    const tiltPenalty = 1 - Math.min(0.4, this.heroTilt * 0.5);
    this.prompt = {
      analysis,
      openedAt: this.now,
      deadline: this.now + 20 * tiltPenalty,
      timeBankUntil: 0,
      raiseTo,
      minRaise: legal.minRaiseTo,
      maxRaise: legal.maxRaiseTo,
      toCall: legal.callAmount,
      canCheck: legal.canCheck,
      canRaise: legal.canRaise,
      isBet: legal.isBet,
      pot: hand.pot,
      bigBlind: bb,
      street: hand.street,
    };
    const vis = this.seats[hand.seats[idx].seat]!;
    vis.acting = true;
    vis.actStart = this.now;
    vis.actEnd = this.prompt.deadline;
    this.hooks.sound('turn');
  }

  heroAct(action: PlayerAction, timedOut = false): void {
    const p = this.prompt;
    const hand = this.hand;
    if (!p || !hand) return;
    const idx = this.heroSeatIdx();
    const legal = hand.legalActions();
    let a = action;
    if (a.type === 'raise') {
      if (!legal.canRaise) a = legal.callAmount > 0 ? { type: 'call' } : { type: 'check' };
      else a = { type: 'raise', to: Math.max(legal.minRaiseTo, Math.min(legal.maxRaiseTo, Math.round(a.to))) };
    }
    if (a.type === 'check' && !legal.canCheck) a = { type: 'call' };
    if (p.timeBankUntil > 0) this.timeBank = Math.max(0, this.timeBank - (this.now - p.deadline));
    const grade = gradeDecision(p.analysis, a);
    this.grades.push(grade);
    this.badges.push({ grade, at: this.now });
    if (timedOut) this.system('You ran out of time.');
    this.prompt = null;
    const vis = this.seats[hand.seats[idx].seat]!;
    vis.acting = false;
    if (a.type === 'fold') this.heroFolded = true;
    hand.act(a);
    this.nextAt = this.now;
  }

  requestSitOut(): void {
    this.sitOutNext = true;
    this.system('You will sit out the next hand. Breathe.');
  }

  // ------------------------------------------------------------ event playback

  /** Apply one hand event to the visual state; returns its duration in seconds. */
  private consume(ev: HandEvent): number {
    const now = this.now;
    const hand = this.hand!;
    switch (ev.t) {
      case 'blind': {
        const s = this.seats[ev.seat]!;
        s.stack -= ev.amount;
        s.bet += ev.amount;
        this.flights.push({ kind: 'chips', from: { seat: ev.seat }, to: { bet: ev.seat }, amount: ev.amount, start: now, dur: 0.3 });
        return 0.12;
      }
      case 'ante': {
        const s = this.seats[ev.seat]!;
        s.stack -= ev.amount;
        this.pot += ev.amount;
        this.flights.push({ kind: 'chips', from: { seat: ev.seat }, to: { pot: true }, amount: ev.amount, start: now, dur: 0.35 });
        this.hooks.sound('chip', { volume: 0.5 });
        return 0.25;
      }
      case 'deal': {
        let k = 0;
        for (let round = 0; round < 2; round++) {
          for (const seatNo of ev.order) {
            const s = this.seats[seatNo]!;
            s.hasCards = true;
            s.dealtAt = now;
            this.flights.push({ kind: 'card', from: { deck: true }, to: { seat: seatNo }, amount: round, start: now + k * 0.05 * this.speed, dur: 0.28 });
            k++;
          }
        }
        const heroSeat = hand.seats.find((s) => s.id === HERO_ID);
        if (heroSeat) this.seats[heroSeat.seat]!.hole = [...heroSeat.hole];
        this.hooks.sound('card');
        return 0.2 + k * 0.05;
      }
      case 'action': {
        const s = this.seats[ev.seat]!;
        s.stack -= ev.added;
        s.bet = ev.to;
        s.allIn = ev.allIn;
        s.lastActionAt = now;
        const verb =
          ev.type === 'fold' ? 'Fold' : ev.type === 'check' ? 'Check' : ev.type === 'call' ? `Call ${chips(ev.added)}` : ev.type === 'bet' ? `Bet ${chips(ev.to)}` : `Raise ${chips(ev.to)}`;
        s.lastAction = ev.allIn ? 'All-in' : verb;
        if (ev.type === 'fold') {
          s.folded = true;
          this.flights.push({ kind: 'card', from: { seat: ev.seat }, to: { muck: true }, amount: 0, start: now, dur: 0.3 });
          this.hooks.sound('fold', { volume: 0.6 });
        } else if (ev.type === 'check') this.hooks.sound('check');
        else {
          this.flights.push({ kind: 'chips', from: { seat: ev.seat }, to: { bet: ev.seat }, amount: ev.added, start: now, dur: 0.3 });
          this.hooks.sound(ev.allIn ? 'allin' : 'chips');
        }
        const name = s.name;
        const text =
          ev.type === 'fold' ? `${name} folds` : ev.type === 'check' ? `${name} checks` : ev.type === 'call' ? `${name} calls ${chips(ev.added)}` : ev.type === 'bet' ? `${name} bets ${chips(ev.to)}` : `${name} raises to ${chips(ev.to)}`;
        if (ev.type !== 'fold' || this.speed === 1) this.dealer(text + (ev.allIn ? ' and is all-in' : ''));
        return ev.type === 'fold' ? 0.25 : 0.45;
      }
      case 'street': {
        const collected = this.collectBets();
        const base = collected ? 0.35 : 0.05;
        for (let i = 0; i < ev.cards.length; i++) {
          this.board.push(ev.cards[i]);
          this.boardShownAt.push(now + base * this.speed + i * 0.12 * this.speed);
        }
        for (const s of this.seats) if (s && !s.allIn && !s.folded) s.lastAction = '';
        this.dealer(`${ev.street[0].toUpperCase() + ev.street.slice(1)}: ${ev.board.map(cardToString).join(' ')}`);
        this.hooks.sound('flip');
        if (this.revealed) this.updateEquities();
        return base + 0.55 + ev.cards.length * 0.12 + (this.revealed ? 1.1 : 0);
      }
      case 'return': {
        const s = this.seats[ev.seat]!;
        s.bet -= ev.amount;
        s.stack += ev.amount;
        return 0.1;
      }
      case 'reveal': {
        this.revealed = true;
        this.collectBets();
        for (const hs of hand.seats) {
          if (hs.folded) continue;
          const s = this.seats[hs.seat]!;
          s.hole = [...hs.hole];
        }
        this.updateEquities();
        const heroIn = hand.seats.some((s) => s.id === HERO_ID && !s.folded);
        if (heroIn) {
          this.heroAllInReveal = true;
          this.hooks.heartbeat(true);
        }
        this.hooks.sound('flip');
        return 1.6;
      }
      case 'showdown': {
        this.collectBets();
        for (const h of ev.hands) {
          const s = this.seats[h.seat]!;
          s.hole = [...h.hole];
          s.handLabel = describe(h.score);
        }
        this.hooks.sound('flip');
        return 1.1;
      }
      case 'award': {
        if (ev.index === 0) {
          this.collectBets();
          this.potBeforeAward = this.pot;
        }
        ev.pot.winners.forEach((seat, k) => {
          const s = this.seats[seat]!;
          s.winner = true;
          s.stack += ev.pot.shares[k];
          this.flights.push({ kind: 'chips', from: { pot: true }, to: { seat }, amount: ev.pot.shares[k], start: now, dur: 0.6 });
        });
        this.pot = Math.max(0, this.pot - ev.pot.amount);
        const names = ev.pot.winners.map((w) => this.seatName(w)).join(' & ');
        const label = ev.pot.winners.length === 1 ? this.seats[ev.pot.winners[0]]!.handLabel : '';
        const potName = ev.potCount > 1 ? (ev.index === 0 ? 'the main pot' : `side pot ${ev.index}`) : 'the pot';
        this.dealer(`${names} wins ${potName} (${chips(ev.pot.amount)})${label ? ` with ${label}` : ''}`);
        const heroWon = ev.pot.winners.some((w) => this.seats[w]?.isHero);
        this.hooks.sound(heroWon ? 'win' : 'chips', { volume: heroWon ? 1 : 0.6 });
        if (heroWon) this.biggestPot = Math.max(this.biggestPot, ev.pot.amount);
        if (ev.index === ev.potCount - 1) this.afterAward();
        return 0.9;
      }
      case 'end':
        this.hooks.heartbeat(false);
        return 0.1;
    }
  }

  private collectBets(): boolean {
    let any = false;
    for (const s of this.seats) {
      if (s && s.bet > 0) {
        this.flights.push({ kind: 'chips', from: { bet: s.seat }, to: { pot: true }, amount: s.bet, start: this.now, dur: 0.35 });
        this.pot += s.bet;
        s.bet = 0;
        any = true;
      }
    }
    if (any) this.hooks.sound('chips', { volume: 0.5 });
    return any;
  }

  private updateEquities(): void {
    const hand = this.hand!;
    const live = hand.seats.filter((s) => !s.folded);
    const eq = equityKnownHands(live.map((s) => s.hole), this.board, this.rng, 6000);
    live.forEach((s, i) => (this.seats[s.seat]!.equity = eq[i]));
  }

  /** Chat reactions, bad beats and tilt after a pot is decided. */
  private afterAward(): void {
    const hand = this.hand!;
    const winners = new Set(hand.potResults.flatMap((p) => p.winners));
    const heroSeat = hand.seats.find((s) => s.id === HERO_ID);
    const heroVis = heroSeat ? this.seats[heroSeat.seat] : null;
    const heroWasIn = heroSeat && !heroSeat.folded;
    const bigPot = this.potBeforeAward > hand.bigBlind * 20;

    if (heroWasIn && heroVis) {
      const lost = !winners.has(heroSeat!.seat);
      const eq = heroVis.equity;
      if (lost && eq !== null && eq >= 0.6) {
        this.heroTilt = Math.min(1, this.heroTilt + 0.45);
        this.system(`Bad beat. You were ${Math.round(eq * 100)}% to win.`);
      } else if (lost && bigPot) {
        this.heroTilt = Math.min(1, this.heroTilt + 0.2);
      } else if (!lost) {
        this.heroTilt = Math.max(0, this.heroTilt - 0.15);
      }
      // Rival reactions.
      const rivalSeat = hand.seats.find((s) => s.id === `npc:${RIVAL}` && !s.folded);
      if (rivalSeat) {
        if (winners.has(rivalSeat.seat) && lost) this.say(RIVAL, this.rng.pick(RIVAL_LINES.winVsHero));
        else if (!lost && !winners.has(rivalSeat.seat) && this.rng.chance(0.6)) this.say(RIVAL, this.rng.pick(RIVAL_LINES.loseVsHero));
      }
    }
    if (bigPot && this.rng.chance(0.55)) {
      const losers = hand.seats.filter((s) => !s.folded && !winners.has(s.seat) && s.id !== HERO_ID && s.id !== `npc:${RIVAL}`);
      const winnersNpc = hand.seats.filter((s) => winners.has(s.seat) && s.id !== HERO_ID && s.id !== `npc:${RIVAL}`);
      if (losers.length && this.rng.chance(0.6)) this.say(this.seatName(losers[0].seat), salt(this.rng));
      else if (winnersNpc.length) this.say(this.seatName(winnersNpc[0].seat), brag(this.rng));
      else if (heroWasIn) {
        const other = hand.seats.find((s) => s.id !== HERO_ID && !winners.has(s.seat));
        if (other) this.say(this.seatName(other.seat), nice(this.rng));
      }
    }
  }

  private endHand(): void {
    const t = this.t!;
    const hand = this.hand!;
    const heroStart = hand.seats.find((s) => s.id === HERO_ID)?.startStack ?? 0;
    const events = t.finishTick(hand);
    this.hooks.heartbeat(false);
    this.handleTourneyEvents(events);
    // Busted players at the hero's table say goodbye.
    for (const e of events) {
      if (e.t === 'bust' && !e.isHero && e.tableId === this.tableId && this.rng.chance(0.45)) {
        if (e.name === RIVAL) this.say(RIVAL, this.rng.pick(RIVAL_LINES.bustsSelf));
        else this.say(e.name, bustLine(this.rng));
      }
    }
    if (t.hero.busted) {
      const rivalWon = hand.potResults.some((p) => p.winners.some((w) => hand.seatByNumber(w)?.id === `npc:${RIVAL}`));
      if (rivalWon) this.say(RIVAL, this.rng.pick(RIVAL_LINES.heroBust));
      this.bustInfo = { place: t.hero.place, prize: t.hero.prizeCents };
      this.bustAt = this.now + 3;
      this.hooks.sound('bust');
      this.banner = {
        title: t.hero.prizeCents > 0 ? `You finished ${ordinal(t.hero.place)}` : `Eliminated in ${ordinal(t.hero.place)}`,
        sub: t.hero.prizeCents > 0 ? `Won ${money(t.hero.prizeCents)}` : 'Better luck next time',
        at: this.now,
        color: t.hero.prizeCents > 0 ? '#f2c14e' : '#ef4d5a',
      };
      void heroStart;
      return;
    }
    if (t.finished) {
      this.bustInfo = { place: 1, prize: t.hero.prizeCents };
      this.bustAt = this.now + 4;
      this.hooks.celebrate();
      this.banner = { title: 'YOU WON THE TOURNAMENT', sub: money(t.hero.prizeCents), at: this.now, color: '#f2c14e' };
      this.storyText('won-' + this.lobbyEvent!.spec!.id, 'Dee', 'You did WHAT?! Pay the rent. Then come see me. We need to talk about your future.', false);
      return;
    }
    // Hourly can of energy drink.
    const hoursIn = Math.floor((t.clockMinutes - t.spec.startClock) / 50);
    while (this.cansShown <= hoursIn) {
      this.cansShown++;
      this.hooks.addCan();
    }
    if (this.pace === 'sprint') {
      this.beginSprint();
      return;
    }
    this.startNextHand();
  }

  private handleTourneyEvents(events: TEvent[]): void {
    const t = this.t!;
    for (const e of events) {
      switch (e.t) {
        case 'level':
          this.system(`Blinds are now ${chips(e.blinds.sb)}/${chips(e.blinds.bb)}, ante ${chips(e.blinds.ante)}.`);
          this.timeBank = Math.min(60, this.timeBank + 5);
          this.lastLevelUpAt = this.now;
          this.hooks.sound('level');
          break;
        case 'handForHand':
          this.system('We are on the bubble. Hand-for-hand play.');
          this.banner = { title: 'THE BUBBLE', sub: `${t.remaining - t.paidPlaces} away from the money`, at: this.now, color: '#f28a3a' };
          this.hooks.sound('bubble');
          if (!t.hero.busted) this.storyText('dee-bubble', 'Dee', 'Bubble? Breathe. Fold the trash, shove the good ones. Nobody remembers who min-cashed scared.');
          break;
        case 'bubble':
          this.system(`${e.bubbleName} bursts the bubble. Everyone left is in the money!`);
          if (!t.hero.busted) {
            this.banner = { title: 'IN THE MONEY', sub: `Min cash ${money(t.payouts[t.payouts.length - 1])}`, at: this.now, color: '#3ecf6e' };
            this.hooks.sound('cash');
            this.storyText('dee-itm', 'Dee', "In the money. Now play to win, not to min-cash.");
          }
          break;
        case 'finalTable':
          if (!t.hero.busted) {
            this.system('Final table! Good luck to all nine players.');
            this.banner = { title: 'FINAL TABLE', sub: `${ordinal(9)} pays ${money(t.prizeFor(9))} · 1st pays ${money(t.prizeFor(1))}`, at: this.now, color: '#f2c14e' };
            this.hooks.sound('bubble');
            this.storyText('dee-ft', 'Dee', 'A final table? At this hour? Call me when it is over. Win or lose.');
          }
          break;
        case 'tableBroken':
        case 'moved':
        case 'bust':
        case 'finished':
          break;
      }
    }
  }

  // ------------------------------------------------------------ sprint

  beginSprint(): void {
    const t = this.t!;
    this.sprinting = true;
    this.sprintStopReason = '';
    this.prompt = null;
    this.sprintMilestone = { bubble: t.remaining <= t.paidPlaces + 15, final: t.tables.size === 1 };
  }

  stopSprint(reason = 'Stopped'): void {
    this.sprinting = false;
    this.pace = 'smart';
    this.sprintStopReason = reason;
    this.system(`Sprint stopped: ${reason}.`);
    if (!this.t!.hero.busted && !this.t!.finished) this.startNextHand();
  }

  private sprintStep(): void {
    const t = this.t!;
    const t0 = performance.now();
    while (performance.now() - t0 < 10) {
      const events = t.simulateTick(sprintProfile());
      this.handsPlayed++;
      this.handleTourneyEvents(events);
      if (t.hero.busted) {
        this.sprinting = false;
        this.bustInfo = { place: t.hero.place, prize: t.hero.prizeCents };
        this.bustAt = this.now + 2.5;
        this.hooks.sound('bust');
        this.banner = {
          title: t.hero.prizeCents > 0 ? `You finished ${ordinal(t.hero.place)}` : `Eliminated in ${ordinal(t.hero.place)}`,
          sub: t.hero.prizeCents > 0 ? `Won ${money(t.hero.prizeCents)}` : 'Busted while sprinting',
          at: this.now,
          color: t.hero.prizeCents > 0 ? '#f2c14e' : '#ef4d5a',
        };
        return;
      }
      if (t.finished) {
        this.sprinting = false;
        this.bustInfo = { place: 1, prize: t.hero.prizeCents };
        this.bustAt = this.now + 3;
        this.hooks.celebrate();
        return;
      }
      if (!this.sprintMilestone.bubble && t.remaining <= t.paidPlaces + 15) return this.stopSprint('approaching the bubble');
      if (!this.sprintMilestone.final && t.tables.size === 1) return this.stopSprint('final table');
    }
  }

  // ------------------------------------------------------------ results

  private showResults(): void {
    const t = this.t!;
    const info = this.bustInfo!;
    this.bankrollCents += info.prize;
    const graded = this.grades.length;
    const acc = graded ? this.grades.reduce((a, g) => a + ({ Best: 100, Good: 88, Inaccuracy: 62, Mistake: 30, Blunder: 0 }[g.grade]), 0) / graded : 0;
    this.results = {
      eventName: t.spec.name,
      place: info.place,
      entrants: t.spec.entrants,
      prizeCents: info.prize,
      buyInCents: t.spec.buyInCents,
      hands: this.handsPlayed,
      grades: [...this.grades],
      accuracy: acc,
      biggestPot: this.biggestPot,
      won: info.place === 1,
      bustHand: '',
    };
    this.history.unshift({ name: t.spec.name, place: info.place, entrants: t.spec.entrants, prize: info.prize, accuracy: acc });
    this.history = this.history.slice(0, 20);
    this.bustInfo = null;
    this.hand = null;
    this.screen = 'results';
    this.resultsAt = this.now;
    this.hooks.heartbeat(false);
    this.save();
    if (this.bankrollCents < 25 && info.prize === 0) {
      this.storyText('broke', 'Dee', "Broke? It happens to everybody. Freeroll's always running. Or come by Tuesday. Bring quarters for the dryers.", false);
    } else if (info.prize > 0) {
      this.storyText('first-cash', 'Landlord', `Saw your light on all night. Rent + late fee is $1,225. Friday.`);
    }
  }

  leaveResults(): void {
    this.screen = 'lobby';
    this.results = null;
    this.t = null;
  }

  /** How far into the dawn the sky should be, from the tournament clock. */
  clockMinutes(): number {
    return this.t ? this.t.clockMinutes : 2 * 60 + 7;
  }
}
