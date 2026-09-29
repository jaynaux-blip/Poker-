import { Rng } from './rng';

/** Procedural online-poker screen names. */

const WORDS_A = [
  'River', 'Nut', 'Donk', 'Ace', 'Bluff', 'Stack', 'Fold', 'Shove', 'Grind', 'Tilt', 'Cooler', 'Flop', 'Lucky',
  'Kicker', 'Suited', 'Pocket', 'Chip', 'Felt', 'Muck', 'Turbo', 'Hyper', 'Deep', 'Short', 'Snap', 'Tank', 'Punt',
  'Value', 'Rail', 'Ship', 'Spew', 'Hero', 'Villain', 'Nit', 'Whale', 'Night', 'Neon', 'Coffee', 'Rent', 'Brick',
];
const WORDS_B = [
  'King', 'Lord', 'Ninja', 'Wizard', 'Queen', 'Master', 'Machine', 'Monster', 'Shark', 'Fish', 'Crusher', 'Rider',
  'Hunter', 'Boss', 'Owl', 'Ghost', 'Goblin', 'Pilot', 'Bandit', 'Doctor', 'Barber', 'Prophet', 'Dealer', 'Cowboy',
];
const FIRST = [
  'mike', 'jenny', 'kostas', 'dave', 'luis', 'priya', 'tomas', 'anna', 'jay', 'marco', 'sven', 'olga', 'kev', 'nina',
  'raj', 'sam', 'leo', 'mia', 'dmitri', 'yuki', 'hector', 'ben', 'chloe', 'omar', 'lars', 'ivy', 'gus', 'fran',
];
const WHOLE = [
  'VeggieLasagna', 'nolimitnancy', 'sunrun', 'd0nkeyk0ng', 'LaFleur77', 'grandmas_chips', 'BigSlickRick',
  'pocket_rockets', 'callmemaybe', 'the_nit_whisperer', 'IShoveAnyTwo', 'MinCashMike', 'RunItTwice', 'tiltedtower',
  'SetMineSally', 'BadBeatBobby', 'jamorfold', 'overlayhunter', 'deucesnever', 'ICMhero', 'floatdaddy', 'suitedconnector',
  'sleepybluffs', 'rakeback_ron', 'coldcalledit', 'QuadsOrBust', 'reg_n_roll', 'blindstealer', 'CheckRaiseCheryl',
];

function leet(s: string, rng: Rng): string {
  return s.replace(/[aeio]/g, (ch) => (rng.chance(0.35) ? ({ a: '4', e: '3', i: '1', o: '0' } as Record<string, string>)[ch] : ch));
}

export function screenName(rng: Rng): string {
  const roll = rng.next();
  if (roll < 0.18) return rng.pick(WHOLE) + (rng.chance(0.4) ? String(rng.int(99)) : '');
  if (roll < 0.45) return rng.pick(WORDS_A) + rng.pick(WORDS_B) + (rng.chance(0.5) ? String(rng.int(1000)) : '');
  if (roll < 0.7) {
    const sep = rng.pick(['', '_', '.', '']);
    return rng.pick(FIRST) + sep + (rng.chance(0.5) ? String(1965 + rng.int(40)) : rng.pick(WORDS_A).toLowerCase());
  }
  if (roll < 0.85) return leet(rng.pick(WORDS_A).toLowerCase() + rng.pick(WORDS_B).toLowerCase(), rng);
  return rng.pick(FIRST).toUpperCase() + rng.pick(['_', 'x', '']) + rng.pick(WORDS_B) + rng.pick(['', '!', 'xx', 'z']);
}

export function uniqueNames(count: number, rng: Rng, reserved: string[] = []): string[] {
  const seen = new Set(reserved.map((n) => n.toLowerCase()));
  const out: string[] = [];
  while (out.length < count) {
    let n = screenName(rng);
    if (n.length > 16) n = n.slice(0, 16);
    if (seen.has(n.toLowerCase())) n = n.slice(0, 13) + rng.int(1000);
    if (seen.has(n.toLowerCase())) continue;
    seen.add(n.toLowerCase());
    out.push(n);
  }
  return out;
}
