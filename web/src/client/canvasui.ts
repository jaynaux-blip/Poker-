/**
 * Minimal immediate-mode UI for the in-world laptop screen. The client draws
 * into a 2D canvas every frame; widgets register hit rectangles and read the
 * pointer state that the 3D raycast feeds in.
 */

export const W = 1600;
export const H = 1000;

export const C = {
  bg: '#0a111c',
  bg2: '#0e1726',
  panel: '#111c2e',
  panel2: '#16233a',
  line: '#223352',
  ink: '#e6edf7',
  muted: '#7b8aa3',
  dim: '#4d5b73',
  accent: '#27d3c3',
  accent2: '#3b82f6',
  gold: '#f2c14e',
  red: '#ef4d5a',
  orange: '#f28a3a',
  green: '#3ecf6e',
  felt: '#0f6a4e',
  feltDark: '#083a2b',
  rail: '#151a22',
};

export const FONT = 'Inter, "Segoe UI", Roboto, Arial, sans-serif';
export const MONO = '"JetBrains Mono", Menlo, Consolas, monospace';

export interface Rect {
  x: number;
  y: number;
  w: number;
  h: number;
}

export class Pointer {
  x = -1;
  y = -1;
  down = false;
  pressed = false; // went down this frame
  released = false; // went up this frame
  wheel = 0;
  active = false; // pointer is over the screen

  endFrame(): void {
    this.pressed = false;
    this.released = false;
    this.wheel = 0;
  }
}

export class UI {
  readonly canvas: HTMLCanvasElement;
  readonly ctx: CanvasRenderingContext2D;
  readonly pointer = new Pointer();
  cursor: 'default' | 'pointer' = 'default';
  private pressedId: string | null = null;
  time = 0;

  constructor() {
    this.canvas = document.createElement('canvas');
    this.canvas.width = W;
    this.canvas.height = H;
    this.ctx = this.canvas.getContext('2d')!;
  }

  begin(time: number): void {
    this.time = time;
    this.cursor = 'default';
  }

  end(): void {
    if (this.pointer.released) this.pressedId = null;
    this.pointer.endFrame();
  }

  hover(r: Rect): boolean {
    const p = this.pointer;
    return p.active && p.x >= r.x && p.x <= r.x + r.w && p.y >= r.y && p.y <= r.y + r.h;
  }

  /** Returns true when clicked (press and release inside the same widget). */
  clickable(id: string, r: Rect, enabled = true): { hover: boolean; down: boolean; clicked: boolean } {
    const hover = enabled && this.hover(r);
    if (hover) this.cursor = 'pointer';
    if (hover && this.pointer.pressed) this.pressedId = id;
    const down = this.pressedId === id && this.pointer.down && hover;
    const clicked = enabled && hover && this.pointer.released && this.pressedId === id;
    return { hover, down, clicked };
  }

  // ---------------------------------------------------------------- drawing

  rrect(r: Rect, radius: number, fill?: string | CanvasGradient, stroke?: string, lw = 1): void {
    const c = this.ctx;
    c.beginPath();
    c.roundRect(r.x, r.y, r.w, r.h, radius);
    if (fill) {
      c.fillStyle = fill;
      c.fill();
    }
    if (stroke) {
      c.strokeStyle = stroke;
      c.lineWidth = lw;
      c.stroke();
    }
  }

  text(
    s: string,
    x: number,
    y: number,
    opts: { size?: number; weight?: number; color?: string; align?: CanvasTextAlign; baseline?: CanvasTextBaseline; font?: string; maxWidth?: number } = {},
  ): number {
    const c = this.ctx;
    c.font = `${opts.weight ?? 500} ${opts.size ?? 18}px ${opts.font ?? FONT}`;
    c.fillStyle = opts.color ?? C.ink;
    c.textAlign = opts.align ?? 'left';
    c.textBaseline = opts.baseline ?? 'alphabetic';
    let str = s;
    if (opts.maxWidth && c.measureText(str).width > opts.maxWidth) {
      while (str.length > 1 && c.measureText(str + '…').width > opts.maxWidth) str = str.slice(0, -1);
      str += '…';
    }
    c.fillText(str, x, y);
    return c.measureText(str).width;
  }

  measure(s: string, size: number, weight = 500, font = FONT): number {
    this.ctx.font = `${weight} ${size}px ${font}`;
    return this.ctx.measureText(s).width;
  }

  button(
    id: string,
    r: Rect,
    label: string,
    opts: { kind?: 'primary' | 'secondary' | 'danger' | 'ghost' | 'gold'; enabled?: boolean; size?: number; sub?: string; hotkey?: string } = {},
  ): boolean {
    const enabled = opts.enabled ?? true;
    const st = this.clickable(id, r, enabled);
    const kind = opts.kind ?? 'secondary';
    const palette: Record<string, [string, string, string]> = {
      primary: ['#1fb3a5', '#27d3c3', '#06201d'],
      secondary: ['#1a2940', '#233756', C.ink],
      danger: ['#7a2230', '#9b2b3c', '#ffe9ec'],
      ghost: ['rgba(255,255,255,0.02)', 'rgba(255,255,255,0.07)', C.ink],
      gold: ['#c9962b', '#f2c14e', '#231704'],
    };
    const [base, hi, ink] = palette[kind];
    const c = this.ctx;
    const g = c.createLinearGradient(0, r.y, 0, r.y + r.h);
    g.addColorStop(0, st.hover ? hi : base);
    g.addColorStop(1, base);
    c.globalAlpha = enabled ? 1 : 0.35;
    this.rrect({ ...r, y: r.y + (st.down ? 2 : 0) }, 10, g, kind === 'ghost' ? 'rgba(255,255,255,0.12)' : undefined);
    const size = opts.size ?? 22;
    const cy = r.y + r.h / 2 + (st.down ? 2 : 0);
    if (opts.sub) {
      this.text(label, r.x + r.w / 2, cy - 4, { size, weight: 700, color: ink, align: 'center', baseline: 'alphabetic' });
      this.text(opts.sub, r.x + r.w / 2, cy + size * 0.85, { size: size * 0.72, weight: 600, color: ink, align: 'center', baseline: 'alphabetic', font: MONO });
    } else {
      this.text(label, r.x + r.w / 2, cy + 1, { size, weight: 700, color: ink, align: 'center', baseline: 'middle' });
    }
    if (opts.hotkey) {
      this.text(opts.hotkey, r.x + r.w - 10, r.y + 16, { size: 12, weight: 700, color: ink, align: 'right', baseline: 'middle', font: MONO });
    }
    c.globalAlpha = 1;
    return st.clicked;
  }

  /** Horizontal slider; returns the new value. */
  slider(id: string, r: Rect, value: number, min: number, max: number, step = 1): number {
    const c = this.ctx;
    const st = this.clickable(id, { x: r.x - 10, y: r.y - 12, w: r.w + 20, h: r.h + 24 });
    let v = value;
    if ((st.down || (this.pressedId === id && this.pointer.down)) && max > min) {
      const t = Math.max(0, Math.min(1, (this.pointer.x - r.x) / r.w));
      v = min + t * (max - min);
      v = Math.round(v / step) * step;
    }
    if (st.hover && this.pointer.wheel !== 0) v -= Math.sign(this.pointer.wheel) * step;
    v = Math.max(min, Math.min(max, v));
    const t = max > min ? (v - min) / (max - min) : 0;
    this.rrect({ x: r.x, y: r.y + r.h / 2 - 4, w: r.w, h: 8 }, 4, '#0b1422', C.line);
    this.rrect({ x: r.x, y: r.y + r.h / 2 - 4, w: r.w * t, h: 8 }, 4, C.accent);
    c.beginPath();
    c.arc(r.x + r.w * t, r.y + r.h / 2, 13, 0, Math.PI * 2);
    c.fillStyle = st.hover || this.pressedId === id ? '#ffffff' : '#dce6f5';
    c.fill();
    c.strokeStyle = C.accent;
    c.lineWidth = 3;
    c.stroke();
    return v;
  }
}

export function money(cents: number): string {
  const neg = cents < 0;
  const v = Math.abs(cents) / 100;
  const s = v >= 1000 ? v.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 }) : v.toFixed(2);
  return `${neg ? '-' : ''}$${s}`;
}

export function chips(n: number): string {
  if (n >= 10_000_000) return `${(n / 1_000_000).toFixed(1)}M`;
  if (n >= 1_000_000) return `${(n / 1_000_000).toFixed(2)}M`;
  return Math.round(n).toLocaleString('en-US');
}

export function ordinal(n: number): string {
  const s = ['th', 'st', 'nd', 'rd'];
  const v = n % 100;
  return n.toLocaleString('en-US') + (s[(v - 20) % 10] || s[v] || s[0]);
}

export function clockString(minutes: number): string {
  const m = ((Math.floor(minutes) % 1440) + 1440) % 1440;
  const h24 = Math.floor(m / 60);
  const mm = m % 60;
  const h12 = h24 % 12 === 0 ? 12 : h24 % 12;
  return `${h12}:${String(mm).padStart(2, '0')} ${h24 < 12 ? 'AM' : 'PM'}`;
}

export const ease = {
  outCubic: (t: number) => 1 - Math.pow(1 - Math.min(1, Math.max(0, t)), 3),
  inOut: (t: number) => {
    const x = Math.min(1, Math.max(0, t));
    return x < 0.5 ? 4 * x * x * x : 1 - Math.pow(-2 * x + 2, 3) / 2;
  },
  outBack: (t: number) => {
    const x = Math.min(1, Math.max(0, t));
    const c1 = 1.70158;
    const c3 = c1 + 1;
    return 1 + c3 * Math.pow(x - 1, 3) + c1 * Math.pow(x - 1, 2);
  },
};
