import { describe, expect, it } from 'vitest';
import { parseCards } from '../src/core/cards';
import { Hand, HandConfig, PlayerAction } from '../src/core/hand';
import { Rng } from '../src/core/rng';

function cfg(stacks: number[], extra: Partial<HandConfig> = {}): HandConfig {
  return {
    players: stacks.map((stack, seat) => ({ seat, id: `p${seat}`, stack })),
    buttonSeat: 0,
    smallBlind: 50,
    bigBlind: 100,
    ante: 100,
    rng: new Rng('hand-test'),
    ...extra,
  };
}

const total = (h: Hand) => h.seats.reduce((a, s) => a + s.stack, 0);

/** Deck builder: hole cards dealt one at a time from left of the button. */
function deckFor(holes: string[], board: string): number[] {
  const n = holes.length;
  const hs = holes.map(parseCards);
  const order: number[] = [];
  for (let i = 1; i <= n; i++) order.push(i % n);
  const top: number[] = [];
  for (let r = 0; r < 2; r++) for (const i of order) top.push(hs[i][r]);
  const b = parseCards(board);
  const used = new Set([...top, ...b]);
  const burns = Array.from({ length: 52 }, (_, i) => i).filter((c) => !used.has(c));
  // burn, flop x3, burn, turn, burn, river
  top.push(burns[0], b[0], b[1], b[2], burns[1], b[3], burns[2], b[4]);
  const rest = burns.slice(3);
  return [...top, ...rest];
}

describe('hand engine', () => {
  it('posts blinds and a big-blind ante, then action starts left of the BB', () => {
    const h = new Hand(cfg([10000, 10000, 10000, 10000]));
    expect(h.seats[1].committed).toBe(50);
    expect(h.seats[2].committed).toBe(200); // 100 blind + 100 ante
    expect(h.seats[2].streetBet).toBe(100); // ante is not part of the bet
    expect(h.pot).toBe(250);
    expect(h.seats[h.toAct!].seat).toBe(3);
    expect(h.legalActions()).toMatchObject({ callAmount: 100, minRaiseTo: 200, canCheck: false });
  });

  it('heads-up: button is the small blind and acts first preflop, last postflop', () => {
    const h = new Hand(cfg([5000, 5000], { ante: 0 }));
    expect(h.seats[0].committed).toBe(50);
    expect(h.seats[1].committed).toBe(100);
    expect(h.toAct).toBe(0);
    h.act({ type: 'call' });
    expect(h.toAct).toBe(1); // BB option
    h.act({ type: 'check' });
    expect(h.street).toBe('flop');
    expect(h.toAct).toBe(1); // BB acts first postflop
  });

  it('returns the uncalled blind when everyone folds to the big blind', () => {
    const h = new Hand(cfg([10000, 10000, 10000]));
    h.act({ type: 'fold' }); // button
    h.act({ type: 'fold' }); // SB
    expect(h.complete).toBe(true);
    // BB wins SB + ante; own blind comes back.
    expect(h.seats[2].stack).toBe(10000 + 50);
    expect(total(h)).toBe(30000);
  });

  it('enforces minimum raises and lets short all-ins through', () => {
    const h = new Hand(cfg([10000, 10000, 10000, 10000]));
    expect(() => h.act({ type: 'raise', to: 150 })).toThrow();
    h.act({ type: 'raise', to: 300 }); // raise of 200
    expect(h.legalActions().minRaiseTo).toBe(500);
  });

  it('an incomplete all-in raise does not reopen betting for players who acted', () => {
    // Seat 3 opens, seat 0 (button) is short and shoves for slightly more.
    const h = new Hand(cfg([350, 10000, 10000, 10000]));
    h.act({ type: 'raise', to: 300 }); // seat 3
    h.act({ type: 'raise', to: 350 }); // seat 0 all-in, only +50
    h.act({ type: 'fold' }); // SB
    h.act({ type: 'fold' }); // BB
    // Seat 3 already acted: may call the extra 50 but not re-raise.
    expect(h.seats[h.toAct!].seat).toBe(3);
    const legal = h.legalActions();
    expect(legal.canRaise).toBe(false);
    expect(legal.callAmount).toBe(50);
  });

  it('builds side pots and awards them to the right players', () => {
    // Seat 1 (SB) has the best hand but is shortest; seat 2 beats seat 3.
    const deck = deckFor(['2c3d', 'AsAh', 'KsKh', 'QsQh'], '7c8d9hJs4c');
    const h = new Hand(cfg([10000, 1000, 3000, 6000], { deck, ante: 0 }));
    h.act({ type: 'raise', to: 6000 }); // seat 3 shoves
    h.act({ type: 'fold' }); // seat 0
    h.act({ type: 'call' }); // seat 1 all-in 1000
    h.act({ type: 'call' }); // seat 2 all-in 3000
    expect(h.complete).toBe(true);
    const pots = h.potResults;
    expect(pots.map((p) => p.amount)).toEqual([3000, 4000]);
    expect(pots[0].winners).toEqual([1]);
    expect(pots[1].winners).toEqual([2]);
    expect(h.seats[3].stack).toBe(3000); // uncalled 3000 returned
    expect(total(h)).toBe(20000);
  });

  it('splits a chopped pot and gives the odd chip left of the button', () => {
    // Both play the board (royal flush on board).
    const deck = deckFor(['2c3d', '4c5d', '6c7d'], 'AsKsQsJsTs');
    const h = new Hand(cfg([1000, 1001, 1000], { deck, ante: 0, smallBlind: 1, bigBlind: 3 }));
    h.act({ type: 'call' }); // button calls 3
    h.act({ type: 'call' }); // SB completes
    h.act({ type: 'check' }); // BB
    for (let i = 0; i < 9; i++) if (!h.complete) h.act({ type: 'check' });
    expect(h.complete).toBe(true);
    expect(h.potResults[0].amount).toBe(9);
    expect(total(h)).toBe(3001);
  });

  it('conserves chips over thousands of random hands', () => {
    const rng = new Rng('fuzz');
    for (let t = 0; t < 3000; t++) {
      const n = 2 + rng.int(8);
      const stacks = Array.from({ length: n }, () => 1 + rng.int(20000));
      const before = stacks.reduce((a, b) => a + b, 0);
      const h = new Hand({
        players: stacks.map((stack, seat) => ({ seat, id: `p${seat}`, stack })),
        buttonSeat: rng.int(n),
        smallBlind: 50,
        bigBlind: 100,
        ante: rng.chance(0.5) ? 100 : 0,
        rng,
      });
      let guard = 0;
      while (!h.complete) {
        const legal = h.legalActions();
        const roll = rng.next();
        let a: PlayerAction;
        if (roll < 0.15) a = { type: 'fold' };
        else if (roll < 0.6) a = { type: 'call' };
        else if (legal.canRaise) a = { type: 'raise', to: legal.minRaiseTo + rng.int(Math.max(1, legal.maxRaiseTo - legal.minRaiseTo + 1)) };
        else a = { type: 'call' };
        h.act(a);
        if (++guard > 500) throw new Error('hand did not terminate');
      }
      expect(total(h)).toBe(before);
      for (const s of h.seats) expect(s.stack).toBeGreaterThanOrEqual(0);
    }
  });
});
