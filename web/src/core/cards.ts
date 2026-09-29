/**
 * Card encoding: an integer 0..51 where rank = card >> 2 (0 = deuce .. 12 = ace)
 * and suit = card & 3 (0 = clubs, 1 = diamonds, 2 = hearts, 3 = spades).
 */
export type Card = number;

export const RANK_CHARS = '23456789TJQKA';
export const SUIT_CHARS = 'cdhs';
export const SUIT_SYMBOLS = ['♣', '♦', '♥', '♠'];
export const RANK_NAMES = [
  'Deuce', 'Three', 'Four', 'Five', 'Six', 'Seven', 'Eight', 'Nine', 'Ten', 'Jack', 'Queen', 'King', 'Ace',
];
export const RANK_PLURALS = [
  'Deuces', 'Threes', 'Fours', 'Fives', 'Sixes', 'Sevens', 'Eights', 'Nines', 'Tens', 'Jacks', 'Queens', 'Kings', 'Aces',
];

export const rankOf = (c: Card): number => c >> 2;
export const suitOf = (c: Card): number => c & 3;
export const makeCard = (rank: number, suit: number): Card => (rank << 2) | suit;

export function cardToString(c: Card): string {
  return RANK_CHARS[rankOf(c)] + SUIT_CHARS[suitOf(c)];
}

export function parseCard(s: string): Card {
  const r = RANK_CHARS.indexOf(s[0].toUpperCase());
  const su = SUIT_CHARS.indexOf(s[1].toLowerCase());
  if (r < 0 || su < 0) throw new Error(`Bad card: ${s}`);
  return makeCard(r, su);
}

/** Parse "AsKd" or "As Kd" into cards. */
export function parseCards(s: string): Card[] {
  const clean = s.replace(/\s+/g, '');
  const out: Card[] = [];
  for (let i = 0; i < clean.length; i += 2) out.push(parseCard(clean.slice(i, i + 2)));
  return out;
}

export function fullDeck(): Card[] {
  const d: Card[] = [];
  for (let c = 0; c < 52; c++) d.push(c);
  return d;
}

/**
 * Starting-hand class index 0..168 (13x13 grid). Pairs on the diagonal,
 * suited hands above it (row = high rank), offsuit hands below.
 */
export function handClass(a: Card, b: Card): number {
  const ra = rankOf(a);
  const rb = rankOf(b);
  const hi = Math.max(ra, rb);
  const lo = Math.min(ra, rb);
  const suited = suitOf(a) === suitOf(b);
  // Grid indices use 12 - rank so AA sits at (0, 0).
  const row = 12 - hi;
  const col = 12 - lo;
  if (hi === lo) return row * 13 + col;
  return suited ? row * 13 + col : col * 13 + row;
}

/** Human label for a hand class, e.g. "AKs", "T9o", "77". */
export function handClassLabel(cls: number): string {
  const row = Math.floor(cls / 13);
  const col = cls % 13;
  if (row === col) return RANK_CHARS[12 - row] + RANK_CHARS[12 - row];
  if (col > row) return RANK_CHARS[12 - row] + RANK_CHARS[12 - col] + 's';
  return RANK_CHARS[12 - col] + RANK_CHARS[12 - row] + 'o';
}

/** Number of card combinations in a hand class (6 pairs, 4 suited, 12 offsuit). */
export function handClassCombos(cls: number): number {
  const row = Math.floor(cls / 13);
  const col = cls % 13;
  if (row === col) return 6;
  return col > row ? 4 : 12;
}
