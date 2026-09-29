import { handPercentile } from '../equity';
import { PlayerAction } from '../hand';
import { Rng } from '../rng';
import { equityVsModels, heuristicEquity, modelOpponents } from './model';
import { Profile } from './profiles';
import { PlayerView, playersBehind, positionOf, preflopSummary, seatIndex } from './view';

export interface BotContext {
  profile: Profile;
  /** 0..1, raised by bad beats and big losses, decays over time. */
  tilt: number;
  /** 0..1, how much survival matters right now (bubble, pay jumps). */
  icmPressure: number;
  /** Lightweight mode for the off-screen tables of a big field. */
  fast: boolean;
  rng: Rng;
}

export interface BotDecision {
  action: PlayerAction;
  /** How long the player "thinks" before acting: the online timing tell. */
  thinkMs: number;
  /** The bot's own equity estimate, for debugging and replays. */
  equity: number;
}

const OPEN: Record<string, number> = {
  UTG: 0.12,
  'UTG+1': 0.135,
  MP: 0.155,
  LJ: 0.18,
  HJ: 0.22,
  CO: 0.28,
  BTN: 0.42,
  SB: 0.36,
  BB: 0.3,
};

/** Push/fold shoving range for an unopened pot (share of hands). */
export function pushRange(stackBB: number, behind: number): number {
  const r = (1.15 * 1.9) / (Math.max(1, stackBB) * (0.25 + 0.12 * behind));
  return Math.max(0.03, Math.min(1, r));
}

function tilted(p: Profile, tilt: number): Profile {
  if (tilt <= 0) return p;
  return {
    ...p,
    openWidth: p.openWidth * (1 + tilt * 0.8),
    aggression: Math.min(1, p.aggression + tilt * 0.25),
    bluff: Math.min(1, p.bluff + tilt * 0.3),
    stickiness: Math.min(0.95, p.stickiness + tilt * 0.25),
    pushFold: Math.max(0, p.pushFold - tilt * 0.4),
    callWidth: p.callWidth * (1 + tilt * 0.5),
  };
}

export function decide(view: PlayerView, ctx: BotContext): BotDecision {
  const p = tilted(ctx.profile, ctx.tilt);
  const pressure = Math.min(1, ctx.icmPressure * p.icmAware);
  const result = view.street === 'preflop' ? preflop(view, p, pressure, ctx) : postflop(view, p, pressure, ctx);
  return { ...result, action: legalize(view, result.action) };
}

// ------------------------------------------------------------------ helpers

type Choice = { action: PlayerAction; thinkMs: number; equity: number };

function legalize(view: PlayerView, a: PlayerAction): PlayerAction {
  if (a.type === 'raise') {
    if (!view.canRaise) return view.toCall > 0 ? { type: 'call' } : { type: 'check' };
    let to = Math.round(a.to);
    if (to < view.minRaiseTo) to = view.minRaiseTo;
    if (to > view.maxRaiseTo) to = view.maxRaiseTo;
    // Don't leave a sliver behind: commit fully when most of the stack goes in.
    if (to >= view.maxRaiseTo * 0.8) to = view.maxRaiseTo;
    if (to <= view.currentBet) return view.toCall > 0 ? { type: 'call' } : { type: 'check' };
    return { type: 'raise', to };
  }
  if (a.type === 'check' && !view.canCheck) return { type: 'fold' };
  if (a.type === 'fold' && view.canCheck) return { type: 'check' };
  return a;
}

function thinkTime(p: Profile, rng: Rng, strength: number, difficulty: number, snap = false): number {
  if (p.thinkBase === 0) return 0;
  const noise = rng.range(0.75, 1.25);
  if (snap) return rng.range(250, 800);
  let t: number;
  switch (p.timing) {
    case 'honest':
      // Hard decisions take longer; monsters are often snapped.
      t = p.thinkBase * 0.5 + difficulty * p.thinkVar + (strength > 0.85 && rng.chance(0.5) ? -p.thinkBase * 0.3 : 0);
      break;
    case 'reverse':
      // Acts out the opposite: tanks with strength, snaps with air.
      t = p.thinkBase * 0.4 + (strength > 0.7 ? p.thinkVar * 1.1 : strength < 0.3 ? 0 : p.thinkVar * 0.4);
      break;
    default:
      t = p.thinkBase * 0.6 + rng.next() * p.thinkVar;
  }
  return Math.max(300, t * noise);
}

function bbOf(view: PlayerView): number {
  return Math.max(1, view.bigBlind);
}

function allIn(view: PlayerView): PlayerAction {
  return { type: 'raise', to: view.maxRaiseTo };
}

function isInPosition(view: PlayerView): boolean {
  // Last active player to act postflop = closest to the button going backwards.
  const n = view.seats.length;
  for (let k = 0; k < n; k++) {
    const i = (view.buttonIdx - k + n) % n;
    const s = view.seats[i];
    if (!s.folded) return i === view.me;
  }
  return false;
}

// ------------------------------------------------------------------ preflop

function preflop(view: PlayerView, p: Profile, pressure: number, ctx: BotContext): Choice {
  const rng = ctx.rng;
  const bb = bbOf(view);
  const me = view.seats[view.me];
  const stackBB = (me.stack + me.streetBet) / bb;
  const pct = handPercentile(view.hole[0], view.hole[1]);
  const pos = positionOf(view, view.me);
  const sum = preflopSummary(view);
  const strength = 1 - pct;
  const fold = (snap = true): Choice => ({
    action: view.canCheck ? { type: 'check' } : { type: 'fold' },
    thinkMs: thinkTime(p, rng, strength, 0.1, snap),
    equity: strength,
  });
  const act = (action: PlayerAction, difficulty: number): Choice => ({
    action,
    thinkMs: thinkTime(p, rng, strength, difficulty),
    equity: strength,
  });

  if (sum.raises === 0) {
    if (view.me === view.bbIdx) {
      // Limped pot, big blind option.
      if (pct <= 0.12 * p.openWidth && rng.chance(p.aggression)) {
        return act({ type: 'raise', to: bb * (3 + sum.limpers) }, 0.3);
      }
      return act({ type: 'check' }, 0.1);
    }
    const behind = playersBehind(view, view.me);
    if (stackBB <= 14 && sum.limpers === 0) {
      const chart = pushRange(stackBB, behind) * (1 - 0.35 * pressure);
      const sloppy = 0.16 * p.openWidth;
      const push = p.pushFold * chart + (1 - p.pushFold) * sloppy;
      if (pct <= push) return act(allIn(view), Math.abs(pct - push) < 0.05 ? 0.7 : 0.3);
      if (stackBB > 9 && p.pushFold < 0.5 && pct <= 0.2 * p.openWidth) return act({ type: 'raise', to: bb * 2 }, 0.3);
      if (view.canCheck) return act({ type: 'check' }, 0.1);
      return fold(pct > push + 0.15);
    }
    const open = (OPEN[pos] ?? 0.2) * p.openWidth * (1 - 0.3 * pressure);
    if (sum.limpers > 0) {
      if (pct <= open * 0.55 && rng.chance(Math.min(1, p.aggression + 0.1))) {
        return act({ type: 'raise', to: bb * (3.5 + sum.limpers) * p.sizing }, 0.35);
      }
      if (pct <= open * 1.3 && rng.chance(0.5 + p.limpRate * 0.5)) return act({ type: 'call' }, 0.3);
      return fold(pct > open * 1.6);
    }
    if (pct <= open) {
      if (pct > 0.06 && rng.chance(p.limpRate)) return act({ type: 'call' }, 0.2);
      const size = stackBB > 40 ? (pos === 'BTN' || pos === 'CO' ? 2.2 : 2.4) : 2.05;
      return act({ type: 'raise', to: bb * size * p.sizing }, pct > open * 0.8 ? 0.5 : 0.2);
    }
    if (pct <= open * 1.6 && rng.chance(p.limpRate * 0.6)) return act({ type: 'call' }, 0.3);
    return fold(pct > open * 1.3);
  }

  // Facing one or more raises.
  const toCall = view.toCall;
  const raiserIdx = sum.lastRaiserSeat !== null ? seatIndex(view, sum.lastRaiserSeat) : -1;
  const raiser = raiserIdx >= 0 ? view.seats[raiserIdx] : null;
  const someoneAllIn = view.seats.some((s, i) => i !== view.me && !s.folded && s.allIn);
  const bigCall = toCall >= me.stack * 0.35;

  if (someoneAllIn || bigCall || (raiser && raiser.stack < bb * 2)) {
    // A stack-deciding spot: compare equity against the modelled ranges.
    const models = modelOpponents(view);
    const eq = equityVsModels(view.hole, [], models, ctx.fast ? 120 : 500, rng);
    const potOdds = toCall / (view.pot + toCall);
    const req = potOdds + pressure * 0.12 - p.stickiness * 0.08;
    const difficulty = Math.max(0, 1 - Math.abs(eq - req) * 5);
    if (eq >= req) {
      if (view.canRaise && eq > 0.62 && !someoneAllIn && rng.chance(p.aggression)) return { action: allIn(view), thinkMs: thinkTime(p, rng, eq, difficulty), equity: eq };
      return { action: { type: 'call' }, thinkMs: thinkTime(p, rng, eq, difficulty), equity: eq };
    }
    return { action: { type: 'fold' }, thinkMs: thinkTime(p, rng, eq, difficulty), equity: eq };
  }

  const ip = isInPosition(view);
  const raiserPos = raiserIdx >= 0 ? positionOf(view, raiserIdx) : 'UTG';
  const lateRaiser = raiserPos === 'BTN' || raiserPos === 'CO' || raiserPos === 'SB';
  const raiseBB = sum.lastRaiseTo / bb;

  if (sum.raises === 1) {
    // Short enough to re-shove?
    if (stackBB <= 22) {
      const reshove = (0.06 + (lateRaiser ? 0.07 : 0)) * (1 - 0.3 * pressure) * (0.6 + 0.4 * p.openWidth);
      if (pct <= reshove) return act(allIn(view), 0.5);
      if (view.me === view.bbIdx && raiseBB <= 2.2 && pct <= 0.3 * p.callWidth) return act({ type: 'call' }, 0.4);
      return fold(pct > 0.4);
    }
    const valueThree = p.threeBet * (ip ? 1 : 0.85) * (lateRaiser ? 1.4 : 1);
    if (pct <= valueThree) {
      const to = sum.lastRaiseTo * (ip ? 3 : 3.6) * p.sizing;
      return act(to > (me.stack + me.streetBet) * 0.4 ? allIn(view) : { type: 'raise', to }, 0.3);
    }
    if (pct > 0.12 && pct < 0.3 && lateRaiser && rng.chance(p.bluff * 0.25)) {
      return act({ type: 'raise', to: sum.lastRaiseTo * (ip ? 3 : 3.6) }, 0.6);
    }
    let call = view.me === view.bbIdx ? 0.38 : view.me === view.sbIdx ? 0.07 : ip ? 0.12 : 0.09;
    call *= p.callWidth * Math.min(1.2, 2.5 / Math.max(2, raiseBB));
    if (lateRaiser) call *= 1.25;
    if (stackBB < 30) call *= 0.6;
    if (pct <= call) return act({ type: 'call' }, pct > call * 0.8 ? 0.6 : 0.3);
    return fold(pct > call * 1.8);
  }

  // Facing a 3-bet or more.
  const fourBet = 0.022 * (1 + p.bluff);
  if (pct <= fourBet) {
    const to = sum.lastRaiseTo * 2.3;
    return act(stackBB < 60 || to > (me.stack + me.streetBet) * 0.4 ? allIn(view) : { type: 'raise', to }, 0.4);
  }
  const cheap = toCall < me.stack * 0.12;
  if (pct <= 0.055 * p.callWidth * (cheap ? 1.5 : 1)) return act({ type: 'call' }, 0.6);
  return fold(false);
}

// ------------------------------------------------------------------ postflop

function postflop(view: PlayerView, p: Profile, pressure: number, ctx: BotContext): Choice {
  const rng = ctx.rng;
  const me = view.seats[view.me];
  const models = modelOpponents(view);
  const opps = models.length;
  const eq = ctx.fast ? heuristicEquity(view.hole, view.board, opps) : equityVsModels(view.hole, view.board, models, 260, rng);
  const pot = view.pot;
  const toCall = view.toCall;
  const ip = isInPosition(view);
  const sum = preflopSummary(view);
  const pfa = sum.lastRaiserSeat === me.seat;
  const bb = bbOf(view);

  const betTo = (fraction: number): PlayerAction => {
    const size = Math.max(bb, pot * fraction * p.sizing);
    return { type: 'raise', to: view.currentBet + size };
  };
  const out = (action: PlayerAction, threshold: number): Choice => {
    const difficulty = Math.max(0, 1 - Math.abs(eq - threshold) * 4);
    return { action, thinkMs: thinkTime(p, rng, eq, difficulty), equity: eq };
  };

  if (toCall === 0) {
    const valueTh = 0.6 + 0.07 * (opps - 1);
    if (eq >= valueTh) {
      const slowplay = eq > 0.9 && rng.chance(0.25 * (1 - p.aggression));
      if (!slowplay && rng.chance(Math.min(1, p.aggression + 0.1))) {
        const big = view.street === 'river' && eq > 0.85 ? 0.75 + 0.25 * rng.next() : 0.5 + 0.3 * rng.next();
        return out(betTo(big), valueTh);
      }
      return out({ type: 'check' }, valueTh);
    }
    if (pfa && view.street === 'flop' && rng.chance(p.cbet * Math.pow(0.55, opps - 1))) {
      return out(betTo(0.33 + 0.2 * rng.next()), valueTh);
    }
    if (eq >= 0.3 && view.street !== 'river' && rng.chance(p.aggression * 0.3)) {
      return out(betTo(0.45 + 0.25 * rng.next()), valueTh);
    }
    if (eq < 0.3 && opps === 1 && rng.chance(p.bluff * (ip ? 1 : 0.6) * (view.street === 'river' ? 0.8 : 1))) {
      return out(betTo(0.35 + 0.3 * rng.next()), valueTh);
    }
    return out({ type: 'check' }, valueTh);
  }

  const potOdds = toCall / (pot + toCall);
  const commitment = toCall / Math.max(1, me.stack);
  const req = potOdds * (1 - p.stickiness * 0.45) + pressure * 0.1 * (commitment > 0.3 ? 1 : 0.3);
  const raiseTh = 0.78 + 0.05 * (opps - 1);
  if (eq >= raiseTh && view.canRaise && rng.chance(p.aggression)) {
    const to = view.currentBet * 2.6 + (pot - view.currentBet) * 0.25;
    return out(to > view.maxRaiseTo * 0.55 ? allIn(view) : { type: 'raise', to }, raiseTh);
  }
  if (eq >= req) return out({ type: 'call' }, req);
  if (view.canRaise && opps === 1 && eq > 0.22 && view.street !== 'river' && rng.chance(p.bluff * 0.18)) {
    return out({ type: 'raise', to: view.currentBet * 2.7 }, req);
  }
  return out({ type: 'fold' }, req);
}
