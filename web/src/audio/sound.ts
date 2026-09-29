import type { SoundName } from '../game/session';

/**
 * Procedural audio. Every sound is synthesized with Web Audio (no samples):
 * rain on the window, room tone, thunder, clay chips, cards, UI chimes, the
 * phone buzzing and a heartbeat for all-in moments.
 */
export class Sound {
  private ctx: AudioContext | null = null;
  private master!: GainNode;
  private sfx!: GainNode;
  private amb!: GainNode;
  private noise!: AudioBuffer;
  private brown!: AudioBuffer;
  private heartOn = false;
  private nextBeat = 0;
  private dripTimer = 0;
  muted = false;

  start(): void {
    if (this.ctx) {
      void this.ctx.resume();
      return;
    }
    const AC = window.AudioContext || (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext;
    if (!AC) return;
    const ctx = new AC();
    this.ctx = ctx;
    const comp = ctx.createDynamicsCompressor();
    comp.threshold.value = -18;
    comp.ratio.value = 3;
    comp.connect(ctx.destination);
    this.master = ctx.createGain();
    this.master.gain.value = 0.9;
    this.master.connect(comp);
    this.sfx = ctx.createGain();
    this.sfx.gain.value = 0.8;
    this.sfx.connect(this.master);
    this.amb = ctx.createGain();
    this.amb.gain.value = 0;
    this.amb.connect(this.master);
    this.amb.gain.linearRampToValueAtTime(0.9, ctx.currentTime + 3);

    const len = ctx.sampleRate * 3;
    this.noise = ctx.createBuffer(1, len, ctx.sampleRate);
    const d = this.noise.getChannelData(0);
    for (let i = 0; i < len; i++) d[i] = Math.random() * 2 - 1;
    this.brown = ctx.createBuffer(1, len, ctx.sampleRate);
    const b = this.brown.getChannelData(0);
    let last = 0;
    for (let i = 0; i < len; i++) {
      last = (last + 0.02 * (Math.random() * 2 - 1)) / 1.02;
      b[i] = last * 3.5;
    }
    this.ambience();
  }

  setMuted(m: boolean): void {
    this.muted = m;
    if (this.ctx) this.master.gain.setTargetAtTime(m ? 0 : 0.9, this.ctx.currentTime, 0.1);
  }

  private loop(buffer: AudioBuffer, dest: AudioNode): AudioBufferSourceNode {
    const src = this.ctx!.createBufferSource();
    src.buffer = buffer;
    src.loop = true;
    src.connect(dest);
    src.start();
    return src;
  }

  private ambience(): void {
    const ctx = this.ctx!;
    // Rain hiss on glass (muffled by the window).
    const hissBp = ctx.createBiquadFilter();
    hissBp.type = 'bandpass';
    hissBp.frequency.value = 2400;
    hissBp.Q.value = 0.5;
    const hissLp = ctx.createBiquadFilter();
    hissLp.type = 'lowpass';
    hissLp.frequency.value = 5200;
    const hissG = ctx.createGain();
    hissG.gain.value = 0.05;
    hissBp.connect(hissLp).connect(hissG).connect(this.amb);
    this.loop(this.noise, hissBp);
    // Heavy rain body with a slow swell.
    const bodyLp = ctx.createBiquadFilter();
    bodyLp.type = 'lowpass';
    bodyLp.frequency.value = 700;
    const bodyG = ctx.createGain();
    bodyG.gain.value = 0.12;
    const lfo = ctx.createOscillator();
    lfo.frequency.value = 0.07;
    const lfoG = ctx.createGain();
    lfoG.gain.value = 0.05;
    lfo.connect(lfoG).connect(bodyG.gain);
    lfo.start();
    bodyLp.connect(bodyG).connect(this.amb);
    this.loop(this.brown, bodyLp);
    // Room tone: a fridge hum and the laptop fan.
    const hum = ctx.createOscillator();
    hum.frequency.value = 58;
    const humG = ctx.createGain();
    humG.gain.value = 0.008;
    hum.connect(humG).connect(this.amb);
    hum.start();
    const fanLp = ctx.createBiquadFilter();
    fanLp.type = 'lowpass';
    fanLp.frequency.value = 380;
    const fanG = ctx.createGain();
    fanG.gain.value = 0.025;
    fanLp.connect(fanG).connect(this.amb);
    this.loop(this.noise, fanLp);
  }

  /** Called every frame for scheduled ambience (drips, heartbeat). */
  tick(dt: number): { beat: boolean } {
    if (!this.ctx) return { beat: false };
    this.dripTimer -= dt;
    if (this.dripTimer <= 0) {
      this.dripTimer = 0.05 + Math.random() * 0.25;
      this.burst({ freq: 1800 + Math.random() * 3500, q: 6, dur: 0.02, gain: 0.02 + Math.random() * 0.03, dest: this.amb });
    }
    let beat = false;
    if (this.heartOn && this.ctx.currentTime >= this.nextBeat) {
      this.nextBeat = this.ctx.currentTime + 0.68;
      this.thump(0, 62, 0.35);
      this.thump(0.16, 52, 0.25);
      beat = true;
    }
    return { beat };
  }

  heartbeat(on: boolean): void {
    this.heartOn = on;
    if (on && this.ctx) this.nextBeat = this.ctx.currentTime + 0.1;
  }

  private burst(o: { freq: number; q: number; dur: number; gain: number; delay?: number; dest?: AudioNode; type?: BiquadFilterType; sweepTo?: number }): void {
    const ctx = this.ctx!;
    const t = ctx.currentTime + (o.delay ?? 0);
    const src = ctx.createBufferSource();
    src.buffer = this.noise;
    const f = ctx.createBiquadFilter();
    f.type = o.type ?? 'bandpass';
    f.frequency.setValueAtTime(o.freq, t);
    if (o.sweepTo) f.frequency.exponentialRampToValueAtTime(o.sweepTo, t + o.dur);
    f.Q.value = o.q;
    const g = ctx.createGain();
    g.gain.setValueAtTime(0, t);
    g.gain.linearRampToValueAtTime(o.gain, t + 0.002);
    g.gain.exponentialRampToValueAtTime(0.0001, t + o.dur);
    src.connect(f).connect(g).connect(o.dest ?? this.sfx);
    src.start(t, Math.random() * 2, o.dur + 0.05);
  }

  private tone(freq: number, dur: number, gain: number, delay = 0, type: OscillatorType = 'sine', slideTo?: number): void {
    const ctx = this.ctx!;
    const t = ctx.currentTime + delay;
    const o = ctx.createOscillator();
    o.type = type;
    o.frequency.setValueAtTime(freq, t);
    if (slideTo) o.frequency.exponentialRampToValueAtTime(slideTo, t + dur);
    const g = ctx.createGain();
    g.gain.setValueAtTime(0, t);
    g.gain.linearRampToValueAtTime(gain, t + 0.01);
    g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
    o.connect(g).connect(this.sfx);
    o.start(t);
    o.stop(t + dur + 0.05);
  }

  private thump(delay: number, freq: number, gain: number): void {
    this.tone(freq, 0.18, gain, delay, 'sine', freq * 0.6);
  }

  private clack(delay: number, gain = 0.22): void {
    this.burst({ freq: 3200 + Math.random() * 1400, q: 9, dur: 0.035, gain, delay });
    this.burst({ freq: 900 + Math.random() * 300, q: 4, dur: 0.02, gain: gain * 0.4, delay });
  }

  thunder(delay: number, strength: number): void {
    if (!this.ctx) return;
    const ctx = this.ctx;
    const t = ctx.currentTime + delay;
    const src = ctx.createBufferSource();
    src.buffer = this.brown;
    const lp = ctx.createBiquadFilter();
    lp.type = 'lowpass';
    lp.frequency.setValueAtTime(420, t);
    lp.frequency.exponentialRampToValueAtTime(90, t + 4);
    const g = ctx.createGain();
    g.gain.setValueAtTime(0, t);
    g.gain.linearRampToValueAtTime(0.9 * strength, t + 0.25);
    g.gain.exponentialRampToValueAtTime(0.4 * strength, t + 1.2);
    g.gain.exponentialRampToValueAtTime(0.0001, t + 5);
    src.connect(lp).connect(g).connect(this.amb);
    src.start(t, Math.random(), 5.2);
  }

  buzz(): void {
    if (!this.ctx) return;
    for (let i = 0; i < 2; i++) {
      const ctx = this.ctx;
      const t = ctx.currentTime + i * 0.45;
      const o = ctx.createOscillator();
      o.type = 'square';
      o.frequency.value = 155;
      const lp = ctx.createBiquadFilter();
      lp.type = 'lowpass';
      lp.frequency.value = 400;
      const g = ctx.createGain();
      g.gain.setValueAtTime(0, t);
      g.gain.linearRampToValueAtTime(0.18, t + 0.02);
      g.gain.setValueAtTime(0.18, t + 0.28);
      g.gain.linearRampToValueAtTime(0, t + 0.32);
      o.connect(lp).connect(g).connect(this.sfx);
      o.start(t);
      o.stop(t + 0.35);
    }
  }

  canOpen(): void {
    if (!this.ctx) return;
    this.burst({ freq: 900, q: 3, dur: 0.05, gain: 0.25 });
    this.burst({ freq: 6000, q: 0.8, dur: 0.45, gain: 0.12, delay: 0.04, type: 'highpass' });
  }

  play(name: SoundName, opts: { volume?: number } = {}): void {
    if (!this.ctx) return;
    const v = opts.volume ?? 1;
    switch (name) {
      case 'chip':
        this.clack(0, 0.18 * v);
        break;
      case 'chips':
        for (let i = 0; i < 4; i++) this.clack(i * (0.025 + Math.random() * 0.03), 0.16 * v);
        break;
      case 'allin':
        for (let i = 0; i < 9; i++) this.clack(i * 0.03 + Math.random() * 0.02, 0.2 * v);
        this.tone(70, 0.6, 0.35 * v, 0.02, 'sine', 45);
        break;
      case 'card':
        for (let i = 0; i < 4; i++) this.burst({ freq: 2200, q: 1.2, dur: 0.07, gain: 0.08 * v, delay: i * 0.09, sweepTo: 6000 });
        break;
      case 'flip':
        this.burst({ freq: 4200, q: 1.5, dur: 0.03, gain: 0.14 * v, type: 'highpass' });
        break;
      case 'check':
        this.tone(190, 0.06, 0.2 * v, 0, 'sine', 120);
        this.tone(190, 0.06, 0.2 * v, 0.1, 'sine', 120);
        break;
      case 'fold':
        this.burst({ freq: 1500, q: 1, dur: 0.12, gain: 0.07 * v, sweepTo: 600 });
        break;
      case 'turn':
        this.tone(880, 0.25, 0.12 * v);
        this.tone(1318, 0.35, 0.1 * v, 0.12);
        break;
      case 'alert':
        this.tone(1046, 0.3, 0.1 * v);
        break;
      case 'click':
        this.tone(1900, 0.02, 0.05 * v, 0, 'square');
        break;
      case 'win':
        for (let i = 0; i < 8; i++) this.clack(0.1 + i * 0.035, 0.15 * v);
        [523, 659, 784].forEach((f, i) => this.tone(f, 0.4, 0.07 * v, i * 0.08));
        break;
      case 'level':
        this.tone(1046, 1.2, 0.09 * v);
        this.tone(1568, 1.0, 0.05 * v);
        break;
      case 'bust':
        this.tone(220, 0.9, 0.18 * v, 0, 'triangle', 98);
        break;
      case 'move':
        this.burst({ freq: 400, q: 0.7, dur: 0.5, gain: 0.08 * v, sweepTo: 3000 });
        break;
      case 'bubble':
        this.tone(110, 2.2, 0.12 * v, 0, 'sawtooth', 116);
        this.tone(165, 2.2, 0.06 * v, 0.05, 'sine');
        break;
      case 'cash':
        this.tone(1568, 0.25, 0.1 * v);
        this.tone(2093, 0.4, 0.08 * v, 0.08);
        for (let i = 0; i < 6; i++) this.clack(0.15 + i * 0.03, 0.12 * v);
        break;
    }
  }
}
