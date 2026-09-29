/** Blind structures, payout tables and ICM. */

export interface Level {
  sb: number;
  bb: number;
  ante: number; // big-blind ante
}

/** Modern online structure: big-blind ante from level 1. Starting stack 10,000 = 100 BB. */
export const STANDARD_LEVELS: Level[] = [
  [50, 100], [60, 120], [80, 160], [100, 200], [125, 250], [150, 300], [200, 400], [250, 500],
  [300, 600], [400, 800], [500, 1000], [600, 1200], [800, 1600], [1000, 2000], [1250, 2500],
  [1500, 3000], [2000, 4000], [2500, 5000], [3000, 6000], [4000, 8000], [5000, 10000],
  [6000, 12000], [8000, 16000], [10000, 20000], [12500, 25000], [15000, 30000], [20000, 40000],
  [25000, 50000], [30000, 60000], [40000, 80000], [50000, 100000], [60000, 120000],
  [80000, 160000], [100000, 200000], [125000, 250000], [150000, 300000], [200000, 400000],
  [250000, 500000], [300000, 600000], [400000, 800000], [500000, 1000000],
].map(([sb, bb]) => ({ sb, bb, ante: bb }));

/**
 * Payout table. About 15% of the field is paid, with prizes following a
 * power law (1st takes roughly 16-20% of a 1,000-player pool) and a floor
 * so the min-cash is at least `minCash`. Amounts are in cents and sum
 * exactly to the prize pool.
 */
export function payoutTable(poolCents: number, entrants: number, minCashCents: number, paidShare = 0.15): number[] {
  const paid = Math.max(1, Math.min(entrants - 1, Math.round(entrants * paidShare)));
  if (entrants <= 3) return [poolCents];
  const weights: number[] = [];
  for (let i = 1; i <= paid; i++) weights.push(1 / Math.pow(i, paid > 60 ? 1.0 : paid > 12 ? 0.95 : 0.85));
  const wSum = weights.reduce((a, b) => a + b, 0);
  let cents = weights.map((w) => (w / wSum) * poolCents);
  // Enforce the min-cash floor, taking the difference proportionally from the top.
  const floor = Math.min(minCashCents, poolCents / paid);
  let deficit = 0;
  cents = cents.map((c) => {
    if (c < floor) {
      deficit += floor - c;
      return floor;
    }
    return c;
  });
  if (deficit > 0) {
    const above = cents.map((c) => Math.max(0, c - floor));
    const aboveSum = above.reduce((a, b) => a + b, 0);
    cents = cents.map((c, i) => c - (aboveSum > 0 ? (above[i] / aboveSum) * deficit : 0));
  }
  // Payout bands: places 10+ share the band average like real sites.
  const bands = payoutBands(paid);
  for (const [a, b] of bands) {
    if (b <= a) continue;
    let sum = 0;
    for (let i = a; i <= b; i++) sum += cents[i - 1];
    for (let i = a; i <= b; i++) cents[i - 1] = sum / (b - a + 1);
  }
  const rounded = cents.map((c) => Math.floor(c));
  let rem = poolCents - rounded.reduce((a, b) => a + b, 0);
  for (let i = 0; rem > 0; i = (i + 1) % rounded.length, rem--) rounded[i]++;
  return rounded;
}

/** Inclusive place ranges that share a payout, e.g. [10, 12], [13, 15]. */
export function payoutBands(paid: number): [number, number][] {
  const bands: [number, number][] = [];
  let place = 1;
  while (place <= paid) {
    let size: number;
    if (place <= 9) size = 1;
    else if (place <= 18) size = 3;
    else if (place <= 72) size = 9;
    else if (place <= 216) size = 18;
    else size = 36;
    const end = Math.min(paid, place + size - 1);
    bands.push([place, end]);
    place = end + 1;
  }
  return bands;
}

/**
 * Independent Chip Model (Malmuth-Harville): each player's expected share of
 * the remaining prizes given the chip stacks. Exact when the number of
 * finishing orders to enumerate is small (final tables), proportional otherwise.
 */
export function icm(stacks: readonly number[], prizes: readonly number[]): number[] {
  const n = stacks.length;
  const total = stacks.reduce((a, b) => a + b, 0);
  const result = new Array(n).fill(0);
  if (n === 0 || total === 0) return result;
  const places = Math.min(prizes.length, n);
  let orders = 1;
  for (let k = 0; k < places; k++) orders *= n - k;
  if (orders > 2_000_000) {
    // Beyond exact range: proportional approximation.
    const prizeSum = prizes.reduce((a, b) => a + b, 0);
    return stacks.map((s) => (s / total) * prizeSum);
  }
  // Expand every finishing order: P(i finishes next) = stack_i / chips left.
  const recurse = (mask: number, depth: number, prob: number, remaining: number) => {
    if (depth >= places) return;
    for (let i = 0; i < n; i++) {
      if (mask & (1 << i)) continue;
      const p = (stacks[i] / remaining) * prob;
      if (p === 0) continue;
      result[i] += p * prizes[depth];
      recurse(mask | (1 << i), depth + 1, p, remaining - stacks[i]);
    }
  };
  recurse(0, 0, 1, total);
  return result;
}
