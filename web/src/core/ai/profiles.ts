import { Rng } from '../rng';

export type Archetype = 'fish' | 'station' | 'nit' | 'tag' | 'lag' | 'maniac' | 'reg' | 'crusher';

/** How a player's decision time relates to their hand: the online timing tell. */
export type TimingStyle = 'honest' | 'reverse' | 'balanced';

export interface Profile {
  archetype: Archetype;
  label: string;
  /** Multiplier on baseline opening ranges. */
  openWidth: number;
  /** Chance to limp a hand instead of raising it. */
  limpRate: number;
  /** Share of all hands 3-bet for value. */
  threeBet: number;
  /** Multiplier on flatting ranges versus a raise. */
  callWidth: number;
  /** Chance to bet or raise when holding a value hand. */
  aggression: number;
  /** Chance to bluff when a bluff is plausible. */
  bluff: number;
  /** 0..1: how far below pot odds this player keeps calling. */
  stickiness: number;
  /** Continuation-bet frequency as the preflop raiser. */
  cbet: number;
  /** Bet size multiplier. */
  sizing: number;
  /** How quickly bad beats push this player onto tilt. */
  tiltProne: number;
  /** 0..1: how closely short-stack play follows push/fold charts. */
  pushFold: number;
  /** 0..1: how much bubble and pay-jump pressure changes their play. */
  icmAware: number;
  timing: TimingStyle;
  /** Base and variable think time in milliseconds. */
  thinkBase: number;
  thinkVar: number;
}

const BASE: Record<Archetype, Omit<Profile, 'archetype' | 'timing'>> = {
  fish: { label: 'Fish', openWidth: 1.6, limpRate: 0.55, threeBet: 0.02, callWidth: 2.2, aggression: 0.45, bluff: 0.12, stickiness: 0.55, cbet: 0.5, sizing: 0.9, tiltProne: 0.8, pushFold: 0.2, icmAware: 0.1, thinkBase: 1400, thinkVar: 2600 },
  station: { label: 'Calling Station', openWidth: 1.3, limpRate: 0.4, threeBet: 0.015, callWidth: 2.6, aggression: 0.3, bluff: 0.05, stickiness: 0.85, cbet: 0.4, sizing: 0.8, tiltProne: 0.5, pushFold: 0.3, icmAware: 0.1, thinkBase: 1200, thinkVar: 1800 },
  nit: { label: 'Nit', openWidth: 0.6, limpRate: 0.05, threeBet: 0.015, callWidth: 0.6, aggression: 0.6, bluff: 0.04, stickiness: 0.05, cbet: 0.55, sizing: 1.0, tiltProne: 0.3, pushFold: 0.6, icmAware: 0.8, thinkBase: 1600, thinkVar: 2200 },
  tag: { label: 'TAG', openWidth: 1.0, limpRate: 0.02, threeBet: 0.04, callWidth: 1.0, aggression: 0.75, bluff: 0.18, stickiness: 0.2, cbet: 0.65, sizing: 1.0, tiltProne: 0.35, pushFold: 0.85, icmAware: 0.6, thinkBase: 1500, thinkVar: 2800 },
  lag: { label: 'LAG', openWidth: 1.45, limpRate: 0.02, threeBet: 0.075, callWidth: 1.3, aggression: 0.85, bluff: 0.32, stickiness: 0.3, cbet: 0.75, sizing: 1.1, tiltProne: 0.45, pushFold: 0.8, icmAware: 0.4, thinkBase: 1200, thinkVar: 2600 },
  maniac: { label: 'Maniac', openWidth: 3.2, limpRate: 0.05, threeBet: 0.2, callWidth: 2.2, aggression: 0.95, bluff: 0.5, stickiness: 0.45, cbet: 0.9, sizing: 1.45, tiltProne: 0.7, pushFold: 0.25, icmAware: 0.05, thinkBase: 700, thinkVar: 1200 },
  reg: { label: 'Reg', openWidth: 1.1, limpRate: 0.01, threeBet: 0.05, callWidth: 1.05, aggression: 0.8, bluff: 0.24, stickiness: 0.22, cbet: 0.62, sizing: 1.0, tiltProne: 0.25, pushFold: 0.95, icmAware: 0.85, thinkBase: 1800, thinkVar: 3200 },
  crusher: { label: 'Crusher', openWidth: 1.2, limpRate: 0.0, threeBet: 0.06, callWidth: 1.1, aggression: 0.82, bluff: 0.28, stickiness: 0.24, cbet: 0.58, sizing: 1.0, tiltProne: 0.1, pushFold: 1.0, icmAware: 1.0, thinkBase: 2000, thinkVar: 3000 },
};

/** Opponent mix for each stake band. Micros are soft; high rollers are not. */
export const POPULATIONS: Record<string, Partial<Record<Archetype, number>>> = {
  freeroll: { fish: 38, station: 18, maniac: 14, nit: 12, tag: 10, lag: 5, reg: 3 },
  micro: { fish: 26, station: 18, nit: 15, tag: 16, lag: 8, maniac: 7, reg: 10 },
  low: { fish: 16, station: 12, nit: 16, tag: 24, lag: 12, maniac: 4, reg: 15, crusher: 1 },
  high: { fish: 4, nit: 6, tag: 20, lag: 15, reg: 35, crusher: 20 },
};

export function makeProfile(archetype: Archetype, rng: Rng): Profile {
  const b = BASE[archetype];
  // Individual variation so no two Fish play identically.
  const jitter = (v: number, amt = 0.12) => Math.max(0, v * (1 + rng.gauss(0, amt)));
  const timingRoll = rng.next();
  const timing: TimingStyle =
    archetype === 'crusher' || archetype === 'reg'
      ? timingRoll < 0.75
        ? 'balanced'
        : 'honest'
      : timingRoll < 0.65
        ? 'honest'
        : timingRoll < 0.85
          ? 'reverse'
          : 'balanced';
  return {
    archetype,
    timing,
    label: b.label,
    openWidth: jitter(b.openWidth),
    limpRate: Math.min(1, jitter(b.limpRate)),
    threeBet: jitter(b.threeBet, 0.2),
    callWidth: jitter(b.callWidth),
    aggression: Math.min(1, jitter(b.aggression, 0.08)),
    bluff: Math.min(1, jitter(b.bluff, 0.2)),
    stickiness: Math.min(0.95, jitter(b.stickiness, 0.15)),
    cbet: Math.min(1, jitter(b.cbet, 0.1)),
    sizing: jitter(b.sizing, 0.08),
    tiltProne: Math.min(1, jitter(b.tiltProne, 0.2)),
    pushFold: Math.min(1, jitter(b.pushFold, 0.1)),
    icmAware: Math.min(1, jitter(b.icmAware, 0.15)),
    thinkBase: jitter(b.thinkBase, 0.2),
    thinkVar: jitter(b.thinkVar, 0.2),
  };
}

export function rollArchetype(population: Partial<Record<Archetype, number>>, rng: Rng): Archetype {
  const entries = Object.entries(population) as [Archetype, number][];
  const total = entries.reduce((a, [, w]) => a + w, 0);
  let r = rng.next() * total;
  for (const [a, w] of entries) {
    r -= w;
    if (r <= 0) return a;
  }
  return entries[entries.length - 1][0];
}

/** Solid default used for Sprint mode, where the game plays the hero's hands. */
export function sprintProfile(): Profile {
  return { archetype: 'tag', timing: 'balanced', ...BASE.tag, thinkBase: 0, thinkVar: 0 };
}
