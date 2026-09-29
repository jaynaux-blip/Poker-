import { describe, expect, it } from 'vitest';
import { Session } from '../src/game/session';
import { LOBBY } from '../src/game/events';

function makeSession() {
  const log: string[] = [];
  const s = new Session({
    sound: () => {},
    text: (from, body) => log.push(`${from}: ${body}`),
    heartbeat: () => {},
    addCan: () => log.push('can'),
    celebrate: () => log.push('celebrate'),
  });
  return { s, log };
}

/** Drive a whole tournament with fake time and a random-ish hero. */
function playOut(eventIndex: number, pace: 'full' | 'smart', seed: number, maxSeconds = 60 * 60 * 6) {
  const { s, log } = makeSession();
  s.bankrollCents = 1000;
  s.screen = 'lobby';
  s.register(eventIndex);
  expect(s.screen).toBe('table');
  s.pace = pace;
  let now = 0;
  let r = seed;
  const rand = () => ((r = (r * 16807) % 2147483647) / 2147483647);
  let decisions = 0;
  while (now < maxSeconds && (s.screen as string) === 'table') {
    now += 0.1;
    s.update(now);
    const p = s.prompt;
    if (p && rand() < 0.3) {
      decisions++;
      const x = rand();
      if (x < 0.35) s.heroAct({ type: 'fold' });
      else if (x < 0.75) s.heroAct(p.canCheck ? { type: 'check' } : { type: 'call' });
      else s.heroAct({ type: 'raise', to: p.minRaise + Math.floor(rand() * (p.maxRaise - p.minRaise + 1)) });
    }
  }
  return { s, log, decisions, now };
}

describe('session flow', () => {
  it('plays a full hyper tournament to the results screen', () => {
    const { s, decisions } = playOut(1, 'smart', 7);
    expect(s.screen).toBe('results');
    expect(s.results).not.toBeNull();
    expect(decisions).toBeGreaterThan(0);
    expect(s.results!.grades.length).toBe(decisions);
    const t = s.t!;
    expect(t.hero.busted || t.finished).toBe(true);
    // Bankroll: 1000 - buy-in + prize.
    expect(s.bankrollCents).toBe(1000 - LOBBY[1].buyInCents + s.results!.prizeCents);
  }, 120000);

  it('survives many seeds in full pace without errors', () => {
    for (let seed = 1; seed <= 6; seed++) {
      const { s } = playOut(1, seed % 2 ? 'full' : 'smart', seed * 97);
      expect(['results', 'table']).toContain(s.screen);
    }
  }, 300000);

  it('sprint mode fast-forwards and stops at milestones or bust', () => {
    const { s } = makeSession();
    s.bankrollCents = 1000;
    s.register(0);
    s.pace = 'sprint';
    s.beginSprint();
    let now = 0;
    while (s.sprinting && now < 600) {
      now += 0.1;
      s.update(now);
    }
    expect(s.sprinting).toBe(false);
    const t = s.t!;
    const atMilestone = t.remaining <= t.paidPlaces + 15 || t.hero.busted || t.finished || t.tables.size === 1;
    expect(atMilestone).toBe(true);
  }, 120000);
});
