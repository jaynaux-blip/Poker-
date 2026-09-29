import { Card } from '../cards';
import { ActionRecord, Hand, Street } from '../hand';

/**
 * Everything a player is allowed to know when making a decision: public
 * table state plus their own hole cards. Bots and the decision grader only
 * ever see this view, which is how the "AI never sees hidden cards"
 * guarantee is enforced structurally.
 */
export interface PublicSeat {
  seat: number;
  id: string;
  stack: number;
  streetBet: number;
  committed: number;
  folded: boolean;
  allIn: boolean;
}

export interface PlayerView {
  me: number; // index into seats
  hole: Card[];
  seats: PublicSeat[];
  board: Card[];
  street: Street;
  history: ActionRecord[];
  buttonIdx: number;
  bbIdx: number;
  sbIdx: number;
  smallBlind: number;
  bigBlind: number;
  ante: number;
  pot: number;
  currentBet: number;
  toCall: number;
  minRaiseTo: number;
  maxRaiseTo: number;
  canRaise: boolean;
  canCheck: boolean;
}

export function makeView(hand: Hand, seatIdx: number): PlayerView {
  const legal = hand.legalActions();
  return {
    me: seatIdx,
    hole: [...hand.seats[seatIdx].hole],
    seats: hand.seats.map((s) => ({
      seat: s.seat,
      id: s.id,
      stack: s.stack,
      streetBet: s.streetBet,
      committed: s.committed,
      folded: s.folded,
      allIn: s.allIn,
    })),
    board: [...hand.board],
    street: hand.street,
    history: hand.history.map((h) => ({ ...h })),
    buttonIdx: hand.buttonIndex,
    bbIdx: hand.bigBlindIndex,
    sbIdx: hand.smallBlindIndex,
    smallBlind: hand.smallBlind,
    bigBlind: hand.bigBlind,
    ante: hand.ante,
    pot: hand.pot,
    currentBet: hand.currentBet,
    toCall: legal.callAmount,
    minRaiseTo: legal.minRaiseTo,
    maxRaiseTo: legal.maxRaiseTo,
    canRaise: legal.canRaise,
    canCheck: legal.canCheck,
  };
}

export type PositionName = 'BTN' | 'SB' | 'BB' | 'UTG' | 'UTG+1' | 'MP' | 'LJ' | 'HJ' | 'CO';

/** Position label for a seat index, given the number of players dealt in. */
export function positionOf(view: Pick<PlayerView, 'seats' | 'buttonIdx' | 'sbIdx' | 'bbIdx'>, idx: number): PositionName {
  if (idx === view.buttonIdx) return 'BTN';
  if (idx === view.sbIdx) return 'SB';
  if (idx === view.bbIdx) return 'BB';
  const n = view.seats.length;
  // Seats between the big blind and the button, counted back from the button.
  const fromButton = (view.buttonIdx - idx + n) % n; // 1 = cutoff
  const names: PositionName[] = ['CO', 'HJ', 'LJ', 'MP', 'UTG+1', 'UTG'];
  const early = (idx - view.bbIdx + n) % n; // 1 = first to act
  if (early === 1 && n >= 4) return 'UTG';
  return names[Math.min(fromButton - 1, names.length - 1)];
}

/** Number of players still to act behind `idx` preflop who have not folded. */
export function playersBehind(view: PlayerView, idx: number): number {
  if (idx === view.bbIdx) return 0;
  const n = view.seats.length;
  let count = 0;
  for (let k = 1; k < n; k++) {
    const i = (idx + k) % n;
    const s = view.seats[i];
    if (!s.folded && !s.allIn) count++;
    // The big blind is the last to act preflop.
    if (i === view.bbIdx) break;
  }
  return count;
}

export interface StreetSummary {
  raises: number;
  limpers: number;
  lastRaiserSeat: number | null;
  lastRaiseTo: number;
}

export function preflopSummary(view: PlayerView): StreetSummary {
  let raises = 0;
  let limpers = 0;
  let lastRaiserSeat: number | null = null;
  let lastRaiseTo = view.bigBlind;
  for (const h of view.history) {
    if (h.street !== 'preflop') continue;
    if (h.type === 'raise' || h.type === 'bet') {
      raises++;
      lastRaiserSeat = h.seat;
      lastRaiseTo = h.to;
    } else if (h.type === 'call' && raises === 0) limpers++;
  }
  return { raises, limpers, lastRaiserSeat, lastRaiseTo };
}

export function seatIndex(view: PlayerView, seat: number): number {
  return view.seats.findIndex((s) => s.seat === seat);
}
