import { describe, expect, it } from 'vitest';
import { sprintProfile } from '../src/core/ai/profiles';
import { icm, payoutBands, payoutTable } from '../src/core/structure';
import { Tournament, TournamentSpec } from '../src/core/tournament';

const spec = (entrants: number): TournamentSpec => ({
  id: 't', name: 'Test', buyInCents: 110, feeCents: 10, guaranteeCents: 0, entrants,
  startingStack: 10000, levelMinutes: 5, secondsPerHand: 40, population: 'micro', speed: 'Turbo', startClock: 0,
});

describe('payouts', () => {
  it('sum exactly to the pool and are non-increasing', () => {
    for (const n of [9, 45, 180, 1000, 2400]) {
      const p = payoutTable(n * 100, n, 140);
      expect(p.reduce((a, b) => a + b, 0)).toBe(n * 100);
      for (let i = 1; i < p.length; i++) expect(p[i]).toBeLessThanOrEqual(p[i - 1] + 1);
      expect(p[p.length - 1]).toBeGreaterThanOrEqual(139);
    }
  });

  it('bands places after 9th', () => {
    expect(payoutBands(30).slice(8, 12)).toEqual([[9, 9], [10, 12], [13, 15], [16, 18]]);
  });
});

describe('ICM', () => {
  it('equal stacks get equal equity; total equals prize pool', () => {
    const e = icm([1000, 1000, 1000], [50, 30, 20]);
    for (const v of e) expect(v).toBeCloseTo(100 / 3, 6);
  });
  it('the chip leader gets less than chip-proportional equity', () => {
    const e = icm([7000, 2000, 1000], [50, 30, 20]);
    expect(e.reduce((a, b) => a + b, 0)).toBeCloseTo(100, 6);
    expect(e[0]).toBeLessThan(70);
    expect(e[2]).toBeGreaterThan(10);
  });
});

describe('tournament', () => {
  it('runs to completion with chip conservation, balanced tables and correct places', () => {
    const t = new Tournament(spec(180), 'Hero', 'full-run');
    const total = 180 * 10000;
    const places = new Set<number>();
    let guard = 0;
    while (!t.finished) {
      const ev = t.simulateTick(sprintProfile());
      for (const e of ev) if (e.t === 'bust') places.add(e.place);
      const chips = t.alivePlayers().reduce((a, p) => a + p.stack, 0);
      expect(chips).toBe(total);
      // Tables rebalance between rounds, before the next deal.
      t.balance();
      const sizes = [...t.tables.values()].map((tb) => tb.seats.filter(Boolean).length);
      expect(Math.max(...sizes) - Math.min(...sizes)).toBeLessThanOrEqual(1);
      // Every alive player sits exactly where the table says.
      for (const p of t.alivePlayers()) expect(t.tables.get(p.tableId)!.seats[p.seat]).toBe(p.id);
      if (++guard > 2000) throw new Error('did not finish');
    }
    places.add(1);
    expect(places.size).toBe(180);
    const prizes = [...t.players.values()].reduce((a, p) => a + p.prizeCents, 0);
    expect(prizes).toBe(t.prizePoolCents);
  });

  it('scripted moves can seat a rival at the hero table', () => {
    const t = new Tournament(spec(90), 'Hero', 'rival', [{ name: 'gh0stfold', archetype: 'crusher' }]);
    const heroTable = t.hero.tableId;
    t.moveToTable('npc:gh0stfold', heroTable);
    expect(t.players.get('npc:gh0stfold')!.tableId).toBe(heroTable);
    const seats = t.tables.get(heroTable)!.seats.filter(Boolean);
    expect(seats.length).toBeLessThanOrEqual(9);
    const all = [...t.tables.values()].flatMap((tb) => tb.seats.filter(Boolean));
    expect(new Set(all).size).toBe(all.length);
    expect(all.length).toBe(90);
  });
});
