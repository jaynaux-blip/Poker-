import { Card } from '../cards';
import { detPow } from '../detmath';
import { CLASS_PERCENTILE, RangeBand } from '../equity';
import { handClass } from '../cards';
import { Category, evaluate, straightHigh } from '../evaluator';
import { Rng } from '../rng';
import { PlayerView, positionOf, seatIndex } from './view';

/**
 * What one player believes about an opponent's holding, built only from
 * public actions: a preflop range band plus how much postflop aggression or
 * calling the opponent has shown.
 */
export interface OpponentModel {
  seat: number;
  band: RangeBand;
  aggr: number; // postflop bets and raises
  calls: number; // postflop calls
}

const OPEN_BAND: Record<string, number> = {
  UTG: 0.14,
  'UTG+1': 0.16,
  MP: 0.18,
  LJ: 0.21,
  HJ: 0.25,
  CO: 0.32,
  BTN: 0.45,
  SB: 0.4,
  BB: 0.3,
};

export function modelOpponents(view: PlayerView): OpponentModel[] {
  const models: OpponentModel[] = [];
  for (let i = 0; i < view.seats.length; i++) {
    if (i === view.me) continue;
    const s = view.seats[i];
    if (s.folded) continue;
    models.push(modelSeat(view, s.seat));
  }
  return models;
}

export function modelSeat(view: PlayerView, seat: number): OpponentModel {
  const idx = seatIndex(view, seat);
  let band: RangeBand = { min: 0, max: 1 };
  let raisesSeen = 0;
  let aggr = 0;
  let calls = 0;
  const bbStack = Math.max(1, view.bigBlind);

  for (const h of view.history) {
    if (h.street === 'preflop') {
      const isRaise = h.type === 'raise' || h.type === 'bet';
      if (h.seat === seat) {
        if (isRaise) {
          if (raisesSeen === 0) {
            // Open-raise. Short-stack shoves are wider than opens.
            const open = OPEN_BAND[positionOf(view, idx)] ?? 0.25;
            const shoveWide = h.allIn && h.to / bbStack <= 15 ? 0.1 : 0;
            band = { min: 0, max: open + shoveWide };
          } else if (raisesSeen === 1) band = { min: 0, max: h.allIn ? 0.12 : 0.07 };
          else band = { min: 0, max: h.allIn ? 0.06 : 0.03 };
        } else if (h.type === 'call') {
          if (raisesSeen === 0) {
            band = idx === view.sbIdx ? { min: 0.03, max: 0.8 } : { min: 0.04, max: 0.65 };
          } else if (raisesSeen === 1) {
            band = idx === view.bbIdx ? { min: 0.03, max: 0.55 } : { min: 0.015, max: 0.35 };
          } else band = { min: 0.005, max: 0.12 };
        }
      }
      if (isRaise) raisesSeen++;
    } else if (h.seat === seat) {
      if (h.type === 'bet' || h.type === 'raise') aggr++;
      else if (h.type === 'call') calls++;
    }
  }
  return { seat, band, aggr, calls };
}

// ------------------------------------------------------------------ board reading

/** 0 = nothing, 1 = one pair using a hole card, 2 = two pair or better using a hole card. */
export function madeLevel(a: Card, b: Card, board: readonly Card[], boardCat: number): number {
  scratch[0] = a;
  scratch[1] = b;
  for (let i = 0; i < board.length; i++) scratch[2 + i] = board[i];
  const cat = evaluate(scratch, 2 + board.length) >> 20;
  if (cat <= boardCat) return 0;
  if (cat >= Category.Trips) return 2;
  if (cat === Category.TwoPair && boardCat === Category.HighCard) return 2;
  return 1;
}

/** True for a flush draw or open-ended straight draw (flop and turn only). */
export function hasDraw(a: Card, b: Card, board: readonly Card[]): boolean {
  if (board.length >= 5) return false;
  const suitCount = [0, 0, 0, 0];
  let mask = 0;
  suitCount[a & 3]++;
  suitCount[b & 3]++;
  mask |= (1 << (a >> 2)) | (1 << (b >> 2));
  for (const c of board) {
    suitCount[c & 3]++;
    mask |= 1 << (c >> 2);
  }
  for (let s = 0; s < 4; s++) if (suitCount[s] === 4 && ((a & 3) === s || (b & 3) === s)) return true;
  if (straightHigh(mask) >= 0) return false;
  let outs = 0;
  for (let r = 0; r < 13; r++) if (!(mask & (1 << r)) && straightHigh(mask | (1 << r)) >= 0) outs++;
  return outs >= 2;
}

export function boardCategory(board: readonly Card[]): number {
  if (board.length === 0) return 0;
  for (let i = 0; i < board.length; i++) scratch[i] = board[i];
  return evaluate(scratch, board.length) >> 20;
}

const scratch = new Int32Array(7);

// ------------------------------------------------------------------ equity

function accepts(m: OpponentModel, a: Card, b: Card, board: readonly Card[], boardCat: number, r: number): boolean {
  const p = CLASS_PERCENTILE[handClass(a, b)];
  if (p <= m.band.min || p > m.band.max) return false;
  if (board.length === 0 || (m.aggr === 0 && m.calls === 0)) return true;
  const made = madeLevel(a, b, board, boardCat);
  const draw = made === 0 && hasDraw(a, b, board);
  if (m.aggr >= 2) return made === 2 || (made === 1 && r < 0.55) || (draw && r < 0.35) || r < 0.08;
  if (m.aggr === 1) return made >= 1 || (draw && r < 0.8) || r < 0.22;
  return made >= 1 || draw || r < 0.25;
}

/**
 * Monte Carlo equity against modelled opponents. Opponent hands are drawn
 * from their preflop band and filtered by what their postflop actions imply.
 */
export function equityVsModels(
  hole: readonly Card[],
  board: readonly Card[],
  models: readonly OpponentModel[],
  iterations: number,
  rng: Rng,
): number {
  if (models.length === 0) return 1;
  const used = new Uint8Array(52);
  const live: Card[] = [];
  const dead = new Uint8Array(52);
  for (const c of hole) dead[c] = 1;
  for (const c of board) dead[c] = 1;
  for (let c = 0; c < 52; c++) if (!dead[c]) live.push(c);
  const boardCat = boardCategory(board);
  const cards = new Int32Array(7);
  const opp = new Int32Array(models.length * 2);
  let won = 0;

  for (let it = 0; it < iterations; it++) {
    used.fill(0);
    for (let o = 0; o < models.length; o++) {
      let a = -1;
      let b = -1;
      for (let tries = 0; tries < 80; tries++) {
        const x = live[rng.int(live.length)];
        const y = live[rng.int(live.length)];
        if (x === y || used[x] || used[y]) continue;
        a = x;
        b = y;
        if (accepts(models[o], x, y, board, boardCat, rng.next())) break;
      }
      if (a < 0) {
        do a = live[rng.int(live.length)]; while (used[a]);
        do b = live[rng.int(live.length)]; while (used[b] || b === a);
      }
      used[a] = 1;
      used[b] = 1;
      opp[o * 2] = a;
      opp[o * 2 + 1] = b;
    }
    let n = 2;
    for (const c of board) cards[n++] = c;
    while (n < 7) {
      const c = live[rng.int(live.length)];
      if (used[c]) continue;
      used[c] = 1;
      cards[n++] = c;
    }
    cards[0] = hole[0];
    cards[1] = hole[1];
    const hs = evaluate(cards, 7);
    let ties = 1;
    let lost = false;
    for (let o = 0; o < models.length; o++) {
      cards[0] = opp[o * 2];
      cards[1] = opp[o * 2 + 1];
      const os = evaluate(cards, 7);
      if (os > hs) {
        lost = true;
        break;
      }
      if (os === hs) ties++;
    }
    if (!lost) won += 1 / ties;
  }
  return won / iterations;
}

/**
 * Cheap equity estimate from made-hand strength and draws, used by the
 * lightweight bots that play the rest of the tournament field.
 */
export function heuristicEquity(hole: readonly Card[], board: readonly Card[], opponents: number): number {
  const boardCat = boardCategory(board);
  for (let i = 0; i < board.length; i++) scratch[2 + i] = board[i];
  scratch[0] = hole[0];
  scratch[1] = hole[1];
  const score = evaluate(scratch, 2 + board.length);
  const cat = score >> 20;
  let eq: number;
  if (cat <= boardCat && cat <= Category.Pair) {
    eq = 0.12 + 0.02 * Math.max(hole[0] >> 2, hole[1] >> 2) / 12;
  } else if (cat === Category.Pair) {
    const pairRank = (score >> 16) & 15;
    let higherBoard = 0;
    for (const c of board) if (c >> 2 > pairRank) higherBoard++;
    eq = higherBoard === 0 ? 0.62 + 0.012 * pairRank : higherBoard === 1 ? 0.45 : 0.32;
  } else if (cat === Category.TwoPair) eq = boardCat === Category.Pair ? 0.55 : 0.76;
  else if (cat === Category.Trips) eq = 0.82;
  else if (cat === Category.Straight) eq = 0.86;
  else if (cat === Category.Flush) eq = 0.9;
  else eq = 0.97;
  if (board.length < 5 && hasDraw(hole[0], hole[1], board)) eq += board.length === 3 ? 0.18 : 0.1;
  eq = Math.min(0.99, eq);
  return detPow(eq, 1 + 0.6 * (opponents - 1));
}
