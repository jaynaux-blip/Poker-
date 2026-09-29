/**
 * Seeded pseudo-random number generator (sfc32).
 *
 * Every shuffle and every AI coin-flip goes through an Rng instance so a whole
 * tournament can be replayed exactly from its seed. The generator is uniform;
 * nothing in the game biases the deck.
 */
export class Rng {
  private a: number;
  private b: number;
  private c: number;
  private d: number;

  constructor(seed: number | string = Date.now()) {
    const [a, b, c, d] = seedState(String(seed));
    this.a = a;
    this.b = b;
    this.c = c;
    this.d = d;
    // Discard the first outputs to mix the state.
    for (let i = 0; i < 15; i++) this.nextUint32();
  }

  nextUint32(): number {
    this.a >>>= 0;
    this.b >>>= 0;
    this.c >>>= 0;
    this.d >>>= 0;
    let t = (this.a + this.b) | 0;
    this.a = this.b ^ (this.b >>> 9);
    this.b = (this.c + (this.c << 3)) | 0;
    this.c = (this.c << 21) | (this.c >>> 11);
    this.d = (this.d + 1) | 0;
    t = (t + this.d) | 0;
    this.c = (this.c + t) | 0;
    return t >>> 0;
  }

  /** Uniform float in [0, 1). */
  next(): number {
    return this.nextUint32() / 4294967296;
  }

  /** Uniform integer in [0, n). Rejection sampling avoids modulo bias. */
  int(n: number): number {
    if (n <= 0) throw new Error('Rng.int: n must be positive');
    const limit = Math.floor(4294967296 / n) * n;
    let x = this.nextUint32();
    while (x >= limit) x = this.nextUint32();
    return x % n;
  }

  range(min: number, max: number): number {
    return min + (max - min) * this.next();
  }

  chance(p: number): boolean {
    return this.next() < p;
  }

  pick<T>(items: readonly T[]): T {
    return items[this.int(items.length)];
  }

  /** Approximately normal sample (Irwin–Hall with 4 terms). */
  gauss(mean = 0, sd = 1): number {
    const s = this.next() + this.next() + this.next() + this.next() - 2;
    return mean + sd * s * Math.sqrt(3);
  }

  /** In-place Fisher–Yates shuffle. */
  shuffle<T>(items: T[]): T[] {
    for (let i = items.length - 1; i > 0; i--) {
      const j = this.int(i + 1);
      const tmp = items[i];
      items[i] = items[j];
      items[j] = tmp;
    }
    return items;
  }

  /** Derive an independent child generator. */
  fork(label: string | number): Rng {
    return new Rng(`${this.nextUint32()}:${label}`);
  }
}

/** cyrb128 string hash, used to expand a seed into 128 bits of state. */
function seedState(str: string): [number, number, number, number] {
  let h1 = 1779033703;
  let h2 = 3144134277;
  let h3 = 1013904242;
  let h4 = 2773480762;
  for (let i = 0; i < str.length; i++) {
    const k = str.charCodeAt(i);
    h1 = h2 ^ Math.imul(h1 ^ k, 597399067);
    h2 = h3 ^ Math.imul(h2 ^ k, 2869860233);
    h3 = h4 ^ Math.imul(h3 ^ k, 951274213);
    h4 = h1 ^ Math.imul(h4 ^ k, 2716044179);
  }
  h1 = Math.imul(h3 ^ (h1 >>> 18), 597399067);
  h2 = Math.imul(h4 ^ (h2 >>> 22), 2869860233);
  h3 = Math.imul(h1 ^ (h3 >>> 17), 951274213);
  h4 = Math.imul(h2 ^ (h4 >>> 19), 2716044179);
  h1 ^= h2 ^ h3 ^ h4;
  h2 ^= h1;
  h3 ^= h1;
  h4 ^= h1;
  return [h1 >>> 0, h2 >>> 0, h3 >>> 0, h4 >>> 0];
}
