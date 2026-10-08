# Road to 100K — Poker Bankroll Tracker

A single-file bankroll tracker for the $500 → $100,000 poker bankroll challenge documented on YouTube.

## What it tracks

- **Current bankroll**, profit since day one, and percent progress to the goal
- **Milestone rail** on a log scale ($500 → $1k → $2.5k → $5k → $10k → $25k → $50k → $100k)
- **Session stats**: sessions played, hours, hourly rate, winning-session rate, best and worst session, current streak, profit per day
- **Suggested stakes**: the cash game (25 max buy-ins) and tournament average buy-in (100 buy-ins) the bankroll supports, plus a stakes ladder showing both at every milestone from $500 to $100k
- **Bankroll curve** by session, with the next milestone drawn as a target line
- **Session book** with buy-in, cash-out, hours, episode number, venue, and notes; edit or delete any row
- **Episode stat card**: a copy-ready summary for video descriptions
- **CSV export** of the whole session book

## Running it

Open `index.html` in any browser. Sessions are saved in that browser's local storage.

The same page is also published as a private claude.ai artifact with shared cloud storage, so sessions logged from a phone show up on a laptop and viewers can be given a read-only live link.

## Session types

- **Cash game**: buy-in, cash-out, hours. Result = cash-out − buy-in.
- **Tournament**: buy-in, payout, hours. Result = payout − buy-in.
- **Adjustment**: a signed amount for anything that is not a session (withdrawal, bonus, expense). Adjustments change the bankroll but are left out of session stats.

Settings (challenge name, starting bankroll, goal, start date) are under **Challenge settings** at the bottom of the page.
