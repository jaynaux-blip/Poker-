import { TournamentSpec } from '../core/tournament';

/** Tournaments in the RiverLine lobby tonight. */
export interface LobbyEvent {
  spec: TournamentSpec | null; // null = flavor listing you can't join tonight
  name: string;
  start: string;
  buyInLabel: string;
  buyInCents: number;
  game: string;
  speed: string;
  entrantsLabel: string;
  guaranteeLabel: string;
  status: 'Late Reg' | 'Registering' | 'Starts Sun' | 'Running' | 'Tomorrow';
  blurb: string;
}

const base = { startingStack: 10000, secondsPerHand: 42, tableSize: 9 } as const;

export const LOBBY: LobbyEvent[] = [
  {
    spec: { ...base, id: 'night-owl', name: '$1.10 Night Owl Turbo', buyInCents: 110, feeCents: 10, guaranteeCents: 100000, entrants: 1000, levelMinutes: 5, population: 'micro', speed: 'Turbo', startClock: 2 * 60 + 11 },
    name: 'Night Owl Turbo',
    start: 'Now',
    buyInLabel: '$1.10',
    buyInCents: 110,
    game: "NL Hold'em",
    speed: 'Turbo · 5 min',
    entrantsLabel: '1,000',
    guaranteeLabel: '$1,000 GTD',
    status: 'Late Reg',
    blurb: 'The big one tonight. 1,000 grinders, 150 paid, $178 to the winner.',
  },
  {
    spec: { ...base, id: 'hyper-sprint', name: '$0.25 Hyper Sprint', buyInCents: 25, feeCents: 2, guaranteeCents: 0, entrants: 180, levelMinutes: 3, population: 'micro', speed: 'Hyper', startClock: 2 * 60 + 11 },
    name: 'Hyper Sprint',
    start: 'Now',
    buyInLabel: '$0.25',
    buyInCents: 25,
    game: "NL Hold'em",
    speed: 'Hyper · 3 min',
    entrantsLabel: '180',
    guaranteeLabel: '$41.40 pool',
    status: 'Late Reg',
    blurb: 'Quick and wild. 180 players, blinds up every 3 minutes.',
  },
  {
    spec: { ...base, id: 'freeroll', name: 'Midnight Freeroll', buyInCents: 0, feeCents: 0, guaranteeCents: 5000, entrants: 1000, levelMinutes: 4, population: 'freeroll', speed: 'Turbo', startClock: 2 * 60 + 11 },
    name: 'Midnight Freeroll',
    start: 'Now',
    buyInLabel: 'Free',
    buyInCents: 0,
    game: "NL Hold'em",
    speed: 'Turbo · 4 min',
    entrantsLabel: '1,000',
    guaranteeLabel: '$50 pool',
    status: 'Late Reg',
    blurb: 'Free to enter. Everyone shoves. Somebody has to win the $9.',
  },
  { spec: null, name: 'Bounty Builder', start: '4:00 AM', buyInLabel: '$5.50', buyInCents: 550, game: "NL Hold'em PKO", speed: 'Regular', entrantsLabel: '612', guaranteeLabel: '$3K GTD', status: 'Registering', blurb: '' },
  { spec: null, name: 'Big Stack $11', start: '8:00 PM', buyInLabel: '$11', buyInCents: 1100, game: "NL Hold'em", speed: 'Regular', entrantsLabel: '—', guaranteeLabel: '$10K GTD', status: 'Tomorrow', blurb: '' },
  { spec: null, name: 'Sunday Showdown', start: 'Sun 3:00 PM', buyInLabel: '$215', buyInCents: 21500, game: "NL Hold'em", speed: 'Regular', entrantsLabel: '—', guaranteeLabel: '$1M GTD', status: 'Starts Sun', blurb: '' },
  { spec: null, name: 'Step 1 → Grand Circuit', start: '5:30 AM', buyInLabel: '$2.20', buyInCents: 220, game: 'Satellite', speed: 'Turbo', entrantsLabel: '88', guaranteeLabel: '8 tickets', status: 'Registering', blurb: '' },
];
