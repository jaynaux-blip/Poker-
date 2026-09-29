import { describe, expect, it } from 'vitest';
import { parseCards } from '../src/core/cards';
import { Hand } from '../src/core/hand';
import { Rng } from '../src/core/rng';
import { decide } from '../src/core/ai/bot';
import { Archetype, makeProfile } from '../src/core/ai/profiles';
import { makeView } from '../src/core/ai/view';
import { analyzeDecision, gradeDecision } from '../src/core/ai/grading';

function deckWithHole(holes: string[]): number[] {
  const n = holes.length;
  const hs = holes.map(parseCards);
  const order: number[] = [];
  for (let i = 1; i <= n; i++) order.push(i % n);
  const top: number[] = [];
  for (let r = 0; r < 2; r++) for (const i of order) top.push(hs[i][r]);
  const used = new Set(top);
  return [...top, ...Array.from({ length: 52 }, (_, i) => i).filter((c) => !used.has(c))];
}

function play(archetypes: Archetype[], hands: number, seed: string, fast = false) {
  const rng = new Rng(seed);
  const n = archetypes.length;
  const profiles = archetypes.map((a) => makeProfile(a, rng));
  const vpip = new Array(n).fill(0);
  const pfr = new Array(n).fill(0);
  let button = 0;
  for (let t = 0; t < hands; t++) {
    const h = new Hand({
      players: archetypes.map((_, seat) => ({ seat, id: `p${seat}`, stack: 10000 })),
      buttonSeat: button,
      smallBlind: 50,
      bigBlind: 100,
      ante: 100,
      rng,
    });
    button = (button + 1) % n;
    const vol = new Set<number>();
    const raised = new Set<number>();
    let guard = 0;
    while (!h.complete) {
      const idx = h.toAct!;
      const view = makeView(h, idx);
      const d = decide(view, { profile: profiles[idx], tilt: 0, icmPressure: 0, fast, rng });
      if (h.street === 'preflop') {
        if (d.action.type === 'call' || d.action.type === 'raise') vol.add(idx);
        if (d.action.type === 'raise') raised.add(idx);
      }
      h.act(d.action);
      if (++guard > 400) throw new Error('did not terminate');
    }
    for (const i of vol) vpip[i]++;
    for (const i of raised) pfr[i]++;
  }
  return { vpip: vpip.map((v) => v / hands), pfr: pfr.map((v) => v / hands) };
}

describe('bots', () => {
  it('never fold aces preflop and fold 72o under the gun', () => {
    const rng = new Rng('aces');
    for (const arch of ['fish', 'nit', 'tag', 'maniac', 'crusher'] as Archetype[]) {
      const profile = makeProfile(arch, rng);
      for (let t = 0; t < 30; t++) {
        // Seat 3 is UTG with a 9-handed table (button 0).
        const holes = Array.from({ length: 9 }, () => '');
        const hole = t % 2 === 0 ? 'AsAh' : '7c2d';
        const deckCards = deckWithHole(holes.map((_, i) => (i === 3 ? hole : ['Kd9s', 'Qc8h', 'Jd4c', '', 'Th3s', '9c5d', '8s6h', '6c4h', '5s3d'][i])));
        const h = new Hand({
          players: holes.map((_, seat) => ({ seat, id: `p${seat}`, stack: 10000 })),
          buttonSeat: 0, smallBlind: 50, bigBlind: 100, ante: 100, rng, deck: deckCards,
        });
        expect(h.seats[h.toAct!].seat).toBe(3);
        const d = decide(makeView(h, h.toAct!), { profile, tilt: 0, icmPressure: 0, fast: false, rng });
        if (hole === 'AsAh') expect(d.action.type).not.toBe('fold');
        else if (arch !== 'maniac' && arch !== 'fish') expect(d.action.type).toBe('fold');
      }
    }
  });

  it('archetypes produce distinct, plausible preflop stats', () => {
    const archs: Archetype[] = ['nit', 'tag', 'lag', 'fish', 'station', 'maniac'];
    const { vpip, pfr } = play(archs, 1500, 'stats');
    const byArch = Object.fromEntries(archs.map((a, i) => [a, { vpip: vpip[i], pfr: pfr[i] }]));
    // eslint-disable-next-line no-console
    console.log(Object.entries(byArch).map(([a, s]) => `${a}: VPIP ${(s.vpip * 100).toFixed(0)} PFR ${(s.pfr * 100).toFixed(0)}`).join(' | '));
    expect(byArch.nit.vpip).toBeLessThan(byArch.tag.vpip);
    expect(byArch.tag.vpip).toBeLessThan(byArch.maniac.vpip);
    expect(byArch.fish.vpip - byArch.fish.pfr).toBeGreaterThan(byArch.tag.vpip - byArch.tag.pfr);
    expect(byArch.nit.vpip).toBeGreaterThan(0.06);
    expect(byArch.maniac.vpip).toBeLessThan(0.75);
  });

  it('fast bots play complete hands without errors', () => {
    play(['tag', 'fish', 'nit', 'lag', 'station', 'maniac', 'reg', 'crusher', 'fish'], 400, 'fast', true);
  });
});

describe('decision grading', () => {
  it('folding aces preflop is a blunder and shoving them is fine', () => {
    const rng = new Rng('grade');
    const deckCards = deckWithHole(['Kd9s', 'Qc8h', 'AsAh', '7c2d']);
    const h = new Hand({
      players: [0, 1, 2, 3].map((seat) => ({ seat, id: `p${seat}`, stack: 1000 })),
      buttonSeat: 0, smallBlind: 50, bigBlind: 100, ante: 100, rng, deck: deckCards,
    });
    h.act({ type: 'raise', to: 1000 }); // seat 3 shoves
    h.act({ type: 'fold' }); // button
    h.act({ type: 'fold' }); // SB
    // BB (seat 2) holds aces facing a shove.
    const a = analyzeDecision(makeView(h, h.toAct!), rng);
    expect(gradeDecision(a, { type: 'fold' }).grade).toBe('Blunder');
    expect(gradeDecision(a, { type: 'call' }).grade).toBe('Best');
  });

  it('calling off with 72o against a tight shove is graded as a mistake or worse', () => {
    const rng = new Rng('grade2');
    const deckCards = deckWithHole(['Kd9s', 'Qc8h', '7c2d', 'AsKh']);
    // Hero (seat 2, big blind) holds 7-2 offsuit facing a 50bb UTG shove.
    const h2 = new Hand({
      players: [0, 1, 2, 3].map((seat) => ({ seat, id: `p${seat}`, stack: 5000 })),
      buttonSeat: 0, smallBlind: 50, bigBlind: 100, ante: 100, rng, deck: deckCards,
    });
    h2.act({ type: 'raise', to: 5000 }); // UTG shoves 50bb
    h2.act({ type: 'fold' });
    h2.act({ type: 'fold' });
    const a = analyzeDecision(makeView(h2, h2.toAct!), rng);
    const g = gradeDecision(a, { type: 'call' });
    expect(['Mistake', 'Blunder']).toContain(g.grade);
    expect(gradeDecision(a, { type: 'fold' }).grade).toBe('Best');
  });
});
