import { Card, cardToString } from '../core/cards';
import { describe, evaluateHand } from '../core/evaluator';
import { payoutBands } from '../core/structure';
import { HERO_ID } from '../core/tournament';
import { LOBBY } from '../game/events';
import { Flight, Session, SeatVis } from '../game/session';
import { C, H, MONO, Rect, UI, W, chips, clockString, ease, money, ordinal } from './canvasui';
import { drawCard, drawChipStack } from './cardart';

/**
 * RiverLine: the fictional poker site running on the laptop. Draws the boot
 * screen, lobby, table and results into the UI canvas each frame.
 */

const TABLE = { cx: 590, cy: 430, rx: 420, ry: 226 };
const TOP = 64;
const SIDE_X = 1188;

const GRADE_STYLE: Record<string, { color: string; icon: string }> = {
  Best: { color: '#3ecf6e', icon: '★' },
  Good: { color: '#27d3c3', icon: '✓' },
  Inaccuracy: { color: '#f2c14e', icon: '?!' },
  Mistake: { color: '#f28a3a', icon: '?' },
  Blunder: { color: '#ef4d5a', icon: '??' },
};

function hashColor(name: string): string {
  let h = 0;
  for (let i = 0; i < name.length; i++) h = (h * 31 + name.charCodeAt(i)) >>> 0;
  const hue = h % 360;
  return `hsl(${hue}, 45%, 42%)`;
}

export class RiverLine {
  private keyRaise = 0;
  onLeanBack: (() => void) | null = null;

  constructor(private ui: UI, private s: Session) {}

  draw(now: number): void {
    const ui = this.ui;
    const c = ui.ctx;
    c.save();
    const bg = c.createLinearGradient(0, 0, 0, H);
    bg.addColorStop(0, C.bg2);
    bg.addColorStop(1, C.bg);
    c.fillStyle = bg;
    c.fillRect(0, 0, W, H);
    this.topBar(now);
    switch (this.s.screen) {
      case 'boot':
        this.boot(now);
        break;
      case 'lobby':
        this.lobby(now);
        break;
      case 'table':
        this.table(now);
        break;
      case 'results':
        this.resultsScreen(now);
        break;
    }
    this.drawCursor();
    c.restore();
  }

  // ------------------------------------------------------------------ chrome

  private logo(x: number, y: number, scale = 1): void {
    const c = this.ui.ctx;
    c.save();
    c.translate(x, y);
    c.scale(scale, scale);
    c.strokeStyle = C.accent;
    c.lineWidth = 5;
    c.lineCap = 'round';
    for (let k = 0; k < 2; k++) {
      c.beginPath();
      for (let i = 0; i <= 36; i++) {
        const px = i;
        const py = 6 + k * 11 + Math.sin((i / 36) * Math.PI * 2) * 5;
        if (i === 0) c.moveTo(px, py);
        else c.lineTo(px, py);
      }
      c.globalAlpha = k === 0 ? 1 : 0.55;
      c.stroke();
    }
    c.globalAlpha = 1;
    this.ui.text('River', 48, 26, { size: 28, weight: 800, color: C.ink });
    this.ui.text('Line', 48 + this.ui.measure('River', 28, 800), 26, { size: 28, weight: 400, color: C.accent });
    c.restore();
  }

  private topBar(now: number): void {
    const ui = this.ui;
    const c = ui.ctx;
    c.fillStyle = '#08101b';
    c.fillRect(0, 0, W, TOP);
    c.fillStyle = C.line;
    c.fillRect(0, TOP - 1, W, 1);
    this.logo(24, 16, 1);
    const s = this.s;
    if (s.screen !== 'boot') {
      const tabs = ['Lobby', 'My Tournaments', 'Cashier'];
      let x = 250;
      tabs.forEach((t, i) => {
        const active = (i === 0 && s.screen === 'lobby') || (i === 1 && (s.screen === 'table' || s.screen === 'results'));
        ui.text(t, x, 40, { size: 17, weight: active ? 700 : 500, color: active ? C.ink : C.muted });
        if (active) ui.rrect({ x, y: TOP - 4, w: ui.measure(t, 17, 700), h: 3 }, 1.5, C.accent);
        x += ui.measure(t, 17, 700) + 34;
      });
    }
    // Balance and clock.
    const clock = clockString(s.clockMinutes());
    ui.text(clock, W - 24, 40, { size: 17, weight: 600, color: C.muted, align: 'right', font: MONO });
    const bal = `Balance ${money(s.bankrollCents)}`;
    const bw = ui.measure(bal, 17, 700) + 28;
    ui.rrect({ x: W - 140 - bw, y: 16, w: bw, h: 32 }, 16, '#10213a', C.line);
    ui.text(bal, W - 140 - bw / 2, 38, { size: 17, weight: 700, color: C.gold, align: 'center' });
    // Connection dot.
    c.beginPath();
    c.arc(W - 128, 32, 4, 0, Math.PI * 2);
    c.fillStyle = Math.sin(now * 2) > -0.9 ? C.green : C.dim;
    c.fill();
  }

  private drawCursor(): void {
    const p = this.ui.pointer;
    if (!p.active) return;
    const c = this.ui.ctx;
    c.save();
    c.translate(p.x, p.y);
    c.beginPath();
    if (this.ui.cursor === 'pointer') {
      c.arc(0, 0, 9, 0, Math.PI * 2);
      c.fillStyle = 'rgba(39,211,195,0.25)';
      c.fill();
      c.strokeStyle = '#ffffff';
      c.lineWidth = 2;
      c.stroke();
    } else {
      c.moveTo(0, 0);
      c.lineTo(0, 26);
      c.lineTo(7, 20);
      c.lineTo(12, 31);
      c.lineTo(16, 29);
      c.lineTo(11, 18);
      c.lineTo(19, 18);
      c.closePath();
      c.fillStyle = '#ffffff';
      c.fill();
      c.strokeStyle = '#000';
      c.lineWidth = 1.5;
      c.stroke();
    }
    c.restore();
  }

  // ------------------------------------------------------------------ boot

  private boot(now: number): void {
    const ui = this.ui;
    const c = ui.ctx;
    // Animated waves.
    c.save();
    c.globalAlpha = 0.18;
    for (let k = 0; k < 6; k++) {
      c.beginPath();
      for (let x = 0; x <= W; x += 8) {
        const y = 640 + k * 34 + Math.sin(x / 140 + now * 0.6 + k) * 18;
        if (x === 0) c.moveTo(x, y);
        else c.lineTo(x, y);
      }
      c.strokeStyle = k % 2 ? C.accent : C.accent2;
      c.lineWidth = 2;
      c.stroke();
    }
    c.restore();
    const card: Rect = { x: W / 2 - 280, y: 200, w: 560, h: 420 };
    ui.rrect(card, 18, 'rgba(17,28,46,0.92)', C.line);
    this.logo(card.x + 150, card.y + 50, 1.6);
    ui.text(`Welcome back, ${this.s.heroName}`, W / 2, card.y + 175, { size: 26, weight: 700, align: 'center' });
    ui.text('Account balance', W / 2, card.y + 225, { size: 16, color: C.muted, align: 'center' });
    ui.text(money(this.s.bankrollCents), W / 2, card.y + 275, { size: 44, weight: 800, color: C.gold, align: 'center', font: MONO });
    if (ui.button('login', { x: card.x + 60, y: card.y + 320, w: card.w - 120, h: 64 }, 'Log in', { kind: 'primary', size: 24 })) {
      this.s.screen = 'lobby';
      this.s.onBoot();
    }
    ui.text('Play responsibly. RiverLine is a fictional site.', W / 2, card.y + card.h + 40, { size: 14, color: C.dim, align: 'center' });
  }

  // ------------------------------------------------------------------ lobby

  private lobby(_now: number): void {
    const ui = this.ui;
    const s = this.s;
    const x0 = 24;
    const y0 = TOP + 24;
    ui.text('Tournaments', x0, y0 + 30, { size: 30, weight: 800 });
    ui.text("NL Hold'em · Tonight", x0 + 210, y0 + 30, { size: 17, color: C.muted });
    const cols = [
      { t: 'Start', x: 0, w: 110 },
      { t: 'Tournament', x: 110, w: 250 },
      { t: 'Buy-in', x: 360, w: 100 },
      { t: 'Speed', x: 460, w: 140 },
      { t: 'Entrants', x: 600, w: 110 },
      { t: 'Prize', x: 710, w: 140 },
      { t: 'Status', x: 850, w: 130 },
    ];
    const tx = x0;
    const ty = y0 + 60;
    const tw = 1000;
    ui.rrect({ x: tx, y: ty, w: tw, h: 44 }, 8, C.panel2);
    for (const col of cols) ui.text(col.t, tx + 18 + col.x, ty + 28, { size: 14, weight: 700, color: C.muted });
    LOBBY.forEach((ev, i) => {
      const r: Rect = { x: tx, y: ty + 52 + i * 62, w: tw, h: 56 };
      const st = ui.clickable(`row${i}`, r);
      if (st.clicked) {
        s.selected = i;
        s.confirmRegister = false;
      }
      const sel = s.selected === i;
      ui.rrect(r, 10, sel ? '#15304a' : st.hover ? '#132238' : C.panel, sel ? C.accent : undefined, sel ? 1.5 : 1);
      const dim = ev.spec === null;
      const col = dim ? C.dim : C.ink;
      const y = r.y + 35;
      ui.text(ev.start, tx + 18, y, { size: 16, weight: 600, color: ev.start === 'Now' ? C.accent : col });
      ui.text(ev.name, tx + 18 + 110, y, { size: 17, weight: 700, color: col, maxWidth: 240 });
      ui.text(ev.buyInLabel, tx + 18 + 360, y, { size: 17, weight: 700, color: dim ? C.dim : C.gold, font: MONO });
      ui.text(ev.speed, tx + 18 + 460, y, { size: 15, color: dim ? C.dim : C.muted });
      ui.text(ev.entrantsLabel, tx + 18 + 600, y, { size: 16, color: col, font: MONO });
      ui.text(ev.guaranteeLabel, tx + 18 + 710, y, { size: 16, weight: 600, color: col });
      const stColor = ev.status === 'Late Reg' ? C.green : ev.status === 'Registering' ? C.accent2 : C.dim;
      ui.rrect({ x: tx + 18 + 850, y: r.y + 15, w: 110, h: 26 }, 13, 'rgba(255,255,255,0.04)', stColor);
      ui.text(ev.status, tx + 18 + 905, r.y + 33, { size: 13, weight: 700, color: stColor, align: 'center' });
    });

    // Detail panel.
    const ev = LOBBY[s.selected];
    const px = 1048;
    const pr: Rect = { x: px, y: y0, w: W - px - 24, h: 700 };
    ui.rrect(pr, 14, C.panel, C.line);
    ui.text(ev.name, px + 24, y0 + 48, { size: 28, weight: 800, maxWidth: pr.w - 48 });
    ui.text(`${ev.game} · ${ev.speed}`, px + 24, y0 + 80, { size: 16, color: C.muted });
    if (ev.spec) {
      const sp = ev.spec;
      const facts: [string, string][] = [
        ['Buy-in', ev.buyInCents === 0 ? 'Free' : `${money(sp.buyInCents - sp.feeCents)} + ${money(sp.feeCents)}`],
        ['Starting stack', `${chips(sp.startingStack)} (100 BB)`],
        ['Blind levels', `${sp.levelMinutes} minutes`],
        ['Players', chips(sp.entrants)],
        ['Places paid', chips(Math.round(sp.entrants * 0.15))],
        ['Prize pool', ev.guaranteeLabel],
      ];
      facts.forEach(([k, v], i) => {
        const y = y0 + 130 + i * 40;
        ui.text(k, px + 24, y, { size: 16, color: C.muted });
        ui.text(v, pr.x + pr.w - 24, y, { size: 17, weight: 700, align: 'right' });
      });
      this.wrap(ev.blurb, px + 24, y0 + 400, pr.w - 48, 17, C.ink, 24);
      const afford = s.canAfford(ev);
      const by = y0 + 560;
      if (!s.confirmRegister) {
        if (ui.button('reg', { x: px + 24, y: by, w: pr.w - 48, h: 64 }, afford ? `Register · ${ev.buyInLabel}` : 'Insufficient funds', { kind: afford ? 'primary' : 'secondary', enabled: afford, size: 22 })) {
          s.confirmRegister = true;
        }
      } else {
        ui.text(`Balance after: ${money(s.bankrollCents - ev.buyInCents)}`, px + 24, by - 14, { size: 15, color: C.muted });
        if (ui.button('regok', { x: px + 24, y: by, w: (pr.w - 60) / 2, h: 64 }, 'Confirm', { kind: 'gold', size: 22 })) {
          s.confirmRegister = false;
          s.register(s.selected);
        }
        if (ui.button('regno', { x: px + 36 + (pr.w - 60) / 2, y: by, w: (pr.w - 60) / 2, h: 64 }, 'Cancel', { kind: 'ghost', size: 22 })) {
          s.confirmRegister = false;
        }
      }
    } else {
      this.wrap('Registration for this event opens later. Tonight, the grind starts smaller.', px + 24, y0 + 150, pr.w - 48, 17, C.muted, 24);
    }

    // Recent results.
    const ry = ty + 52 + LOBBY.length * 62 + 26;
    ui.text('Recent results', x0, ry, { size: 18, weight: 700 });
    if (s.history.length === 0) ui.text('No tournaments yet tonight.', x0, ry + 32, { size: 15, color: C.muted });
    s.history.slice(0, 3).forEach((h, i) => {
      const y = ry + 32 + i * 28;
      ui.text(h.name, x0, y, { size: 15, color: C.muted, maxWidth: 360 });
      ui.text(`${ordinal(h.place)} / ${chips(h.entrants)}`, x0 + 400, y, { size: 15, font: MONO });
      ui.text(h.prize > 0 ? `+${money(h.prize)}` : '—', x0 + 560, y, { size: 15, weight: 700, color: h.prize > 0 ? C.green : C.dim, font: MONO });
      ui.text(`Accuracy ${h.accuracy.toFixed(0)}`, x0 + 700, y, { size: 15, color: C.muted });
    });
  }

  private wrap(text: string, x: number, y: number, maxW: number, size: number, color: string, lh: number): number {
    const words = text.split(' ');
    let line = '';
    let yy = y;
    for (const w of words) {
      const test = line ? `${line} ${w}` : w;
      if (this.ui.measure(test, size) > maxW && line) {
        this.ui.text(line, x, yy, { size, color });
        line = w;
        yy += lh;
      } else line = test;
    }
    if (line) this.ui.text(line, x, yy, { size, color });
    return yy + lh;
  }

  // ------------------------------------------------------------------ table

  private heroSeatNo(): number {
    const i = this.s.seats.findIndex((x) => x?.isHero);
    return i < 0 ? 0 : i;
  }

  private slotPos(seat: number): { x: number; y: number; a: number } {
    const slot = (seat - this.heroSeatNo() + 9) % 9;
    const a = ((90 + slot * 40) * Math.PI) / 180;
    return { x: TABLE.cx + (TABLE.rx + 50) * Math.cos(a), y: TABLE.cy + (TABLE.ry + 64) * Math.sin(a), a };
  }

  private betPos(seat: number): { x: number; y: number } {
    const p = this.slotPos(seat);
    return { x: TABLE.cx + (p.x - TABLE.cx) * 0.56, y: TABLE.cy + (p.y - TABLE.cy) * 0.5 };
  }

  private table(now: number): void {
    const ui = this.ui;
    const c = ui.ctx;
    const s = this.s;
    const t = s.t;
    if (!t) return;

    // Felt and rail.
    c.save();
    c.beginPath();
    c.ellipse(TABLE.cx, TABLE.cy + 10, TABLE.rx + 34, TABLE.ry + 34, 0, 0, Math.PI * 2);
    c.fillStyle = 'rgba(0,0,0,0.5)';
    c.fill();
    c.beginPath();
    c.ellipse(TABLE.cx, TABLE.cy, TABLE.rx + 30, TABLE.ry + 30, 0, 0, Math.PI * 2);
    const rail = c.createLinearGradient(0, TABLE.cy - TABLE.ry, 0, TABLE.cy + TABLE.ry);
    rail.addColorStop(0, '#2a3342');
    rail.addColorStop(1, '#10151d');
    c.fillStyle = rail;
    c.fill();
    c.beginPath();
    c.ellipse(TABLE.cx, TABLE.cy, TABLE.rx, TABLE.ry, 0, 0, Math.PI * 2);
    const felt = c.createRadialGradient(TABLE.cx, TABLE.cy - 40, 30, TABLE.cx, TABLE.cy, TABLE.rx);
    felt.addColorStop(0, '#15875f');
    felt.addColorStop(0.7, C.felt);
    felt.addColorStop(1, C.feltDark);
    c.fillStyle = felt;
    c.fill();
    c.strokeStyle = 'rgba(255,255,255,0.08)';
    c.lineWidth = 2;
    c.beginPath();
    c.ellipse(TABLE.cx, TABLE.cy, TABLE.rx - 40, TABLE.ry - 34, 0, 0, Math.PI * 2);
    c.stroke();
    c.restore();
    c.globalAlpha = 0.14;
    this.logo(TABLE.cx - 95, TABLE.cy + 88, 1.25);
    c.globalAlpha = 1;

    // Pot and board.
    if (s.pot > 0) {
      drawChipStack(c, s.pot, TABLE.cx, TABLE.cy - 75, 1);
      ui.rrect({ x: TABLE.cx - 80, y: TABLE.cy - 128, w: 160, h: 30 }, 15, 'rgba(0,0,0,0.45)');
      ui.text(`Pot ${chips(s.pot)}`, TABLE.cx, TABLE.cy - 107, { size: 17, weight: 700, align: 'center' });
    }
    const bw = 84;
    const bh = 118;
    const bx0 = TABLE.cx - (5 * bw + 4 * 10) / 2;
    const heroSeat = s.seats.find((x) => x?.isHero);
    const winnerCards = this.winningCards();
    s.board.forEach((card, i) => {
      const at = s.boardShownAt[i] ?? 0;
      if (now < at) return;
      const flip = Math.min(1, (now - at) / 0.25);
      drawCard(c, card, bx0 + i * (bw + 10), TABLE.cy - 45, bw, bh, { flip, highlight: winnerCards.has(card), dim: winnerCards.size > 0 && !winnerCards.has(card) });
    });

    // Dealer button.
    const bp = this.slotPos(s.buttonSeat);
    const dbx = TABLE.cx + (bp.x - TABLE.cx) * 0.74 + 34;
    const dby = TABLE.cy + (bp.y - TABLE.cy) * 0.7;
    c.beginPath();
    c.arc(dbx, dby, 15, 0, Math.PI * 2);
    c.fillStyle = '#f4f4f4';
    c.fill();
    ui.text('D', dbx, dby + 1, { size: 16, weight: 800, color: '#111', align: 'center', baseline: 'middle' });

    // Bets.
    for (const seat of s.seats) {
      if (!seat || seat.bet <= 0) continue;
      const p = this.betPos(seat.seat);
      drawChipStack(c, seat.bet, p.x, p.y, 0.85);
      ui.text(chips(seat.bet), p.x, p.y + 30, { size: 15, weight: 700, align: 'center', color: '#fff' });
    }

    // Seats.
    for (const seat of s.seats) if (seat) this.drawSeat(seat, now);
    // Empty seats.
    s.seats.forEach((seat, i) => {
      if (seat) return;
      const p = this.slotPos(i);
      ui.rrect({ x: p.x - 60, y: p.y - 22, w: 120, h: 44 }, 22, 'rgba(255,255,255,0.03)', 'rgba(255,255,255,0.08)');
      ui.text('Empty', p.x, p.y + 6, { size: 14, color: C.dim, align: 'center' });
    });

    // Flights.
    for (const f of s.flights) this.drawFlight(f, now);

    // Hero hand strength and pot odds.
    if (heroSeat && heroSeat.hole.length === 2 && !heroSeat.folded && s.board.length >= 3) {
      const shown = s.board.filter((_, i) => now >= (s.boardShownAt[i] ?? 0));
      if (shown.length >= 3) {
        const label = describe(evaluateHand(heroSeat.hole, shown));
        const hp = this.slotPos(heroSeat.seat);
        ui.rrect({ x: hp.x - 110, y: hp.y - 190, w: 220, h: 28 }, 14, 'rgba(0,0,0,0.55)');
        ui.text(label, hp.x, hp.y - 170, { size: 15, weight: 700, color: C.gold, align: 'center' });
      }
    }

    this.gradeBadges(now);
    this.controls(now);
    this.sidePanel(now);
    this.overlays(now);
  }

  private winningCards(): Set<Card> {
    const s = this.s;
    const out = new Set<Card>();
    if (!s.hand?.complete || s.board.length < 5) return out;
    const winner = s.seats.find((x) => x?.winner && x.handLabel);
    if (!winner) return out;
    // Highlight the five cards that make the winning hand.
    const all = [...winner.hole, ...s.board];
    const target = evaluateHand(winner.hole, s.board);
    for (let a = 0; a < 7; a++)
      for (let b = a + 1; b < 7; b++) {
        const five = all.filter((_, i) => i !== a && i !== b);
        if (evaluateHand(five.slice(0, 2), five.slice(2)) === target) {
          five.forEach((c2) => out.add(c2));
          return out;
        }
      }
    return out;
  }

  private drawSeat(seat: SeatVis, now: number): void {
    const ui = this.ui;
    const c = ui.ctx;
    const s = this.s;
    const p = this.slotPos(seat.seat);
    const isHero = seat.isHero;
    const pw = isHero ? 230 : 190;
    const ph = 64;
    const plate: Rect = { x: p.x - pw / 2, y: p.y - ph / 2, w: pw, h: ph };
    const dim = seat.folded && !seat.winner;

    // Cards.
    if (seat.hasCards && !seat.folded) {
      const dealt = now - seat.dealtAt;
      if (isHero && seat.hole.length === 2) {
        const cw = 80;
        const ch = 112;
        seat.hole.forEach((card, i) => {
          const show = Math.min(1, Math.max(0, (dealt - 0.35 - i * 0.1) / 0.25));
          drawCard(c, card, p.x - cw - 4 + i * (cw + 8), p.y - ph / 2 - ch + 16, cw, ch, { flip: show, rotate: (i - 0.5) * 0.06, highlight: seat.winner });
        });
      } else if (seat.hole.length === 2) {
        const cw = 58;
        const ch = 82;
        seat.hole.forEach((card, i) => drawCard(c, card, p.x - cw - 2 + i * (cw + 4), p.y - ph / 2 - ch + 14, cw, ch, { flip: 1, highlight: seat.winner }));
      } else if (dealt > 0.4) {
        const cw = 44;
        const ch = 62;
        for (let i = 0; i < 2; i++) drawCard(c, null, p.x - 30 + i * 18, p.y - ph / 2 - ch + 18, cw, ch, { rotate: (i - 0.5) * 0.15 });
      }
    } else if (isHero && seat.folded && s.autoFolded && now - s.autoFolded.at < 1.5) {
      s.autoFolded.cards.forEach((card, i) => drawCard(c, card, p.x - 84 + i * 88, p.y - ph / 2 - 100, 80, 112, { alpha: 1 - (now - s.autoFolded!.at) / 1.5, dim: true }));
    }

    // Plate.
    c.globalAlpha = dim ? 0.55 : 1;
    if (seat.acting) {
      ui.rrect({ x: plate.x - 4, y: plate.y - 4, w: plate.w + 8, h: plate.h + 8 }, 16, undefined, C.accent, 2.5);
    }
    if (seat.winner && s.hand?.complete) {
      c.save();
      c.shadowColor = 'rgba(242,193,78,0.9)';
      c.shadowBlur = 24;
      ui.rrect(plate, 14, '#2d2410');
      c.restore();
    }
    const plateBg = c.createLinearGradient(0, plate.y, 0, plate.y + plate.h);
    plateBg.addColorStop(0, isHero ? '#1b3552' : '#1a2536');
    plateBg.addColorStop(1, isHero ? '#10223a' : '#0f1726');
    ui.rrect(plate, 14, plateBg, seat.isRival ? '#b8324a' : seat.winner && s.hand?.complete ? C.gold : 'rgba(255,255,255,0.1)', seat.isRival ? 2 : 1);
    // Avatar.
    c.beginPath();
    c.arc(plate.x + 32, p.y, 22, 0, Math.PI * 2);
    c.fillStyle = seat.isRival ? '#2a0d14' : hashColor(seat.name);
    c.fill();
    ui.text(seat.isRival ? '👻' : seat.name.slice(0, 2).toUpperCase(), plate.x + 32, p.y + 1, { size: seat.isRival ? 20 : 16, weight: 800, align: 'center', baseline: 'middle', color: '#fff' });
    ui.text(seat.name, plate.x + 62, p.y - 6, { size: 16, weight: 700, color: seat.isRival ? '#ff8da0' : C.ink, maxWidth: pw - 72 });
    const bb = s.hand?.bigBlind ?? 1;
    const stackText = seat.stack <= 0 && seat.allIn ? 'ALL-IN' : chips(seat.stack);
    ui.text(stackText, plate.x + 62, p.y + 19, { size: 16, weight: 700, color: seat.allIn ? C.red : C.gold, font: MONO });
    if (seat.stack > 0) ui.text(`${(seat.stack / bb).toFixed(seat.stack / bb < 10 ? 1 : 0)} BB`, plate.x + pw - 12, p.y + 19, { size: 13, color: C.muted, align: 'right' });
    c.globalAlpha = 1;

    // Timer bar.
    if (seat.acting) {
      const total = seat.actEnd - seat.actStart;
      const left = Math.max(0, seat.actEnd - now);
      const frac = total > 0 ? left / total : 0;
      ui.rrect({ x: plate.x + 8, y: plate.y + plate.h + 5, w: plate.w - 16, h: 6 }, 3, 'rgba(0,0,0,0.5)');
      const col = isHero && s.prompt && s.prompt.timeBankUntil > 0 ? C.orange : frac < 0.25 ? C.red : C.accent;
      let f = frac;
      if (isHero && s.prompt && s.prompt.timeBankUntil > 0) f = Math.max(0, (s.prompt.timeBankUntil - now) / Math.max(1, s.timeBank));
      ui.rrect({ x: plate.x + 8, y: plate.y + plate.h + 5, w: (plate.w - 16) * f, h: 6 }, 3, col);
    }

    // Last action tag.
    if (seat.lastAction && now - seat.lastActionAt < 30) {
      const tag = seat.lastAction;
      const col = tag === 'Fold' ? C.dim : tag === 'Check' ? '#5f7fa6' : tag.startsWith('Call') ? C.accent2 : tag === 'All-in' ? C.red : C.orange;
      const tw = ui.measure(tag, 14, 800) + 20;
      const ty = plate.y + plate.h + (seat.acting ? 16 : 8);
      ui.rrect({ x: p.x - tw / 2, y: ty, w: tw, h: 24 }, 12, col);
      ui.text(tag, p.x, ty + 17, { size: 14, weight: 800, align: 'center', color: '#fff' });
    }

    // All-in equity (broadcast style).
    if (seat.equity !== null && !seat.folded && s.hand && !s.hand.complete) {
      const eq = seat.equity;
      const ey = plate.y - 28;
      const ex = p.x + (isHero ? 128 : 100);
      ui.rrect({ x: ex - 36, y: ey - 18, w: 72, h: 30 }, 8, 'rgba(0,0,0,0.7)', eq >= 0.5 ? C.green : C.red, 1.5);
      ui.text(`${Math.round(eq * 100)}%`, ex, ey + 4, { size: 17, weight: 800, align: 'center', color: eq >= 0.5 ? C.green : '#ff9aa4', font: MONO });
    }
    if (seat.handLabel && s.hand?.complete && !seat.folded) {
      const tw = ui.measure(seat.handLabel, 13, 700) + 16;
      ui.rrect({ x: p.x - tw / 2, y: plate.y - 20, w: tw, h: 22 }, 11, seat.winner ? C.gold : 'rgba(0,0,0,0.7)');
      ui.text(seat.handLabel, p.x, plate.y - 4, { size: 13, weight: 700, align: 'center', color: seat.winner ? '#231704' : C.ink });
    }

    // HUD.
    if (s.hud && !isHero && s.t) {
      const pl = s.t.players.get(seat.id);
      if (pl && pl.hands >= 3) {
        const vp = Math.round((pl.vpipHands / pl.hands) * 100);
        const pf = Math.round((pl.pfrHands / pl.hands) * 100);
        const hud = `${vp}/${pf} · ${pl.hands}h`;
        const col = vp >= 45 ? '#ff9aa4' : vp <= 14 ? '#8fb7ff' : C.muted;
        ui.text(hud, p.x, plate.y - (seat.hasCards && !seat.folded ? 72 : 8), { size: 13, weight: 700, align: 'center', color: col, font: MONO });
      }
    }
  }

  private flightPoint(end: Flight['from'] | Flight['to'], seatNo?: number): { x: number; y: number } {
    if ('pot' in end && end.pot) return { x: TABLE.cx, y: TABLE.cy - 75 };
    if ('deck' in end && end.deck) return { x: TABLE.cx, y: TABLE.cy - 160 };
    if ('muck' in end && end.muck) return { x: TABLE.cx, y: TABLE.cy - 20 };
    if ('bet' in end && end.bet !== undefined) return this.betPos(end.bet);
    const seat = (end as { seat?: number }).seat ?? seatNo ?? 0;
    const p = this.slotPos(seat);
    return { x: p.x, y: p.y - 30 };
  }

  private drawFlight(f: Flight, now: number): void {
    const t = (now - f.start) / f.dur;
    if (t < 0 || t > 1) return;
    const k = ease.outCubic(t);
    const a = this.flightPoint(f.from);
    const b = this.flightPoint(f.to);
    const x = a.x + (b.x - a.x) * k;
    const y = a.y + (b.y - a.y) * k - Math.sin(k * Math.PI) * 20;
    const c = this.ui.ctx;
    if (f.kind === 'chips') drawChipStack(c, f.amount, x, y, 0.75, 1 - Math.max(0, t - 0.85) * 5);
    else drawCard(c, null, x - 20, y - 28, 40, 56, { rotate: k * 3, alpha: 1 - Math.max(0, t - 0.8) * 4 });
  }

  private gradeBadges(now: number): void {
    const s = this.s;
    const hero = s.seats.find((x) => x?.isHero);
    if (!hero) return;
    const p = this.slotPos(hero.seat);
    s.badges.forEach((b, i) => {
      const age = now - b.at;
      const st = GRADE_STYLE[b.grade.grade];
      const alpha = Math.min(1, age * 5) * (1 - Math.max(0, age - 2.6) / 0.6);
      const y = p.y - 70 - i * 6 - ease.outCubic(age * 2) * 30;
      const c = this.ui.ctx;
      c.globalAlpha = Math.max(0, alpha);
      const label = `${st.icon} ${b.grade.grade}`;
      const w = this.ui.measure(label, 17, 800) + 28;
      const x = p.x + 150;
      this.ui.rrect({ x, y: y - 20, w, h: 34 }, 17, st.color);
      this.ui.text(label, x + w / 2, y + 3, { size: 17, weight: 800, align: 'center', baseline: 'middle', color: '#08101b' });
      c.globalAlpha = 1;
    });
    // Explanation of the latest graded decision under the action area.
    const last = s.grades[s.grades.length - 1];
    const lastBadge = s.badges[s.badges.length - 1];
    if (last && lastBadge && now - lastBadge.at < 3.2 && !s.prompt) {
      const st = GRADE_STYLE[last.grade];
      const lines = this.wrapLines(last.note, 520, 16);
      lines.slice(0, 2).forEach((l, i) => this.ui.text(l, 1170, 930 + i * 24, { size: 16, weight: 600, color: st.color, align: 'right' }));
    }
  }

  // ------------------------------------------------------------------ controls

  /** Keyboard shortcuts from main.ts. */
  key(k: string): void {
    const s = this.s;
    const p = s.prompt;
    if (!p) return;
    const lower = k.toLowerCase();
    if (lower === 'f') s.heroAct({ type: 'fold' });
    else if (lower === 'c' || lower === 'x') s.heroAct(p.canCheck ? { type: 'check' } : { type: 'call' });
    else if (lower === 'r' || lower === 'b') s.heroAct({ type: 'raise', to: p.raiseTo });
    else if (lower === 'a') s.heroAct({ type: 'raise', to: p.maxRaise });
    else if (k === 'ArrowUp') p.raiseTo = Math.min(p.maxRaise, p.raiseTo + p.bigBlind);
    else if (k === 'ArrowDown') p.raiseTo = Math.max(p.minRaise, p.raiseTo - p.bigBlind);
    this.keyRaise++;
  }

  private controls(now: number): void {
    const ui = this.ui;
    const s = this.s;
    const p = s.prompt;
    const ax = 640;
    const aw = 1170 - ax;

    // Pace and HUD (bottom-left).
    const paces: [typeof s.pace, string][] = [['full', 'Full'], ['smart', 'Smart'], ['sprint', 'Sprint']];
    ui.text('Pace', 24, 900, { size: 13, weight: 700, color: C.muted });
    paces.forEach(([k, label], i) => {
      const r: Rect = { x: 24 + i * 96, y: 910, w: 92, h: 38 };
      const st = ui.clickable(`pace${k}`, r);
      const active = s.pace === k;
      ui.rrect(r, 9, active ? '#1f4d63' : st.hover ? '#172a42' : C.panel, active ? C.accent : C.line);
      ui.text(label, r.x + r.w / 2, r.y + 25, { size: 15, weight: 700, align: 'center', color: active ? C.ink : C.muted });
      if (st.clicked && !active) {
        s.pace = k;
        if (k === 'sprint') s.system('Sprint: the game will play your hands (solid TAG style) until the bubble or the final table.');
      }
    });
    const hudR: Rect = { x: 24, y: 958, w: 130, h: 30 };
    const hs = ui.clickable('hud', hudR);
    if (hs.clicked) s.hud = !s.hud;
    ui.rrect(hudR, 8, s.hud ? '#16354a' : C.panel, s.hud ? C.accent : C.line);
    ui.text(`HUD ${s.hud ? 'ON' : 'OFF'}`, hudR.x + hudR.w / 2, hudR.y + 21, { size: 13, weight: 700, align: 'center', color: s.hud ? C.ink : C.muted });
    const leanR: Rect = { x: 164, y: 958, w: 148, h: 30 };
    if (ui.clickable('lean', leanR).clicked) this.onLeanBack?.();
    ui.rrect(leanR, 8, C.panel, C.line);
    ui.text('Lean back ␣', leanR.x + leanR.w / 2, leanR.y + 21, { size: 13, weight: 700, align: 'center', color: C.muted });

    // Composure meter.
    const tilt = s.heroTilt;
    ui.text('Composure', 24, 858, { size: 13, weight: 700, color: C.muted });
    ui.rrect({ x: 112, y: 848, w: 170, h: 10 }, 5, '#0b1422', C.line);
    const comp = 1 - tilt;
    ui.rrect({ x: 112, y: 848, w: 170 * comp, h: 10 }, 5, comp > 0.6 ? C.green : comp > 0.35 ? C.gold : C.red);
    if (tilt > 0.3 && !s.sitOutNext) {
      if (ui.button('sitout', { x: 24, y: 868, w: 258, h: 26 }, 'Step away: sit out next hand', { kind: 'ghost', size: 13 })) s.requestSitOut();
    }

    if (!p) {
      // Waiting state.
      if (s.hand && !s.hand.complete && !s.sprinting) {
        const toAct = s.hand.toAct !== null ? s.hand.seats[s.hand.toAct] : null;
        if (toAct && toAct.id !== HERO_ID) {
          ui.text(`Waiting for ${s.seats[toAct.seat]?.name ?? '…'}`, 1170, 940, { size: 15, color: C.dim, align: 'right' });
        }
      }
      return;
    }

    // Bet sizing.
    if (p.canRaise) {
      const presets: [string, number][] = p.street === 'preflop'
        ? [['Min', p.minRaise], ['2.5x', Math.round((s.hand?.currentBet ?? p.bigBlind) * 2.5)], ['3x', Math.round((s.hand?.currentBet ?? p.bigBlind) * 3)], ['Pot', this.potRaise(p)], ['All-in', p.maxRaise]]
        : [['⅓', this.potFrac(p, 1 / 3)], ['½', this.potFrac(p, 0.5)], ['¾', this.potFrac(p, 0.75)], ['Pot', this.potRaise(p)], ['All-in', p.maxRaise]];
      presets.forEach(([label, v], i) => {
        const r: Rect = { x: ax + i * 72, y: 800, w: 66, h: 34 };
        const val = Math.max(p.minRaise, Math.min(p.maxRaise, v));
        if (ui.button(`pre${i}`, r, label, { kind: p.raiseTo === val ? 'primary' : 'secondary', size: 15 })) p.raiseTo = val;
      });
      const box: Rect = { x: ax + 370, y: 800, w: aw - 370, h: 34 };
      ui.rrect(box, 8, '#0b1422', C.line);
      ui.text(chips(p.raiseTo), box.x + box.w - 12, box.y + 24, { size: 18, weight: 700, align: 'right', font: MONO });
      ui.text(`${(p.raiseTo / p.bigBlind).toFixed(1)} BB`, box.x + 10, box.y + 23, { size: 13, color: C.muted });
      p.raiseTo = ui.slider('raise', { x: ax + 10, y: 842, w: aw - 20, h: 26 }, p.raiseTo, p.minRaise, p.maxRaise, Math.max(1, Math.round(p.bigBlind / 4)));
    }

    // Main buttons.
    const bw = (aw - 20) / 3;
    const by = 880;
    if (ui.button('fold', { x: ax, y: by, w: bw, h: 72 }, 'Fold', { kind: 'danger', size: 22, hotkey: 'F' })) s.heroAct({ type: 'fold' });
    const callLabel = p.canCheck ? 'Check' : p.toCall >= (s.hand?.seats.find((x) => x.id === HERO_ID)?.stack ?? Infinity) ? 'Call all-in' : 'Call';
    if (ui.button('call', { x: ax + bw + 10, y: by, w: bw, h: 72 }, callLabel, { kind: 'secondary', size: 22, sub: p.canCheck ? undefined : chips(p.toCall), hotkey: 'C' })) {
      s.heroAct(p.canCheck ? { type: 'check' } : { type: 'call' });
    }
    if (p.canRaise) {
      const allIn = p.raiseTo >= p.maxRaise;
      const lbl = allIn ? 'All-in' : p.isBet ? 'Bet' : 'Raise to';
      if (ui.button('raise', { x: ax + (bw + 10) * 2, y: by, w: bw, h: 72 }, lbl, { kind: allIn ? 'gold' : 'primary', size: 22, sub: chips(p.raiseTo), hotkey: 'R' })) {
        s.heroAct({ type: 'raise', to: p.raiseTo });
      }
    }
    // Pot odds readout (Math skill, level 1).
    if (!p.canCheck && p.toCall > 0) {
      const odds = p.toCall / (p.pot + p.toCall);
      ui.text(`Call ${chips(p.toCall)} to win ${chips(p.pot)} · you need ${Math.round(odds * 100)}% equity`, 1170, 976, { size: 15, weight: 600, color: C.muted, align: 'right' });
    }
    void now;
  }

  private potFrac(p: NonNullable<Session['prompt']>, f: number): number {
    const cur = this.s.hand?.currentBet ?? 0;
    return Math.round(cur + (p.pot + p.toCall) * f);
  }

  private potRaise(p: NonNullable<Session['prompt']>): number {
    const cur = this.s.hand?.currentBet ?? 0;
    return Math.round(cur + p.pot + p.toCall);
  }

  // ------------------------------------------------------------------ side panel

  private sidePanel(now: number): void {
    const ui = this.ui;
    const s = this.s;
    const t = s.t!;
    const x = SIDE_X;
    const w = W - x - 16;
    const card: Rect = { x, y: TOP + 14, w, h: 300 };
    ui.rrect(card, 14, C.panel, C.line);
    ui.text(t.spec.name, x + 18, card.y + 34, { size: 19, weight: 800, maxWidth: w - 120 });
    ui.text(`Table ${s.tableId}`, x + w - 18, card.y + 34, { size: 15, weight: 700, color: C.muted, align: 'right' });
    const lvl = t.level;
    const secs = Math.max(0, Math.round(t.levelSecondsLeft));
    const flash = now - s.lastLevelUpAt < 2 ? C.gold : C.ink;
    ui.text(`Level ${t.levelIndex + 1}`, x + 18, card.y + 68, { size: 15, color: C.muted });
    ui.text(`${chips(lvl.sb)}/${chips(lvl.bb)} · ante ${chips(lvl.ante)}`, x + w - 18, card.y + 68, { size: 17, weight: 700, color: flash, align: 'right', font: MONO });
    ui.text('Next level', x + 18, card.y + 96, { size: 15, color: C.muted });
    ui.text(`${Math.floor(secs / 60)}:${String(secs % 60).padStart(2, '0')} → ${chips(t.nextLevel.sb)}/${chips(t.nextLevel.bb)}`, x + w - 18, card.y + 96, { size: 15, weight: 600, align: 'right', font: MONO });
    const rows: [string, string, string?][] = [
      ['Players left', `${chips(t.remaining)} / ${chips(t.spec.entrants)}`],
      ['Average stack', `${chips(Math.round(t.averageStack))} (${Math.round(t.averageStack / lvl.bb)} BB)`],
      ['Your rank', t.hero.busted ? '—' : `${ordinal(t.heroRank())} of ${chips(t.remaining)}`],
      [
        t.inTheMoney ? 'In the money' : 'To the money',
        t.inTheMoney ? `next jump ${money(t.prizeFor(t.remaining - 1) || t.prizeFor(1))}` : `${chips(t.remaining - t.paidPlaces)} players`,
        t.inTheMoney ? C.green : t.handForHand ? C.orange : undefined,
      ],
      ['Min cash / 1st', `${money(t.payouts[t.payouts.length - 1])} / ${money(t.payouts[0])}`],
    ];
    rows.forEach(([k, v, col], i) => {
      const y = card.y + 134 + i * 32;
      ui.text(k, x + 18, y, { size: 15, color: C.muted });
      ui.text(v, x + w - 18, y, { size: 15, weight: 700, align: 'right', color: col ?? C.ink });
    });

    // Tabs.
    const tabs: [typeof s.rightTab, string][] = [['chat', 'Chat'], ['payouts', 'Payouts'], ['info', 'Standings']];
    const ty = card.y + card.h + 14;
    tabs.forEach(([k, label], i) => {
      const r: Rect = { x: x + i * (w / 3), y: ty, w: w / 3 - 6, h: 36 };
      const st = ui.clickable(`tab${k}`, r);
      if (st.clicked) s.rightTab = k;
      const active = s.rightTab === k;
      ui.rrect(r, 9, active ? C.panel2 : 'transparent', active ? C.line : undefined);
      ui.text(label, r.x + r.w / 2, r.y + 24, { size: 15, weight: 700, align: 'center', color: active ? C.ink : C.muted });
    });
    const box: Rect = { x, y: ty + 44, w, h: H - (ty + 44) - 16 };
    ui.rrect(box, 14, '#0c1524', C.line);
    const c = ui.ctx;
    c.save();
    c.beginPath();
    c.roundRect(box.x, box.y, box.w, box.h, 14);
    c.clip();
    if (s.rightTab === 'chat') {
      const lines: { who: string; text: string; color: string; whoColor: string }[] = [];
      for (const l of s.chat) {
        const color = l.kind === 'dealer' ? '#8fa0bb' : l.kind === 'system' ? C.gold : l.kind === 'rival' ? '#ff8da0' : C.ink;
        const whoColor = l.kind === 'rival' ? '#ff5c7a' : l.kind === 'dealer' ? '#5f6f88' : C.accent;
        lines.push({ who: l.who, text: l.text, color, whoColor });
      }
      // Wrap from the bottom up.
      let y = box.y + box.h - 14;
      for (let i = lines.length - 1; i >= 0 && y > box.y + 10; i--) {
        const l = lines[i];
        const prefix = l.who ? `${l.who}: ` : '';
        const wrapped = this.wrapLines(prefix + l.text, box.w - 28, 14);
        for (let j = wrapped.length - 1; j >= 0 && y > box.y + 10; j--) {
          if (j === 0 && prefix) {
            const pw = ui.text(prefix, box.x + 14, y, { size: 14, weight: 700, color: l.whoColor });
            ui.text(wrapped[0].slice(prefix.length), box.x + 14 + pw, y, { size: 14, color: l.color });
          } else ui.text(wrapped[j], box.x + 14, y, { size: 14, color: l.color });
          y -= 20;
        }
      }
    } else if (s.rightTab === 'payouts') {
      const bands = payoutBands(t.payouts.length);
      let y = box.y + 30;
      for (const [a, b] of bands) {
        if (y > box.y + box.h - 10) break;
        const label = a === b ? ordinal(a) : `${ordinal(a)}–${ordinal(b)}`;
        const reached = t.remaining <= b;
        ui.text(label, box.x + 16, y, { size: 14, color: reached ? C.ink : C.muted });
        ui.text(money(t.payouts[a - 1]), box.x + box.w - 16, y, { size: 14, weight: 700, align: 'right', color: reached ? C.gold : C.muted, font: MONO });
        y += 22;
      }
    } else {
      const st = t.standings();
      const heroRank = t.heroRank();
      let y = box.y + 30;
      st.slice(0, 12).forEach((pl, i) => {
        ui.text(`${i + 1}.`, box.x + 16, y, { size: 14, color: C.muted, font: MONO });
        ui.text(pl.name, box.x + 56, y, { size: 14, weight: pl.isHero ? 800 : 500, color: pl.isHero ? C.accent : pl.name === 'gh0stfold' ? '#ff8da0' : C.ink, maxWidth: 190 });
        ui.text(chips(pl.stack), box.x + box.w - 16, y, { size: 14, weight: 700, align: 'right', font: MONO });
        y += 22;
      });
      if (!t.hero.busted && heroRank > 12) {
        y += 8;
        ui.text(`${heroRank}.`, box.x + 16, y, { size: 14, color: C.muted, font: MONO });
        ui.text(t.hero.name, box.x + 56, y, { size: 14, weight: 800, color: C.accent });
        ui.text(chips(t.hero.stack), box.x + box.w - 16, y, { size: 14, weight: 700, align: 'right', font: MONO });
      }
    }
    c.restore();
  }

  private wrapLines(text: string, maxW: number, size: number): string[] {
    const words = text.split(' ');
    const out: string[] = [];
    let line = '';
    for (const w of words) {
      const test = line ? `${line} ${w}` : w;
      if (this.ui.measure(test, size) > maxW && line) {
        out.push(line);
        line = w;
      } else line = test;
    }
    if (line) out.push(line);
    return out;
  }

  // ------------------------------------------------------------------ overlays

  private overlays(now: number): void {
    const ui = this.ui;
    const s = this.s;
    const c = ui.ctx;
    if (s.moving) {
      const age = now - s.moving.at;
      const a = Math.min(1, age * 4) * (1 - Math.max(0, age - 1.6) / 0.6);
      c.globalAlpha = Math.max(0, a);
      ui.rrect({ x: TABLE.cx - 230, y: TABLE.cy - 70, w: 460, h: 110 }, 16, 'rgba(8,16,27,0.94)', C.accent2);
      ui.text('Table balancing', TABLE.cx, TABLE.cy - 30, { size: 17, color: C.muted, align: 'center' });
      ui.text(`Table ${s.moving.from}  →  Table ${s.moving.to}`, TABLE.cx, TABLE.cy + 12, { size: 28, weight: 800, align: 'center' });
      c.globalAlpha = 1;
    }
    if (s.banner) {
      const age = now - s.banner.at;
      const a = Math.min(1, age * 3) * (1 - Math.max(0, age - 2.6) / 0.6);
      const sc = 0.9 + 0.1 * ease.outBack(Math.min(1, age * 2.5));
      c.save();
      c.globalAlpha = Math.max(0, a);
      c.translate(TABLE.cx, TABLE.cy - 10);
      c.scale(sc, sc);
      ui.rrect({ x: -330, y: -80, w: 660, h: 150 }, 20, 'rgba(6,10,18,0.92)', s.banner.color, 2);
      ui.text(s.banner.title, 0, -12, { size: 42, weight: 900, align: 'center', color: s.banner.color });
      ui.text(s.banner.sub, 0, 36, { size: 20, weight: 600, align: 'center', color: C.ink });
      c.restore();
    }
    if (s.sprinting && s.t) {
      const t = s.t;
      c.fillStyle = 'rgba(5,9,16,0.78)';
      c.fillRect(0, TOP, SIDE_X - 8, H - TOP);
      ui.text('SPRINTING AHEAD', TABLE.cx, 330, { size: 40, weight: 900, align: 'center', color: C.accent });
      ui.text('The game is playing your hands in a solid, tight-aggressive style.', TABLE.cx, 372, { size: 17, color: C.muted, align: 'center' });
      const stats: [string, string][] = [
        ['Level', `${t.levelIndex + 1} · ${chips(t.level.sb)}/${chips(t.level.bb)}`],
        ['Players left', chips(t.remaining)],
        ['Your stack', `${chips(t.hero.stack)} (${Math.round(t.hero.stack / t.level.bb)} BB)`],
        ['Your rank', ordinal(t.heroRank())],
      ];
      stats.forEach(([k, v], i) => {
        const x = TABLE.cx - 330 + i * 220;
        ui.text(k, x + 100, 450, { size: 15, color: C.muted, align: 'center' });
        ui.text(v, x + 100, 482, { size: 21, weight: 800, align: 'center', font: MONO });
      });
      const bar = (now * 0.8) % 1;
      ui.rrect({ x: TABLE.cx - 300, y: 530, w: 600, h: 8 }, 4, '#0b1422');
      ui.rrect({ x: TABLE.cx - 300 + bar * 480, y: 530, w: 120, h: 8 }, 4, C.accent);
      if (ui.button('stopsprint', { x: TABLE.cx - 120, y: 580, w: 240, h: 60 }, 'Take the wheel', { kind: 'gold', size: 20 })) s.stopSprint('you took over');
    }
  }

  // ------------------------------------------------------------------ results

  private resultsScreen(now: number): void {
    const ui = this.ui;
    const s = this.s;
    const r = s.results;
    if (!r) return;
    const c = ui.ctx;
    const cashed = r.prizeCents > 0;
    const color = r.won ? C.gold : cashed ? C.green : C.red;
    ui.text(r.eventName, 60, TOP + 60, { size: 20, weight: 600, color: C.muted });
    ui.text(r.won ? 'Champion.' : `You finished ${ordinal(r.place)}`, 60, TOP + 130, { size: 60, weight: 900, color });
    ui.text(`of ${chips(r.entrants)} players`, 60, TOP + 172, { size: 22, color: C.muted });
    ui.text(cashed ? `+${money(r.prizeCents)}` : 'No cash this time', 60, TOP + 250, { size: 46, weight: 800, color: cashed ? C.gold : C.muted, font: MONO });
    ui.text(`Buy-in ${r.buyInCents ? money(r.buyInCents) : 'free'} · ${r.hands} hands · biggest pot won ${chips(r.biggestPot)}`, 60, TOP + 292, { size: 17, color: C.muted });
    ui.text(`Balance now ${money(s.bankrollCents)}`, 60, TOP + 330, { size: 20, weight: 700, color: C.ink });

    // Accuracy gauge.
    const gx = 1240;
    const gy = TOP + 200;
    const acc = r.accuracy;
    const shown = acc * ease.outCubic((now - s.resultsAt) / 1.4);
    c.lineWidth = 18;
    c.lineCap = 'round';
    c.beginPath();
    c.arc(gx, gy, 110, Math.PI * 0.75, Math.PI * 2.25);
    c.strokeStyle = '#13213a';
    c.stroke();
    c.beginPath();
    c.arc(gx, gy, 110, Math.PI * 0.75, Math.PI * 0.75 + (Math.PI * 1.5 * shown) / 100);
    c.strokeStyle = acc >= 85 ? C.green : acc >= 70 ? C.accent : acc >= 55 ? C.gold : C.red;
    c.stroke();
    c.lineCap = 'butt';
    ui.text(r.grades.length ? acc.toFixed(1) : '—', gx, gy + 14, { size: 52, weight: 900, align: 'center', font: MONO });
    ui.text('Decision accuracy', gx, gy + 60, { size: 16, color: C.muted, align: 'center' });
    const xp = Math.round(r.grades.length * (acc / 100) * 12 + (cashed ? 60 : 0));
    ui.text(`Skill XP +${xp}`, gx, gy + 150, { size: 22, weight: 800, align: 'center', color: C.accent });
    ui.text('XP comes from decisions, not results.', gx, gy + 178, { size: 14, color: C.muted, align: 'center' });

    // Grade distribution.
    const order = ['Best', 'Good', 'Inaccuracy', 'Mistake', 'Blunder'];
    const counts = order.map((g) => r.grades.filter((x) => x.grade === g).length);
    const total = Math.max(1, r.grades.length);
    let bx = 60;
    const by = TOP + 390;
    order.forEach((g, i) => {
      const w = (counts[i] / total) * 1000;
      if (w > 0) ui.rrect({ x: bx, y: by, w: Math.max(4, w - 3), h: 22 }, 5, GRADE_STYLE[g].color);
      bx += w;
    });
    order.forEach((g, i) => {
      ui.text(`${GRADE_STYLE[g].icon} ${g} ${counts[i]}`, 60 + i * 200, by + 52, { size: 16, weight: 700, color: GRADE_STYLE[g].color });
    });

    // Biggest mistakes.
    const worst = [...r.grades].filter((g) => g.grade !== 'Best' && g.grade !== 'Good').sort((a, b) => b.lossBB - a.lossBB).slice(0, 4);
    ui.text('Hand review', 60, by + 110, { size: 22, weight: 800 });
    if (worst.length === 0) ui.text(r.grades.length ? 'Clean game. No significant mistakes found.' : 'No decisions graded.', 60, by + 144, { size: 17, color: C.muted });
    worst.forEach((g, i) => {
      const y = by + 144 + i * 56;
      const st = GRADE_STYLE[g.grade];
      ui.rrect({ x: 60, y: y - 26, w: 1000, h: 48 }, 10, C.panel, C.line);
      ui.text(`${st.icon} ${g.grade}`, 78, y + 4, { size: 16, weight: 800, color: st.color });
      ui.text(`${g.street[0].toUpperCase() + g.street.slice(1)} · you chose ${g.chosen.label.toLowerCase()}`, 220, y + 4, { size: 16, weight: 600 });
      ui.text(g.note, 1045, y + 4, { size: 15, color: C.muted, align: 'right', maxWidth: 480 });
    });

    if (ui.button('backlobby', { x: 1120, y: H - 120, w: 420, h: 70 }, 'Back to lobby', { kind: 'primary', size: 22 })) s.leaveResults();
    void cardToString;
  }
}
