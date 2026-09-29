import { Card, handClass, handClassCombos } from './cards';
import { evaluate } from './evaluator';
import { PREFLOP_ORDER } from './preflopRanking';
import { Rng } from './rng';

/**
 * Preflop percentile of each hand class: the share of all 1326 combos that
 * are at least as strong. AA = 0.0045, 32o = 1.0.
 */
export const CLASS_PERCENTILE: Float64Array = (() => {
  const pct = new Float64Array(169);
  let cum = 0;
  for (const cls of PREFLOP_ORDER) {
    cum += handClassCombos(cls);
    pct[cls] = cum / 1326;
  }
  return pct;
})();

export function handPercentile(a: Card, b: Card): number {
  return CLASS_PERCENTILE[handClass(a, b)];
}

/**
 * A range expressed as a band of the preflop ranking: hands whose percentile
 * lies in (min, max]. {min: 0, max: 1} is any two cards; {min: 0, max: 0.1}
 * is the top 10%. A band with min > 0 models "capped" ranges, e.g. a player
 * who only called preflop and would have raised the very best hands.
 */
export interface RangeBand {
  min: number;
  max: number;
}

export const ANY_TWO: RangeBand = { min: 0, max: 1 };

function inBand(a: Card, b: Card, band: RangeBand): boolean {
  const p = CLASS_PERCENTILE[handClass(a, b)];
  return p > band.min && p <= band.max;
}

/**
 * Monte Carlo equity of `hero` against opponents holding hands from the given
 * ranges, with the board completed at random. Ties split the pot.
 */
export function equityVsRanges(
  hero: readonly Card[],
  board: readonly Card[],
  ranges: readonly RangeBand[],
  iterations: number,
  rng: Rng,
): number {
  if (ranges.length === 0) return 1;
  const dead = new Uint8Array(52);
  for (const c of hero) dead[c] = 1;
  for (const c of board) dead[c] = 1;
  const live: Card[] = [];
  for (let c = 0; c < 52; c++) if (!dead[c]) live.push(c);

  const cards = new Int32Array(7);
  const oppHands = new Int32Array(ranges.length * 2);
  const boardNeed = 5 - board.length;
  const used = new Uint8Array(52);
  let won = 0;

  for (let it = 0; it < iterations; it++) {
    used.fill(0);
    // Deal opponents from their ranges (rejection sampling with a fallback).
    for (let o = 0; o < ranges.length; o++) {
      const band = ranges[o];
      let a = 0;
      let b = 0;
      for (let tries = 0; tries < 60; tries++) {
        a = live[rng.int(live.length)];
        b = live[rng.int(live.length)];
        if (a === b || used[a] || used[b]) continue;
        if (tries > 45 || inBand(a, b, band)) break;
      }
      if (a === b || used[a] || used[b]) {
        // Fallback: any two free cards.
        do a = live[rng.int(live.length)]; while (used[a]);
        do b = live[rng.int(live.length)]; while (used[b] || b === a);
      }
      used[a] = 1;
      used[b] = 1;
      oppHands[o * 2] = a;
      oppHands[o * 2 + 1] = b;
    }
    // Complete the board.
    let n = 2;
    for (const c of board) cards[n++] = c;
    for (let k = 0; k < boardNeed; k++) {
      let c: number;
      do c = live[rng.int(live.length)]; while (used[c]);
      used[c] = 1;
      cards[n++] = c;
    }
    cards[0] = hero[0];
    cards[1] = hero[1];
    const hs = evaluate(cards, 7);
    let ties = 1;
    let lost = false;
    for (let o = 0; o < ranges.length; o++) {
      cards[0] = oppHands[o * 2];
      cards[1] = oppHands[o * 2 + 1];
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
 * Equity for every player when all hands are known (the broadcast-style
 * "win %" bars). Exact enumeration on the flop and turn, Monte Carlo preflop.
 * Returns the share of the pot each player wins on average.
 */
export function equityKnownHands(
  hands: readonly (readonly Card[])[],
  board: readonly Card[],
  rng: Rng,
  preflopSamples = 12000,
): number[] {
  const dead = new Uint8Array(52);
  for (const h of hands) for (const c of h) dead[c] = 1;
  for (const c of board) dead[c] = 1;
  const live: Card[] = [];
  for (let c = 0; c < 52; c++) if (!dead[c]) live.push(c);

  const shares = new Array(hands.length).fill(0);
  const cards = new Int32Array(7);
  const scores = new Array(hands.length).fill(0);
  let total = 0;

  const settle = (full: number[]) => {
    let best = -1;
    let count = 0;
    for (let h = 0; h < hands.length; h++) {
      cards[0] = hands[h][0];
      cards[1] = hands[h][1];
      for (let i = 0; i < 5; i++) cards[2 + i] = full[i];
      const s = evaluate(cards, 7);
      scores[h] = s;
      if (s > best) {
        best = s;
        count = 1;
      } else if (s === best) count++;
    }
    for (let h = 0; h < hands.length; h++) if (scores[h] === best) shares[h] += 1 / count;
    total++;
  };

  const need = 5 - board.length;
  if (need === 0) {
    settle([...board]);
  } else if (need <= 2) {
    const full = [...board, 0, 0].slice(0, 5);
    if (need === 1) {
      for (const c of live) {
        full[4] = c;
        settle(full);
      }
    } else {
      for (let i = 0; i < live.length; i++)
        for (let j = i + 1; j < live.length; j++) {
          full[3] = live[i];
          full[4] = live[j];
          settle(full);
        }
    }
  } else {
    const deck = live.slice();
    const full = new Array(5).fill(0);
    for (let s = 0; s < preflopSamples; s++) {
      for (let i = 0; i < need; i++) {
        const j = i + rng.int(deck.length - i);
        const t = deck[i];
        deck[i] = deck[j];
        deck[j] = t;
      }
      for (let i = 0; i < board.length; i++) full[i] = board[i];
      for (let i = 0; i < need; i++) full[board.length + i] = deck[i];
      settle(full);
    }
  }
  return shares.map((s) => s / total);
}
