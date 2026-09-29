import { clockString } from '../client/canvasui';
import { Phone } from '../scene/props';

/** Draws the phone's lock screen and notifications onto its texture. */
export class PhoneScreen {
  private messages: { from: string; body: string; at: number }[] = [];
  private litUntil = 0;

  constructor(private phone: Phone) {
    this.draw(0, 0);
  }

  notify(from: string, body: string, now: number): void {
    this.messages.unshift({ from, body, at: now });
    this.messages = this.messages.slice(0, 4);
    this.litUntil = now + 9;
  }

  update(now: number, clockMinutes: number): void {
    const lit = now < this.litUntil;
    const fade = lit ? Math.min(1, (this.litUntil - now) / 1.5) : 0;
    const level = lit ? Math.max(0.15, fade) : 0;
    this.phone.screenMat.color.setScalar(level);
    this.phone.light.intensity = level * 0.35;
    if (lit) this.draw(now, clockMinutes);
  }

  private draw(now: number, clockMinutes: number): void {
    const c = this.phone.ctx;
    const W = this.phone.canvas.width;
    const H = this.phone.canvas.height;
    const g = c.createLinearGradient(0, 0, W, H);
    g.addColorStop(0, '#1d2a4a');
    g.addColorStop(1, '#3a1638');
    c.fillStyle = g;
    c.fillRect(0, 0, W, H);
    c.fillStyle = '#fff';
    c.textAlign = 'center';
    c.font = '300 96px Inter, Arial, sans-serif';
    const [time] = clockString(clockMinutes).split(' ');
    c.fillText(time, W / 2, 170);
    c.font = '500 22px Inter, Arial, sans-serif';
    c.fillStyle = 'rgba(255,255,255,0.75)';
    c.fillText('Wednesday', W / 2, 210);
    let y = 260;
    for (const m of this.messages) {
      c.fillStyle = 'rgba(255,255,255,0.16)';
      c.beginPath();
      c.roundRect(16, y, W - 32, 150, 22);
      c.fill();
      c.textAlign = 'left';
      c.fillStyle = '#fff';
      c.font = '700 22px Inter, Arial, sans-serif';
      c.fillText(m.from, 36, y + 38);
      c.font = '400 19px Inter, Arial, sans-serif';
      c.fillStyle = 'rgba(255,255,255,0.88)';
      const words = m.body.split(' ');
      let line = '';
      let ly = y + 70;
      for (const w of words) {
        const test = line ? `${line} ${w}` : w;
        if (c.measureText(test).width > W - 72) {
          c.fillText(line, 36, ly);
          line = w;
          ly += 25;
          if (ly > y + 140) break;
        } else line = test;
      }
      if (ly <= y + 140) c.fillText(line, 36, ly);
      y += 164;
      if (y > H - 100) break;
    }
    this.phone.texture.needsUpdate = true;
    void now;
  }
}
