# Road to 100K — Poker Bankroll Tracker

A single-file bankroll tracker for the $500 → $100,000 poker bankroll challenge documented on YouTube.

## What it tracks

- **Current bankroll**, profit since day one, and percent progress to the goal
- **Milestone rail** on a log scale ($500 → $1k → $2.5k → $5k → $10k → $25k → $50k → $100k)
- **Session stats**: sessions played, hours, hourly rate, winning-session rate, best and worst session, current streak, profit per day
- **Suggested stakes**: the cash game (25 max buy-ins) and tournament average buy-in (100 buy-ins) the bankroll supports, plus a stakes ladder showing both at every milestone from $500 to $100k
- **Bankroll curve** by session, with the next milestone drawn as a target line
- **Session book** with buy-in, cash-out, hours, episode number, venue, and notes; edit or delete any row
- **Episode stat card** and **YouTube tags**: copy-ready blocks for video descriptions; the tags fill in the stakes, site and episode number from the latest session and stay under the 500-character limit
- **CSV export** of the whole session book

## Running it

Open `index.html` in any browser. Sessions are saved in that browser's local storage.

The same page is also published as a private claude.ai artifact with shared cloud storage, so sessions logged from a phone show up on a laptop and viewers can be given a read-only live link.

## Session types

- **Cash game**: buy-in, cash-out, hours. Result = cash-out − buy-in.
- **Tournament**: buy-in, payout, hours. Result = payout − buy-in.
- **Adjustment**: a signed amount for anything that is not a session (withdrawal, bonus, expense). Adjustments change the bankroll but are left out of session stats.

Settings (challenge name, starting bankroll, goal, start date) are under **Challenge settings** at the bottom of the page.

## Range builder

`ranges.html` is a preflop range builder for tournaments. Pick the table size, spot (RFI, vs RFI, vs 3-bet), hero and villain positions, effective stack and model (Chip EV or three ICM presets), and the 13×13 grid fills with a default range. Paint cells with the raise, call, shove or fold brush at 100, 75, 50 or 25 percent, save the edit for that spot, copy the range string, or paste one in.

The defaults are not solver output. They come from two standard hand orderings (deep-stack playability and push-fold equity) and percentage targets per position and depth. The ICM presets shift the Chip EV baseline: calls tighten, and aggression widens when hero covers the villain and narrows when covered. Treat the grids as a starting point and edit them.

## Schedule and game plan

`SCHEDULE.md` / `schedule.pdf` hold the weekly session and study schedule. `MTT-GAME-PLAN.md` / `mtt-game-plan.pdf` hold the tournament game plan: selection, stage-by-stage strategy, exploits, stack-depth rules, ICM, block rules and the timeline.
