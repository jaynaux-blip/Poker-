import { decide } from './ai/bot';
import { Archetype, POPULATIONS, Profile, makeProfile, rollArchetype } from './ai/profiles';
import { makeView } from './ai/view';
import { Hand } from './hand';
import { uniqueNames } from './names';
import { Rng } from './rng';
import { Level, STANDARD_LEVELS, payoutTable } from './structure';

/**
 * Multi-table tournament.
 *
 * Every table in the field plays real hands with the same engine. The table
 * the hero sits at uses full-strength bots; the rest of the field uses the
 * lightweight bot mode so a 1,000-player event runs in a few milliseconds per
 * round. One "tick" is one hand at every table (hand-for-hand timing), which
 * keeps eliminations and the tournament clock consistent across the field.
 */

export interface TournamentSpec {
  id: string;
  name: string;
  buyInCents: number; // total paid, including fee
  feeCents: number;
  guaranteeCents: number; // prize pool floor (fixed pool for freerolls)
  entrants: number;
  startingStack: number;
  levelMinutes: number;
  secondsPerHand: number;
  population: keyof typeof POPULATIONS;
  speed: 'Regular' | 'Turbo' | 'Hyper';
  /** Game clock at the start, minutes after midnight. */
  startClock: number;
  tableSize?: number;
  levels?: Level[];
}

export interface TPlayer {
  id: string;
  name: string;
  isHero: boolean;
  profile: Profile | null;
  stack: number;
  tableId: number;
  seat: number;
  busted: boolean;
  place: number;
  prizeCents: number;
  tilt: number;
  hands: number;
  vpipHands: number;
  pfrHands: number;
}

export interface TTable {
  id: number;
  seats: (string | null)[];
  buttonSeat: number;
}

export type TEvent =
  | { t: 'bust'; id: string; name: string; place: number; prizeCents: number; tableId: number; isHero: boolean }
  | { t: 'level'; level: number; blinds: Level }
  | { t: 'moved'; id: string; from: number; to: number; isHero: boolean }
  | { t: 'tableBroken'; tableId: number }
  | { t: 'handForHand' }
  | { t: 'bubble'; bubbleName: string }
  | { t: 'finalTable' }
  | { t: 'finished'; winnerId: string; winnerName: string };

export const HERO_ID = 'hero';

export class Tournament {
  readonly spec: TournamentSpec;
  readonly rng: Rng;
  readonly players = new Map<string, TPlayer>();
  readonly tables = new Map<number, TTable>();
  readonly payouts: number[]; // cents by place (index 0 = 1st)
  readonly prizePoolCents: number;
  readonly levels: Level[];
  readonly tableSize: number;
  readonly log: TEvent[] = [];

  tick = 0;
  levelIndex = 0;
  remaining: number;
  finished = false;
  private announcedFinal = false;
  private announcedH4H = false;
  private burstBubble = false;
  private startStacks = new Map<string, number>();

  constructor(spec: TournamentSpec, heroName: string, seed: string | number, reservedNames: { name: string; archetype: Archetype }[] = []) {
    this.spec = spec;
    this.rng = new Rng(seed);
    this.levels = spec.levels ?? STANDARD_LEVELS;
    this.tableSize = spec.tableSize ?? 9;
    this.remaining = spec.entrants;
    const pool = Math.max(spec.guaranteeCents, spec.entrants * (spec.buyInCents - spec.feeCents));
    this.prizePoolCents = pool;
    const minCash = spec.buyInCents > 0 ? Math.round((spec.buyInCents - spec.feeCents) * 1.4) : 20;
    this.payouts = payoutTable(pool, spec.entrants, minCash);

    const nameRng = this.rng.fork('names');
    const names = uniqueNames(spec.entrants - 1 - reservedNames.length, nameRng, [heroName, ...reservedNames.map((r) => r.name)]);
    const population = POPULATIONS[spec.population];
    const people: TPlayer[] = [];
    const mk = (id: string, name: string, profile: Profile | null, isHero: boolean): TPlayer => ({
      id, name, isHero, profile, stack: spec.startingStack, tableId: -1, seat: -1, busted: false, place: 0,
      prizeCents: 0, tilt: 0, hands: 0, vpipHands: 0, pfrHands: 0,
    });
    people.push(mk(HERO_ID, heroName, null, true));
    for (const r of reservedNames) people.push(mk(`npc:${r.name}`, r.name, makeProfile(r.archetype, this.rng), false));
    names.forEach((name, i) => people.push(mk(`p${i}`, name, makeProfile(rollArchetype(population, this.rng), this.rng), false)));
    for (const p of people) this.players.set(p.id, p);

    // Seat everyone: fill tables evenly in random order.
    const tableCount = Math.ceil(spec.entrants / this.tableSize);
    for (let t = 1; t <= tableCount; t++) {
      this.tables.set(t, { id: t, seats: new Array(this.tableSize).fill(null), buttonSeat: -1 });
    }
    const order = this.rng.shuffle([...people]);
    order.forEach((p, i) => {
      const table = this.tables.get((i % tableCount) + 1)!;
      const empty = table.seats.map((s, k) => (s === null ? k : -1)).filter((k) => k >= 0);
      const seat = this.rng.pick(empty);
      table.seats[seat] = p.id;
      p.tableId = table.id;
      p.seat = seat;
    });
    for (const t of this.tables.values()) {
      const occupied = t.seats.map((s, k) => (s ? k : -1)).filter((k) => k >= 0);
      t.buttonSeat = this.rng.pick(occupied);
    }
  }

  // ---------------------------------------------------------------- queries

  get hero(): TPlayer {
    return this.players.get(HERO_ID)!;
  }

  get level(): Level {
    return this.levels[Math.min(this.levelIndex, this.levels.length - 1)];
  }

  get nextLevel(): Level {
    return this.levels[Math.min(this.levelIndex + 1, this.levels.length - 1)];
  }

  get paidPlaces(): number {
    return this.payouts.length;
  }

  get inTheMoney(): boolean {
    return this.remaining <= this.paidPlaces;
  }

  get handForHand(): boolean {
    return !this.inTheMoney && this.remaining <= this.paidPlaces + Math.max(2, Math.ceil(this.tables.size / 3));
  }

  get elapsedSeconds(): number {
    return this.tick * this.spec.secondsPerHand;
  }

  /** Seconds until the next level. */
  get levelSecondsLeft(): number {
    const len = this.spec.levelMinutes * 60;
    return len - (this.elapsedSeconds % len);
  }

  get clockMinutes(): number {
    return this.spec.startClock + this.elapsedSeconds / 60;
  }

  get averageStack(): number {
    return (this.spec.entrants * this.spec.startingStack) / Math.max(1, this.remaining);
  }

  alivePlayers(): TPlayer[] {
    return [...this.players.values()].filter((p) => !p.busted);
  }

  standings(): TPlayer[] {
    return this.alivePlayers().sort((a, b) => b.stack - a.stack);
  }

  heroRank(): number {
    const hero = this.hero;
    if (hero.busted) return hero.place;
    let rank = 1;
    for (const p of this.players.values()) if (!p.busted && p.stack > hero.stack) rank++;
    return rank;
  }

  prizeFor(place: number): number {
    return place >= 1 && place <= this.payouts.length ? this.payouts[place - 1] : 0;
  }

  tableOf(id: string): TTable | undefined {
    const p = this.players.get(id);
    return p ? this.tables.get(p.tableId) : undefined;
  }

  /** Survival pressure a player feels right now (bubble, final table pay jumps). */
  pressureFor(p: TPlayer): number {
    const R = this.remaining;
    const paid = this.paidPlaces;
    if (R <= 1) return 0;
    const bbs = p.stack / this.level.bb;
    if (R > paid) {
      const window = Math.max(3, paid * 0.25);
      const closeness = Math.max(0, Math.min(1, 1 - (R - paid) / window));
      if (closeness === 0) return 0;
      const stackFactor = bbs < 8 ? 0.35 : p.stack > this.averageStack * 2 ? 0.25 : 1;
      return closeness * stackFactor;
    }
    if (R <= this.tableSize) return 0.45 * (bbs < 8 ? 0.5 : 1);
    return 0.12;
  }

  // ---------------------------------------------------------------- hands

  /** Create the next hand at a table, moving the button. */
  makeHand(table: TTable): Hand | null {
    const occupied = table.seats.map((s, k) => (s ? k : -1)).filter((k) => k >= 0);
    if (occupied.length < 2) return null;
    // Move the button to the next occupied seat clockwise.
    let b = table.buttonSeat;
    for (let k = 1; k <= this.tableSize; k++) {
      const s = (b + k) % this.tableSize;
      if (table.seats[s]) {
        b = s;
        break;
      }
    }
    table.buttonSeat = b;
    const lvl = this.level;
    const hand = new Hand({
      players: occupied.map((seat) => {
        const id = table.seats[seat]!;
        return { seat, id, stack: this.players.get(id)!.stack };
      }),
      buttonSeat: b,
      smallBlind: lvl.sb,
      bigBlind: lvl.bb,
      ante: lvl.ante,
      rng: this.rng,
    });
    for (const s of hand.seats) this.startStacks.set(s.id, s.startStack);
    return hand;
  }

  /** Let a bot at this hand make its decision (full strength unless `fast`). */
  botDecision(hand: Hand, fast: boolean, profileOverride?: Profile) {
    const idx = hand.toAct!;
    const p = this.players.get(hand.seats[idx].id)!;
    const profile = profileOverride ?? p.profile!;
    return decide(makeView(hand, idx), {
      profile,
      tilt: p.tilt,
      icmPressure: this.pressureFor(p),
      fast,
      rng: this.rng,
    });
  }

  private playInstant(hand: Hand, fast: boolean, heroProfile?: Profile): void {
    let guard = 0;
    while (!hand.complete) {
      const id = hand.seats[hand.toAct!].id;
      const d = this.botDecision(hand, fast, id === HERO_ID ? heroProfile : undefined);
      hand.act(d.action);
      if (++guard > 1000) throw new Error('Hand did not terminate');
    }
  }

  /** Copy a finished hand's stacks back to players and update stats and tilt. */
  private applyHand(hand: Hand): void {
    const vpip = new Set<string>();
    const pfr = new Set<string>();
    for (const h of hand.history) {
      if (h.street !== 'preflop') break;
      const id = hand.seatByNumber(h.seat)!.id;
      if (h.type === 'call' || h.type === 'raise' || h.type === 'bet') vpip.add(id);
      if (h.type === 'raise' || h.type === 'bet') pfr.add(id);
    }
    for (const s of hand.seats) {
      const p = this.players.get(s.id)!;
      p.stack = s.stack;
      p.hands++;
      if (vpip.has(s.id)) p.vpipHands++;
      if (pfr.has(s.id)) p.pfrHands++;
      p.tilt *= 0.93;
      const lost = s.startStack - s.stack;
      if (p.profile && s.stack > 0 && lost > s.startStack * 0.4) {
        p.tilt = Math.min(1, p.tilt + p.profile.tiltProne * 0.6 * (lost / s.startStack));
      }
    }
  }

  // ---------------------------------------------------------------- ticks

  /**
   * Start a round: rebalance tables, then deal the hero's next hand.
   * Returns null when the hero is not playing (busted or tournament over).
   */
  startTick(): { hand: Hand | null; events: TEvent[] } {
    const events = this.balance();
    const hero = this.hero;
    let hand: Hand | null = null;
    if (!hero.busted && !this.finished) hand = this.makeHand(this.tables.get(hero.tableId)!);
    this.log.push(...events);
    return { hand, events };
  }

  /**
   * Finish a round: apply the hero's hand (if any), play every other table,
   * then process eliminations, the clock and milestones.
   */
  finishTick(heroHand: Hand | null, heroAuto?: Profile): TEvent[] {
    const events: TEvent[] = [];
    const heroTableId = this.hero.busted ? -1 : this.hero.tableId;
    if (heroHand) {
      if (!heroHand.complete) this.playInstant(heroHand, false, heroAuto);
      this.applyHand(heroHand);
    }
    for (const table of this.tables.values()) {
      if (table.id === heroTableId && heroHand) continue;
      const hand = this.makeHand(table);
      if (!hand) continue;
      this.playInstant(hand, true);
      this.applyHand(hand);
    }
    events.push(...this.processEliminations());
    this.tick++;
    const newLevel = Math.floor(this.elapsedSeconds / (this.spec.levelMinutes * 60));
    if (newLevel !== this.levelIndex && newLevel < this.levels.length) {
      this.levelIndex = newLevel;
      events.push({ t: 'level', level: newLevel + 1, blinds: this.level });
    }
    if (!this.announcedH4H && this.handForHand) {
      this.announcedH4H = true;
      events.push({ t: 'handForHand' });
    }
    this.log.push(...events);
    return events;
  }

  /** Simulate a whole round with no interactive hand (hero busted or sprinting). */
  simulateTick(heroAuto?: Profile): TEvent[] {
    const start = this.startTick();
    const events = [...start.events];
    events.push(...this.finishTick(start.hand, heroAuto));
    return events;
  }

  private processEliminations(): TEvent[] {
    const events: TEvent[] = [];
    const busted = [...this.players.values()].filter((p) => !p.busted && p.stack <= 0);
    if (busted.length === 0) return events;
    // Players eliminated in the same round: bigger starting stack finishes higher.
    busted.sort((a, b) => (this.startStacks.get(a.id) ?? 0) - (this.startStacks.get(b.id) ?? 0));
    const bubbleBefore = this.remaining > this.paidPlaces;
    for (const p of busted) {
      p.busted = true;
      p.place = this.remaining;
      p.prizeCents = this.prizeFor(p.place);
      this.remaining--;
      const table = this.tables.get(p.tableId);
      if (table) table.seats[p.seat] = null;
      events.push({ t: 'bust', id: p.id, name: p.name, place: p.place, prizeCents: p.prizeCents, tableId: p.tableId, isHero: p.isHero });
    }
    if (bubbleBefore && this.remaining <= this.paidPlaces && !this.burstBubble) {
      this.burstBubble = true;
      const bubbleBoy = busted.find((p) => p.place === this.paidPlaces + 1) ?? busted[busted.length - 1];
      events.push({ t: 'bubble', bubbleName: bubbleBoy.name });
    }
    if (this.remaining === 1) {
      const winner = this.alivePlayers()[0];
      winner.place = 1;
      winner.prizeCents = this.prizeFor(1);
      this.finished = true;
      events.push({ t: 'finished', winnerId: winner.id, winnerName: winner.name });
    }
    return events;
  }

  // ---------------------------------------------------------------- tables

  private count(t: TTable): number {
    let n = 0;
    for (const s of t.seats) if (s) n++;
    return n;
  }

  private seatPlayer(id: string, table: TTable): void {
    const p = this.players.get(id)!;
    const empty = table.seats.map((s, k) => (s === null ? k : -1)).filter((k) => k >= 0);
    const seat = this.rng.pick(empty);
    table.seats[seat] = id;
    p.tableId = table.id;
    p.seat = seat;
  }

  private movePlayer(id: string, to: TTable, events: TEvent[]): void {
    const p = this.players.get(id)!;
    const from = this.tables.get(p.tableId);
    if (from) from.seats[p.seat] = null;
    const fromId = p.tableId;
    this.seatPlayer(id, to);
    events.push({ t: 'moved', id, from: fromId, to: to.id, isHero: p.isHero });
  }

  /** Break tables that are no longer needed and even out table sizes. */
  balance(): TEvent[] {
    const events: TEvent[] = [];
    if (this.finished) return events;
    const alive = this.remaining;
    const target = Math.max(1, Math.ceil(alive / this.tableSize));
    while (this.tables.size > target) {
      const list = [...this.tables.values()].sort((a, b) => this.count(a) - this.count(b) || b.id - a.id);
      const victim = list[0];
      this.tables.delete(victim.id);
      events.push({ t: 'tableBroken', tableId: victim.id });
      for (const id of victim.seats) {
        if (!id) continue;
        const dest = [...this.tables.values()].sort((a, b) => this.count(a) - this.count(b))[0];
        const p = this.players.get(id)!;
        const fromId = p.tableId;
        this.seatPlayer(id, dest);
        events.push({ t: 'moved', id, from: fromId, to: dest.id, isHero: p.isHero });
      }
    }
    // Even out: no table more than one player bigger than another.
    for (let guard = 0; guard < 50; guard++) {
      const list = [...this.tables.values()].sort((a, b) => this.count(a) - this.count(b));
      const small = list[0];
      const big = list[list.length - 1];
      if (this.count(big) - this.count(small) <= 1) break;
      // Move the player due for the big blind next at the big table.
      const occupied = big.seats.map((s, k) => (s ? k : -1)).filter((k) => k >= 0);
      const afterButton = occupied.filter((k) => k > big.buttonSeat).concat(occupied.filter((k) => k <= big.buttonSeat));
      const mover = big.seats[afterButton[Math.min(1, afterButton.length - 1)]]!;
      this.movePlayer(mover, small, events);
    }
    if (!this.announcedFinal && this.tables.size === 1 && alive > 1) {
      this.announcedFinal = true;
      events.push({ t: 'finalTable' });
    }
    return events;
  }

  /** Scripted move (story beats), e.g. seating a rival at the hero's table. */
  moveToTable(id: string, tableId: number): TEvent[] {
    const events: TEvent[] = [];
    const p = this.players.get(id);
    const table = this.tables.get(tableId);
    if (!p || p.busted || !table || p.tableId === tableId) return events;
    if (this.count(table) >= this.tableSize) {
      // Table is full: swap a non-hero player over to the mover's old table.
      const old = this.tables.get(p.tableId)!;
      const swapId = table.seats.find((s) => s && s !== HERO_ID)!;
      const swap = this.players.get(swapId)!;
      old.seats[p.seat] = null;
      table.seats[swap.seat] = null;
      this.seatPlayer(swapId, old);
      this.seatPlayer(id, table);
      events.push({ t: 'moved', id: swapId, from: tableId, to: old.id, isHero: false });
      events.push({ t: 'moved', id, from: old.id, to: tableId, isHero: p.isHero });
      return events;
    }
    this.movePlayer(id, table, events);
    return events;
  }
}
