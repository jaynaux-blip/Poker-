import { Card, RANK_CHARS, rankOf, suitOf } from '../core/cards';
import { FONT } from './canvasui';

/**
 * Card faces and backs, pre-rendered once per size to offscreen canvases.
 * Four-color deck (spades black, hearts red, diamonds blue, clubs green) like
 * most online clients, for fast reading at a glance.
 */

const SUIT_GLYPH = ['♣', '♦', '♥', '♠'];
const SUIT_COLOR = ['#1f8f4e', '#2a6fdb', '#d8313f', '#1b1e24'];
const cache = new Map<string, HTMLCanvasElement>();
let artScale = 1;

/** Cached card art is rendered at this many pixels per logical pixel. */
export function setArtScale(s: number): void {
  artScale = Math.max(1, Math.ceil(s * 2) / 2);
}

function roundRectPath(c: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, r: number): void {
  c.beginPath();
  c.roundRect(x, y, w, h, r);
}

export function cardFace(card: Card, w: number, h: number): HTMLCanvasElement {
  const key = `f${card}:${w}x${h}@${artScale}`;
  const hit = cache.get(key);
  if (hit) return hit;
  const cv = document.createElement('canvas');
  const pad = 6;
  cv.width = Math.ceil((w + pad * 2) * artScale);
  cv.height = Math.ceil((h + pad * 2) * artScale);
  const c = cv.getContext('2d')!;
  c.scale(artScale, artScale);
  c.translate(pad, pad);
  c.shadowColor = 'rgba(0,0,0,0.45)';
  c.shadowBlur = 6;
  c.shadowOffsetY = 2;
  roundRectPath(c, 0, 0, w, h, w * 0.1);
  const g = c.createLinearGradient(0, 0, 0, h);
  g.addColorStop(0, '#ffffff');
  g.addColorStop(1, '#e9edf3');
  c.fillStyle = g;
  c.fill();
  c.shadowColor = 'transparent';
  c.strokeStyle = 'rgba(0,0,0,0.18)';
  c.lineWidth = 1;
  c.stroke();
  const s = suitOf(card);
  const r = RANK_CHARS[rankOf(card)];
  const col = SUIT_COLOR[s];
  c.fillStyle = col;
  c.textAlign = 'left';
  c.textBaseline = 'top';
  c.font = `800 ${Math.round(h * 0.36)}px ${FONT}`;
  c.fillText(r === 'T' ? '10' : r, w * 0.09, h * 0.05);
  c.font = `${Math.round(h * 0.3)}px "Segoe UI Symbol", "DejaVu Sans", ${FONT}`;
  c.fillText(SUIT_GLYPH[s], w * 0.1, h * 0.42);
  c.textAlign = 'right';
  c.textBaseline = 'bottom';
  c.font = `${Math.round(h * 0.5)}px "Segoe UI Symbol", "DejaVu Sans", ${FONT}`;
  c.globalAlpha = 0.9;
  c.fillText(SUIT_GLYPH[s], w * 0.95, h * 0.98);
  cache.set(key, cv);
  return cv;
}

export function cardBack(w: number, h: number): HTMLCanvasElement {
  const key = `b:${w}x${h}@${artScale}`;
  const hit = cache.get(key);
  if (hit) return hit;
  const cv = document.createElement('canvas');
  const pad = 6;
  cv.width = Math.ceil((w + pad * 2) * artScale);
  cv.height = Math.ceil((h + pad * 2) * artScale);
  const c = cv.getContext('2d')!;
  c.scale(artScale, artScale);
  c.translate(pad, pad);
  c.shadowColor = 'rgba(0,0,0,0.45)';
  c.shadowBlur = 6;
  c.shadowOffsetY = 2;
  roundRectPath(c, 0, 0, w, h, w * 0.1);
  c.fillStyle = '#f4f6fa';
  c.fill();
  c.shadowColor = 'transparent';
  roundRectPath(c, w * 0.07, w * 0.07, w - w * 0.14, h - w * 0.14, w * 0.06);
  const g = c.createLinearGradient(0, 0, w, h);
  g.addColorStop(0, '#0e7c72');
  g.addColorStop(1, '#0b3f5c');
  c.fillStyle = g;
  c.fill();
  c.save();
  c.clip();
  c.strokeStyle = 'rgba(255,255,255,0.12)';
  c.lineWidth = 2;
  for (let i = -h; i < w + h; i += 9) {
    c.beginPath();
    c.moveTo(i, 0);
    c.lineTo(i + h, h);
    c.stroke();
  }
  c.restore();
  // River wave mark.
  c.strokeStyle = 'rgba(255,255,255,0.75)';
  c.lineWidth = Math.max(2, w * 0.05);
  c.beginPath();
  const cy = h / 2;
  for (let x = w * 0.25; x <= w * 0.75; x += 1) {
    const y = cy + Math.sin(((x - w * 0.25) / (w * 0.5)) * Math.PI * 2) * h * 0.06;
    if (x === w * 0.25) c.moveTo(x, y);
    else c.lineTo(x, y);
  }
  c.stroke();
  cache.set(key, cv);
  return cv;
}

/** Draw a card centered-top-left at (x, y); `flip` 0..1 animates a turn-over. */
export function drawCard(
  ctx: CanvasRenderingContext2D,
  card: Card | null,
  x: number,
  y: number,
  w: number,
  h: number,
  opts: { flip?: number; alpha?: number; rotate?: number; dim?: boolean; highlight?: boolean } = {},
): void {
  const flip = opts.flip ?? 1;
  const showFace = card !== null && flip >= 0.5;
  const sx = Math.abs(Math.cos(flip * Math.PI)) || 0.02;
  const img = showFace ? cardFace(card!, w, h) : cardBack(w, h);
  ctx.save();
  ctx.globalAlpha = opts.alpha ?? 1;
  ctx.translate(x + w / 2, y + h / 2);
  if (opts.rotate) ctx.rotate(opts.rotate);
  ctx.scale(sx, 1);
  if (opts.highlight) {
    ctx.shadowColor = 'rgba(242,193,78,0.95)';
    ctx.shadowBlur = 22;
  }
  ctx.drawImage(img, -w / 2 - 6, -h / 2 - 6, w + 12, h + 12);
  if (opts.dim) {
    ctx.fillStyle = 'rgba(8,12,20,0.55)';
    ctx.beginPath();
    ctx.roundRect(-w / 2, -h / 2, w, h, w * 0.1);
    ctx.fill();
  }
  ctx.restore();
}

/** Chip colors by denomination tier. */
export function chipColor(value: number): [string, string] {
  if (value >= 100000) return ['#e8e8ee', '#8c3cd6'];
  if (value >= 25000) return ['#f2c14e', '#6b4a10'];
  if (value >= 5000) return ['#ff6f91', '#7a1030'];
  if (value >= 1000) return ['#f5f5f5', '#222'];
  if (value >= 500) return ['#8c52ff', '#2a0f66'];
  if (value >= 100) return ['#1d1f26', '#e6e6e6'];
  if (value >= 25) return ['#2fbf71', '#0c3d22'];
  return ['#e84a5f', '#fff'];
}

/** A small stack of chips representing `amount` (for bets and pots). */
export function drawChipStack(ctx: CanvasRenderingContext2D, amount: number, x: number, y: number, scale = 1, alpha = 1): void {
  if (amount <= 0) return;
  const denoms = [100000, 25000, 5000, 1000, 500, 100, 25, 5];
  let rem = amount;
  const stacks: [number, number][] = [];
  for (const d of denoms) {
    const n = Math.floor(rem / d);
    if (n > 0) {
      stacks.push([d, Math.min(n, 8)]);
      rem -= n * d;
    }
    if (stacks.length >= 3) break;
  }
  if (stacks.length === 0) stacks.push([5, 1]);
  ctx.save();
  ctx.globalAlpha = alpha;
  const r = 13 * scale;
  stacks.forEach(([d, n], si) => {
    const sx = x + (si - (stacks.length - 1) / 2) * r * 2.1;
    const [face, edge] = chipColor(d);
    for (let i = 0; i < n; i++) {
      const sy = y - i * 3.2 * scale;
      ctx.beginPath();
      ctx.ellipse(sx, sy + 2 * scale, r, r * 0.55, 0, 0, Math.PI * 2);
      ctx.fillStyle = 'rgba(0,0,0,0.35)';
      ctx.fill();
      ctx.beginPath();
      ctx.ellipse(sx, sy, r, r * 0.55, 0, 0, Math.PI * 2);
      ctx.fillStyle = face;
      ctx.fill();
      ctx.strokeStyle = edge;
      ctx.lineWidth = 2.2 * scale;
      ctx.setLineDash([4 * scale, 4 * scale]);
      ctx.stroke();
      ctx.setLineDash([]);
    }
  });
  ctx.restore();
}
