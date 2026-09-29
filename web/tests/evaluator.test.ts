import { describe, expect, it } from 'vitest';
import { Card, parseCards } from '../src/core/cards';
import { Category, categoryOf, describe as describeHand, evaluate } from '../src/core/evaluator';
import { equityKnownHands, equityVsRanges, ANY_TWO } from '../src/core/equity';
import { Rng } from '../src/core/rng';

const ev = (s: string) => evaluate(parseCards(s));

/**
 * Reference evaluator: brute force over every 5-card subset using a simple,
 * obviously-correct scorer. Used to cross-check the fast evaluator.
 */
function slowScore5(cards: Card[]): number[] {
  const ranks = cards.map((c) => c >> 2).sort((a, b) => b - a);
  const suits = cards.map((c) => c & 3);
  const flush = suits.every((s) => s === suits[0]);
  const uniq = [...new Set(ranks)];
  let straightHigh = -1;
  if (uniq.length === 5) {
    if (ranks[0] - ranks[4] === 4) straightHigh = ranks[0];
    else if (ranks.join() === '12,3,2,1,0') straightHigh = 3;
  }
  const counts = new Map<number, number>();
  for (const r of ranks) counts.set(r, (counts.get(r) ?? 0) + 1);
  const groups = [...counts.entries()].sort((a, b) => b[1] - a[1] || b[0] - a[0]);
  const shape = groups.map((g) => g[1]).join('');
  const byGroup = groups.map((g) => g[0]);
  if (flush && straightHigh >= 0) return [8, straightHigh];
  if (shape === '41') return [7, ...byGroup];
  if (shape === '32') return [6, ...byGroup];
  if (flush) return [5, ...ranks];
  if (straightHigh >= 0) return [4, straightHigh];
  if (shape === '311') return [3, ...byGroup];
  if (shape === '221') return [2, ...byGroup];
  if (shape === '2111') return [1, ...byGroup];
  return [0, ...ranks];
}

function cmp(a: number[], b: number[]): number {
  for (let i = 0; i < Math.max(a.length, b.length); i++) {
    const d = (a[i] ?? -1) - (b[i] ?? -1);
    if (d) return d;
  }
  return 0;
}

function slowBest(cards: Card[]): number[] {
  let best: number[] = [-1];
  const n = cards.length;
  for (let a = 0; a < n; a++)
    for (let b = a + 1; b < n; b++)
      for (let c = b + 1; c < n; c++)
        for (let d = c + 1; d < n; d++)
          for (let e = d + 1; e < n; e++) {
            const s = slowScore5([cards[a], cards[b], cards[c], cards[d], cards[e]]);
            if (cmp(s, best) > 0) best = s;
          }
  return best;
}

describe('hand evaluator', () => {
  it('ranks categories correctly', () => {
    expect(categoryOf(ev('AsKsQsJsTs'))).toBe(Category.StraightFlush);
    expect(categoryOf(ev('9h9d9c9s2d'))).toBe(Category.Quads);
    expect(categoryOf(ev('KhKdKc2s2d'))).toBe(Category.FullHouse);
    expect(categoryOf(ev('Ah9h7h4h2h'))).toBe(Category.Flush);
    expect(categoryOf(ev('5c4d3h2sAd'))).toBe(Category.Straight);
    expect(categoryOf(ev('7c7d7h2sAd'))).toBe(Category.Trips);
    expect(categoryOf(ev('7c7dAhAs3d'))).toBe(Category.TwoPair);
    expect(categoryOf(ev('7c7dAhKs3d'))).toBe(Category.Pair);
    expect(categoryOf(ev('7c9dAhKs3d'))).toBe(Category.HighCard);
  });

  it('handles the wheel and steel wheel', () => {
    expect(ev('5c4d3h2sAd')).toBeLessThan(ev('6c5d4h3s2d'));
    expect(categoryOf(ev('5s4s3s2sAs'))).toBe(Category.StraightFlush);
    expect(ev('5s4s3s2sAs')).toBeLessThan(ev('6s5s4s3s2s'));
  });

  it('uses kickers', () => {
    expect(ev('AhAdKc7s2d')).toBeGreaterThan(ev('AhAdQc7s2d'));
    expect(ev('AhAdKc7s3d')).toBeGreaterThan(ev('AcAsKd7h2c'));
    expect(ev('AhAdKc7s2d')).toBe(ev('AcAsKd7h2c'));
  });

  it('picks the best five of seven', () => {
    // Two pair plus a third pair: kicker is the third pair's rank.
    const s = ev('KhKd9c9s4d4cAh');
    expect(categoryOf(s)).toBe(Category.TwoPair);
    expect(s).toBeGreaterThan(ev('KhKd9c9s4d4cQh'));
    // Two trips make a full house.
    expect(categoryOf(ev('8h8d8c3s3d3cAh'))).toBe(Category.FullHouse);
    expect(describeHand(ev('8h8d8c3s3d3cAh'))).toBe('Full House, Eights full of Threes');
  });

  it('matches a brute-force reference on 20,000 random 7-card hands', () => {
    const rng = new Rng('evaluator-crosscheck');
    const deck = Array.from({ length: 52 }, (_, i) => i);
    for (let t = 0; t < 20000; t++) {
      rng.shuffle(deck);
      const a = deck.slice(0, 7);
      const b = deck.slice(7, 14);
      const fast = Math.sign(evaluate(a) - evaluate(b));
      const slow = Math.sign(cmp(slowBest(a), slowBest(b)));
      if (fast !== slow) throw new Error(`Mismatch: ${a} vs ${b}`);
    }
  });
});

describe('equity', () => {
  it('AA is about 85% vs a random hand', () => {
    const e = equityVsRanges(parseCards('AsAh'), [], [ANY_TWO], 20000, new Rng(1));
    expect(e).toBeGreaterThan(0.83);
    expect(e).toBeLessThan(0.87);
  });

  it('computes exact equity on the turn', () => {
    // Set vs nut flush draw on the turn.
    const [set, draw] = equityKnownHands(
      [parseCards('7c7d'), parseCards('AhKh')],
      parseCards('7h2h9sQc'),
      new Rng(2),
    );
    // 9 hearts left, but 9h and Qh pair the board and give the set a full house.
    expect(draw).toBeCloseTo(7 / 44, 5);
    expect(set + draw).toBeCloseTo(1, 10);
  });

  it('AKs vs QQ preflop is close to a coin flip', () => {
    const [ak, qq] = equityKnownHands([parseCards('AsKs'), parseCards('QhQd')], [], new Rng(3), 40000);
    expect(ak).toBeGreaterThan(0.44);
    expect(qq).toBeGreaterThan(0.52);
  });
});
