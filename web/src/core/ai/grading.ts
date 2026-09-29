import { PlayerAction } from '../hand';
import { Rng } from '../rng';
import { OpponentModel, equityVsModels, modelOpponents } from './model';
import { PlayerView } from './view';

/**
 * Decision grading: estimates the chip EV of the options available at a
 * decision point and grades the one the player chose, chess-engine style.
 *
 * This is deliberately a lightweight model (equity vs the ranges implied by
 * opponents' actions, a fold-equity estimate for bets and raises, and an
 * equity-realization factor for hands that continue), not a solver. It is
 * accurate for all-in and river decisions and directionally right elsewhere.
 */

export type Grade = 'Best' | 'Good' | 'Inaccuracy' | 'Mistake' | 'Blunder';

export const GRADE_SCORE: Record<Grade, number> = {
  Best: 100,
  Good: 88,
  Inaccuracy: 62,
  Mistake: 30,
  Blunder: 0,
};

export interface OptionEV {
  label: string;
  action: PlayerAction;
  ev: number; // chips, relative to folding now
}

export interface DecisionAnalysis {
  view: PlayerView;
  equity: number;
  potOdds: number;
  options: OptionEV[];
  models: OpponentModel[];
}

export interface DecisionGrade {
  street: PlayerView['street'];
  grade: Grade;
  chosen: OptionEV;
  best: OptionEV;
  equity: number;
  potOdds: number;
  lossBB: number;
  note: string;
}

/**
 * How often one opponent folds to a bet or raise of `ratio` x the pot.
 * Folds scale with size: a min-bet gets almost no folds, a pot-sized bet gets
 * roughly what minimum-defense frequencies allow, and ranges that have shown
 * strength fold less.
 */
function foldProb(m: OpponentModel, ratio: number, preflop: boolean): number {
  const r = Math.max(0, ratio);
  const size = r / (r + 0.5); // 0 for tiny bets, 0.5 at half pot, 0.67 at pot
  if (preflop) {
    // Players yet to act preflop fold most hands to any real raise.
    if (m.band.max >= 0.9) return 0.55 + 0.35 * size;
    return 0.1 + 0.45 * size;
  }
  const maxFold = m.aggr >= 1 ? 0.3 : m.calls >= 1 ? 0.5 : 0.62;
  return maxFold * size;
}

function isInPosition(view: PlayerView): boolean {
  const n = view.seats.length;
  for (let k = 0; k < n; k++) {
    const i = (view.buttonIdx - k + n) % n;
    if (!view.seats[i].folded) return i === view.me;
  }
  return false;
}

export function analyzeDecision(view: PlayerView, rng: Rng, iterations = 1500): DecisionAnalysis {
  const models = modelOpponents(view);
  const equity = equityVsModels(view.hole, view.board, models, iterations, rng);
  const me = view.seats[view.me];
  const P = view.pot;
  const C = view.toCall;
  const preflop = view.street === 'preflop';
  const ip = isInPosition(view);
  const realize = view.street === 'river' ? 1 : preflop ? (ip ? 0.9 : 0.8) : ip ? 0.95 : 0.85;
  const options: OptionEV[] = [];

  if (C > 0) options.push({ label: 'Fold', action: { type: 'fold' }, ev: 0 });
  if (view.canCheck) {
    options.push({ label: 'Check', action: { type: 'check' }, ev: equity * realize * P });
  } else {
    const callAllIn = C >= me.stack;
    const r = callAllIn ? 1 : realize;
    options.push({ label: 'Call', action: { type: 'call' }, ev: equity * r * (P + C) - C });
  }

  if (view.canRaise) {
    const sizes = new Set<number>();
    const add = (to: number) => sizes.add(Math.max(view.minRaiseTo, Math.min(view.maxRaiseTo, Math.round(to))));
    add(view.minRaiseTo);
    if (view.currentBet === 0) {
      add(P * 0.5);
      add(P * 0.75);
      add(P);
    } else {
      add(view.currentBet * 2.5 + (P - view.currentBet) * 0.2);
      add(view.currentBet * 3.5 + (P - view.currentBet) * 0.3);
    }
    add(view.maxRaiseTo);
    for (const to of [...sizes].sort((a, b) => a - b)) {
      options.push({
        label: to === view.maxRaiseTo ? 'All-in' : `${view.currentBet === 0 ? 'Bet' : 'Raise to'} ${to}`,
        action: { type: 'raise', to },
        ev: raiseEV(view, models, equity, realize, to),
      });
    }
  }

  return { view, equity, potOdds: C > 0 ? C / (P + C) : 0, options, models };
}

function raiseEV(view: PlayerView, models: OpponentModel[], equity: number, realize: number, to: number): number {
  const me = view.seats[view.me];
  const P = view.pot;
  const B = to - me.streetBet; // chips we add
  const ratio = (to - view.currentBet) / Math.max(1, P);
  const preflop = view.street === 'preflop';
  let foldAll = 1;
  for (const m of models) foldAll *= foldProb(m, ratio, preflop);
  // Largest amount any single caller would need to add.
  let D = 0;
  for (const s of view.seats) {
    if (s.folded || s === me) continue;
    D = Math.max(D, Math.min(to - s.streetBet, s.stack));
  }
  const eqCalled = equity * (1 - 0.35 * (1 - equity) * Math.min(1, ratio));
  const r = to >= view.maxRaiseTo ? 1 : realize;
  return foldAll * P + (1 - foldAll) * (eqCalled * r * (P + B + D) - B);
}

export function evaluateChoice(analysis: DecisionAnalysis, action: PlayerAction): OptionEV {
  const view = analysis.view;
  if (action.type === 'fold') return { label: 'Fold', action, ev: view.canCheck ? -1e-9 : 0 };
  if (action.type === 'check') return analysis.options.find((o) => o.label === 'Check')!;
  if (action.type === 'call') return analysis.options.find((o) => o.label === 'Call' || o.label === 'Check')!;
  const preflop = view.street === 'preflop';
  const ip = isInPosition(view);
  const realize = view.street === 'river' ? 1 : preflop ? (ip ? 0.9 : 0.8) : ip ? 0.95 : 0.85;
  const to = Math.max(view.minRaiseTo, Math.min(view.maxRaiseTo, action.to));
  return {
    label: to === view.maxRaiseTo ? 'All-in' : `${view.currentBet === 0 ? 'Bet' : 'Raise to'} ${to}`,
    action: { type: 'raise', to },
    ev: raiseEV(view, analysis.models, analysis.equity, realize, to),
  };
}

export function gradeDecision(analysis: DecisionAnalysis, action: PlayerAction): DecisionGrade {
  const view = analysis.view;
  const chosen = evaluateChoice(analysis, action);
  let best = analysis.options[0];
  for (const o of analysis.options) if (o.ev > best.ev) best = o;
  if (chosen.ev > best.ev) best = chosen;
  const loss = Math.max(0, best.ev - chosen.ev);
  const scale = Math.max(view.pot, view.bigBlind * 4);
  const ratio = loss / scale;
  const grade: Grade =
    ratio < 0.04 ? 'Best' : ratio < 0.12 ? 'Good' : ratio < 0.28 ? 'Inaccuracy' : ratio < 0.55 ? 'Mistake' : 'Blunder';
  const lossBB = loss / Math.max(1, view.bigBlind);
  const eqPct = Math.round(analysis.equity * 100);
  const needPct = Math.round(analysis.potOdds * 100);
  let note: string;
  if (grade === 'Best' || grade === 'Good') {
    note = view.toCall > 0 ? `Equity about ${eqPct}% vs ${needPct}% needed to call.` : `Equity about ${eqPct}% against their likely range.`;
  } else {
    note = `${best.label} was better by ${lossBB.toFixed(1)} BB. ` +
      (view.toCall > 0 ? `You had about ${eqPct}% equity and needed ${needPct}%.` : `You had about ${eqPct}% equity.`);
  }
  return { street: view.street, grade, chosen, best, equity: analysis.equity, potOdds: analysis.potOdds, lossBB, note };
}

export function accuracy(grades: readonly DecisionGrade[]): number {
  if (grades.length === 0) return 0;
  return grades.reduce((a, g) => a + GRADE_SCORE[g.grade], 0) / grades.length;
}
