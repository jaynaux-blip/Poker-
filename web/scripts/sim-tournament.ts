/** Headless tournament run for calibrating structures. Run: npx tsx scripts/sim-tournament.ts [entrants] [levelMinutes] */
import { sprintProfile } from '../src/core/ai/profiles';
import { Tournament } from '../src/core/tournament';

const entrants = Number(process.argv[2] ?? 1000);
const levelMinutes = Number(process.argv[3] ?? 5);
const t = new Tournament(
  { id: 'sim', name: 'Sim', buyInCents: 110, feeCents: 10, guaranteeCents: 100000, entrants, startingStack: 10000, levelMinutes, secondsPerHand: 40, population: 'micro', speed: 'Turbo', startClock: 30 },
  'Hero', Date.now(),
);
const total = entrants * 10000;
const t0 = performance.now();
let ftTick = -1; let bubbleTick = -1; let heroOut = -1;
while (!t.finished) {
  const ev = t.simulateTick(sprintProfile());
  for (const e of ev) {
    if (e.t === 'finalTable') ftTick = t.tick;
    if (e.t === 'bubble') bubbleTick = t.tick;
    if (e.t === 'bust' && e.isHero) heroOut = t.tick;
  }
  const chips = t.alivePlayers().reduce((a, p) => a + p.stack, 0);
  if (chips !== total) throw new Error(`chip leak at tick ${t.tick}: ${chips} vs ${total}`);
  if (t.tick % 25 === 0) console.log(`tick ${t.tick} lvl ${t.levelIndex + 1} ${t.level.sb}/${t.level.bb} left ${t.remaining} tables ${t.tables.size}`);
  if (t.tick > 3000) throw new Error('too long');
}
const ms = performance.now() - t0;
console.log(`done: ${t.tick} rounds, bubble @${bubbleTick}, final table @${ftTick}, hero out @${heroOut} place ${t.hero.place}; ${ms.toFixed(0)} ms total, ${(ms / t.tick).toFixed(2)} ms/round`);
console.log('payouts top:', t.payouts.slice(0, 10).map((c) => (c / 100).toFixed(2)).join(' '), '... min', (t.payouts[t.payouts.length - 1] / 100).toFixed(2), 'paid', t.paidPlaces, 'pool', t.prizePoolCents / 100);
