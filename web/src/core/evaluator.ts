import { Card, RANK_NAMES, RANK_PLURALS } from './cards';

/**
 * Fast 5- to 7-card hand evaluator.
 *
 * Returns an integer score where a higher value is a stronger hand:
 *   category << 20 | r1 << 16 | r2 << 12 | r3 << 8 | r4 << 4 | r5
 * Category 0 = high card .. 8 = straight flush. The r-slots hold the ranks
 * that break ties, most significant first.
 */
export enum Category {
  HighCard = 0,
  Pair = 1,
  TwoPair = 2,
  Trips = 3,
  Straight = 4,
  Flush = 5,
  FullHouse = 6,
  Quads = 7,
  StraightFlush = 8,
}

export const CATEGORY_NAMES = [
  'High Card',
  'Pair',
  'Two Pair',
  'Three of a Kind',
  'Straight',
  'Flush',
  'Full House',
  'Four of a Kind',
  'Straight Flush',
];

const STRAIGHT_HIGH = new Int8Array(8192);
const POPCOUNT = new Uint8Array(8192);

(function init() {
  for (let m = 0; m < 8192; m++) {
    let p = 0;
    for (let b = 0; b < 13; b++) if (m & (1 << b)) p++;
    POPCOUNT[m] = p;
    let high = -1;
    for (let top = 12; top >= 4; top--) {
      const run = 0b11111 << (top - 4);
      if ((m & run) === run) {
        high = top;
        break;
      }
    }
    // Wheel: A-2-3-4-5 plays as a five-high straight.
    if (high < 0 && (m & 0b1000000001111) === 0b1000000001111) high = 3;
    STRAIGHT_HIGH[m] = high;
  }
})();

const counts = new Uint8Array(13);
const suitMasks = new Int32Array(4);

function topRanks(mask: number, n: number): number {
  let out = 0;
  let taken = 0;
  for (let r = 12; r >= 0 && taken < n; r--) {
    if (mask & (1 << r)) {
      out = (out << 4) | r;
      taken++;
    }
  }
  // Left-align so slot positions are comparable across categories.
  return out << (4 * (5 - taken));
}

export function evaluate(cards: ArrayLike<Card>, n: number = cards.length): number {
  counts.fill(0);
  suitMasks[0] = suitMasks[1] = suitMasks[2] = suitMasks[3] = 0;
  let rankMask = 0;
  for (let i = 0; i < n; i++) {
    const c = cards[i];
    const r = c >> 2;
    counts[r]++;
    suitMasks[c & 3] |= 1 << r;
    rankMask |= 1 << r;
  }

  // With at most 7 cards a flush excludes quads and full houses, so it can
  // be returned immediately unless it is also a straight flush.
  for (let s = 0; s < 4; s++) {
    const sm = suitMasks[s];
    if (POPCOUNT[sm] >= 5) {
      const sf = STRAIGHT_HIGH[sm];
      if (sf >= 0) return (Category.StraightFlush << 20) | (sf << 16);
      return (Category.Flush << 20) | topRanks(sm, 5);
    }
  }

  let quad = -1;
  let trip1 = -1;
  let trip2 = -1;
  let pair1 = -1;
  let pair2 = -1;
  for (let r = 12; r >= 0; r--) {
    const k = counts[r];
    if (k === 4) quad = r;
    else if (k === 3) {
      if (trip1 < 0) trip1 = r;
      else if (trip2 < 0) trip2 = r;
    } else if (k === 2) {
      if (pair1 < 0) pair1 = r;
      else if (pair2 < 0) pair2 = r;
    }
  }

  if (quad >= 0) {
    const kick = topRanks(rankMask & ~(1 << quad), 1) >> 16;
    return (Category.Quads << 20) | (quad << 16) | (kick << 12);
  }
  if (trip1 >= 0 && (trip2 >= 0 || pair1 >= 0)) {
    const pairRank = Math.max(trip2, pair1);
    return (Category.FullHouse << 20) | (trip1 << 16) | (pairRank << 12);
  }
  const st = STRAIGHT_HIGH[rankMask];
  if (st >= 0) return (Category.Straight << 20) | (st << 16);
  if (trip1 >= 0) {
    const kick = topRanks(rankMask & ~(1 << trip1), 2) >> 12;
    return (Category.Trips << 20) | (trip1 << 16) | (kick << 8);
  }
  if (pair1 >= 0 && pair2 >= 0) {
    const kick = topRanks(rankMask & ~(1 << pair1) & ~(1 << pair2), 1) >> 16;
    return (Category.TwoPair << 20) | (pair1 << 16) | (pair2 << 12) | (kick << 8);
  }
  if (pair1 >= 0) {
    const kick = topRanks(rankMask & ~(1 << pair1), 3) >> 8;
    return (Category.Pair << 20) | (pair1 << 16) | (kick << 4);
  }
  return (Category.HighCard << 20) | topRanks(rankMask, 5);
}

export const categoryOf = (score: number): number => score >> 20;

/** Plain-English description, e.g. "Two Pair, Kings and Nines". */
export function describe(score: number): string {
  const cat = score >> 20;
  const r1 = (score >> 16) & 15;
  const r2 = (score >> 12) & 15;
  switch (cat) {
    case Category.StraightFlush:
      return r1 === 12 ? 'Royal Flush' : `Straight Flush, ${RANK_NAMES[r1]} high`;
    case Category.Quads:
      return `Four of a Kind, ${RANK_PLURALS[r1]}`;
    case Category.FullHouse:
      return `Full House, ${RANK_PLURALS[r1]} full of ${RANK_PLURALS[r2]}`;
    case Category.Flush:
      return `Flush, ${RANK_NAMES[r1]} high`;
    case Category.Straight:
      return `Straight, ${RANK_NAMES[r1]} high`;
    case Category.Trips:
      return `Three of a Kind, ${RANK_PLURALS[r1]}`;
    case Category.TwoPair:
      return `Two Pair, ${RANK_PLURALS[r1]} and ${RANK_PLURALS[r2]}`;
    case Category.Pair:
      return `Pair of ${RANK_PLURALS[r1]}`;
    default:
      return `${RANK_NAMES[r1]} High`;
  }
}

/** Evaluate hole cards plus board (board may have 3-5 cards). */
export function evaluateHand(hole: readonly Card[], board: readonly Card[]): number {
  const all = scratch7;
  let n = 0;
  for (const c of hole) all[n++] = c;
  for (const c of board) all[n++] = c;
  return evaluate(all, n);
}

const scratch7 = new Int32Array(7);
