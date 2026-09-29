import * as THREE from 'three';

/**
 * Procedural texture toolkit. Every surface in the prototype is generated at
 * load time (no downloaded assets): value-noise fBm for albedo, roughness and
 * height, and a Sobel pass to turn height into tangent-space normal maps.
 */

export class Noise {
  private perm: Uint8Array;
  private grad: Float32Array;

  constructor(seed = 1) {
    this.perm = new Uint8Array(512);
    this.grad = new Float32Array(256);
    let s = seed * 9301 + 49297;
    const rand = () => {
      s = (s * 9301 + 49297) % 233280;
      return s / 233280;
    };
    const p = Array.from({ length: 256 }, (_, i) => i);
    for (let i = 255; i > 0; i--) {
      const j = Math.floor(rand() * (i + 1));
      [p[i], p[j]] = [p[j], p[i]];
    }
    for (let i = 0; i < 512; i++) this.perm[i] = p[i & 255];
    for (let i = 0; i < 256; i++) this.grad[i] = rand();
  }

  /** Tileable value noise with integer period `px` x `py`. */
  value(x: number, y: number, px = 256, py = 256): number {
    const xi = Math.floor(x);
    const yi = Math.floor(y);
    const xf = x - xi;
    const yf = y - yi;
    const u = xf * xf * (3 - 2 * xf);
    const v = yf * yf * (3 - 2 * yf);
    const x0 = ((xi % px) + px) % px;
    const y0 = ((yi % py) + py) % py;
    const x1 = (x0 + 1) % px;
    const y1 = (y0 + 1) % py;
    const h = (a: number, b: number) => this.grad[this.perm[(this.perm[a & 255] + b) & 511]];
    const a = h(x0, y0);
    const b = h(x1, y0);
    const c = h(x0, y1);
    const d = h(x1, y1);
    return a + (b - a) * u + (c - a) * v + (a - b - c + d) * u * v;
  }

  fbm(x: number, y: number, octaves = 5, px = 256, py = 256): number {
    let sum = 0;
    let amp = 0.5;
    let freq = 1;
    let norm = 0;
    for (let o = 0; o < octaves; o++) {
      sum += amp * this.value(x * freq, y * freq, px * freq, py * freq);
      norm += amp;
      amp *= 0.5;
      freq *= 2;
    }
    return sum / norm;
  }
}

export function makeCanvas(w: number, h: number): [HTMLCanvasElement, CanvasRenderingContext2D] {
  const c = document.createElement('canvas');
  c.width = w;
  c.height = h;
  const ctx = c.getContext('2d')!;
  return [c, ctx];
}

export function canvasTexture(c: HTMLCanvasElement, srgb = true, repeat = false): THREE.CanvasTexture {
  const t = new THREE.CanvasTexture(c);
  if (srgb) t.colorSpace = THREE.SRGBColorSpace;
  t.anisotropy = 8;
  if (repeat) {
    t.wrapS = THREE.RepeatWrapping;
    t.wrapT = THREE.RepeatWrapping;
  }
  t.needsUpdate = true;
  return t;
}

/** Convert a height field (0..1) into a tangent-space normal map. */
export function normalFromHeight(height: Float32Array, w: number, h: number, strength = 2, wrap = true): THREE.DataTexture {
  const data = new Uint8Array(w * h * 4);
  const at = (x: number, y: number) => {
    if (wrap) {
      x = (x + w) % w;
      y = (y + h) % h;
    } else {
      x = Math.max(0, Math.min(w - 1, x));
      y = Math.max(0, Math.min(h - 1, y));
    }
    return height[y * w + x];
  };
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const dx = (at(x + 1, y) - at(x - 1, y)) * strength;
      const dy = (at(x, y + 1) - at(x, y - 1)) * strength;
      const nx = -dx;
      const ny = dy;
      const nz = 1;
      const len = Math.hypot(nx, ny, nz);
      const i = (y * w + x) * 4;
      data[i] = ((nx / len) * 0.5 + 0.5) * 255;
      data[i + 1] = ((ny / len) * 0.5 + 0.5) * 255;
      data[i + 2] = ((nz / len) * 0.5 + 0.5) * 255;
      data[i + 3] = 255;
    }
  }
  const t = new THREE.DataTexture(data, w, h, THREE.RGBAFormat);
  t.wrapS = t.wrapT = wrap ? THREE.RepeatWrapping : THREE.ClampToEdgeWrapping;
  t.generateMipmaps = true;
  t.minFilter = THREE.LinearMipmapLinearFilter;
  t.magFilter = THREE.LinearFilter;
  t.anisotropy = 8;
  t.needsUpdate = true;
  return t;
}

/** Grayscale data texture (roughness / metalness / AO maps). */
export function grayTexture(values: Float32Array, w: number, h: number, wrap = true): THREE.DataTexture {
  const data = new Uint8Array(w * h * 4);
  for (let i = 0; i < w * h; i++) {
    const v = Math.max(0, Math.min(255, values[i] * 255));
    data[i * 4] = v;
    data[i * 4 + 1] = v;
    data[i * 4 + 2] = v;
    data[i * 4 + 3] = 255;
  }
  const t = new THREE.DataTexture(data, w, h, THREE.RGBAFormat);
  t.wrapS = t.wrapT = wrap ? THREE.RepeatWrapping : THREE.ClampToEdgeWrapping;
  t.generateMipmaps = true;
  t.minFilter = THREE.LinearMipmapLinearFilter;
  t.magFilter = THREE.LinearFilter;
  t.needsUpdate = true;
  return t;
}

export interface PBRSet {
  map: THREE.Texture;
  roughnessMap: THREE.Texture;
  normalMap: THREE.Texture;
}

const clamp01 = (v: number) => Math.max(0, Math.min(1, v));
const mix = (a: number, b: number, t: number) => a + (b - a) * t;

/**
 * Old apartment wall: off-white paint over plaster, with water stains near
 * the top, grime, and patches where the paint has peeled.
 */
export function wallTextures(w = 1024, h = 768, seed = 3): PBRSet {
  const n = new Noise(seed);
  const [c, ctx] = makeCanvas(w, h);
  const img = ctx.createImageData(w, h);
  const height = new Float32Array(w * h);
  const rough = new Float32Array(w * h);
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const u = x / w;
      const v = y / h;
      const plaster = n.fbm(u * 64, v * 48, 4, 64, 48);
      const grime = n.fbm(u * 6 + 10, v * 5, 5, 6, 5);
      const peel = n.fbm(u * 12 + 40, v * 9 + 7, 5, 12, 9);
      const stain = n.fbm(u * 4 + 3, v * 3 + 2, 4, 4, 3) * clamp01(1.2 - v * 2.2); // top of wall
      const peeled = clamp01((peel - 0.66) * 9); // 0 = paint, 1 = exposed plaster
      const edge = clamp01(1 - Math.abs(peel - 0.66) * 60) * 0.6;
      // Paint: dusty sage/greige; exposed plaster: warmer, darker.
      let r = mix(0.63, 0.55, peeled);
      let g = mix(0.64, 0.5, peeled);
      let b = mix(0.58, 0.42, peeled);
      const dirt = 0.82 + 0.18 * grime - 0.06 * plaster;
      const stainK = clamp01((stain - 0.45) * 3) * 0.45;
      r = r * dirt * (1 - stainK * 0.35) + stainK * 0.12;
      g = g * dirt * (1 - stainK * 0.42) + stainK * 0.08;
      b = b * dirt * (1 - stainK * 0.55) + stainK * 0.02;
      const i = (y * w + x) * 4;
      img.data[i] = clamp01(r - edge * 0.12) * 255;
      img.data[i + 1] = clamp01(g - edge * 0.12) * 255;
      img.data[i + 2] = clamp01(b - edge * 0.12) * 255;
      img.data[i + 3] = 255;
      height[y * w + x] = plaster * 0.25 + (1 - peeled) * 0.35 + edge * 0.2;
      rough[y * w + x] = mix(0.78, 0.95, peeled) + (grime - 0.5) * 0.08;
    }
  }
  ctx.putImageData(img, 0, 0);
  return {
    map: canvasTexture(c, true, true),
    roughnessMap: grayTexture(rough, w, h),
    normalMap: normalFromHeight(height, w, h, 3),
  };
}

/**
 * Desk top: worn walnut veneer with fine grain, scratches, a sun-faded patch
 * and coffee rings.
 */
export function woodTextures(w = 1024, h = 512, seed = 7): PBRSet {
  const n = new Noise(seed);
  const [c, ctx] = makeCanvas(w, h);
  const img = ctx.createImageData(w, h);
  const height = new Float32Array(w * h);
  const rough = new Float32Array(w * h);
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const u = x / w;
      const v = y / h;
      const warp = n.fbm(u * 2, v * 6, 5, 2, 6) * 9;
      const rings = Math.sin((v * 22 + warp) * Math.PI);
      const fine = n.fbm(u * 256, v * 8, 3, 256, 8);
      const grain = Math.pow(0.5 + 0.5 * rings, 2.2);
      const pores = n.value(u * 700, v * 90, 700, 90);
      const tone = n.fbm(u * 1.5 + 4, v * 2, 3, 2, 2);
      const k = 0.62 + 0.14 * grain + 0.14 * fine + 0.2 * (tone - 0.5);
      let r = 0.36 * k;
      let g = 0.22 * k;
      let b = 0.13 * k;
      const wear = clamp01((n.fbm(u * 5 + 9, v * 3, 4, 5, 3) - 0.55) * 4);
      r = mix(r, r * 1.25 + 0.03, wear * 0.5);
      g = mix(g, g * 1.2 + 0.02, wear * 0.5);
      b = mix(b, b * 1.1, wear * 0.5);
      const i = (y * w + x) * 4;
      img.data[i] = clamp01(r) * 255;
      img.data[i + 1] = clamp01(g) * 255;
      img.data[i + 2] = clamp01(b) * 255;
      img.data[i + 3] = 255;
      height[y * w + x] = grain * 0.4 + fine * 0.3 - (pores < 0.18 ? 0.25 : 0);
      rough[y * w + x] = 0.42 + 0.2 * (1 - grain) + wear * 0.25 + (pores < 0.18 ? 0.15 : 0);
    }
  }
  ctx.putImageData(img, 0, 0);
  // Scratches.
  const rs = new Noise(seed + 1);
  ctx.globalCompositeOperation = 'screen';
  for (let i = 0; i < 90; i++) {
    const x0 = rs.value(i * 3.1, 0.5) * w;
    const y0 = rs.value(i * 1.7, 9.5) * h;
    const len = 20 + rs.value(i, 3.3) * 160;
    const ang = (rs.value(i * 0.7, 5) - 0.5) * 0.8;
    ctx.strokeStyle = `rgba(255,220,180,${0.05 + rs.value(i, 7) * 0.08})`;
    ctx.lineWidth = 0.6 + rs.value(i, 11) * 0.8;
    ctx.beginPath();
    ctx.moveTo(x0, y0);
    ctx.lineTo(x0 + Math.cos(ang) * len, y0 + Math.sin(ang) * len);
    ctx.stroke();
  }
  // Coffee rings.
  ctx.globalCompositeOperation = 'multiply';
  const ring = (cx: number, cy: number, rad: number, a: number) => {
    ctx.strokeStyle = `rgba(90,55,30,${a})`;
    ctx.lineWidth = 2.5;
    ctx.beginPath();
    ctx.arc(cx, cy, rad, 0.2, Math.PI * 1.85);
    ctx.stroke();
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.arc(cx + 1, cy - 1, rad - 2, 0, Math.PI * 2);
    ctx.stroke();
  };
  ring(w * 0.78, h * 0.35, 30, 0.45);
  ring(w * 0.81, h * 0.42, 30, 0.3);
  ring(w * 0.2, h * 0.62, 28, 0.35);
  ctx.globalCompositeOperation = 'source-over';
  return {
    map: canvasTexture(c, true, true),
    roughnessMap: grayTexture(rough, w, h),
    normalMap: normalFromHeight(height, w, h, 1.5),
  };
}

/** Old floorboards. */
export function floorTextures(w = 1024, h = 1024, seed = 11): PBRSet {
  const n = new Noise(seed);
  const [c, ctx] = makeCanvas(w, h);
  const img = ctx.createImageData(w, h);
  const height = new Float32Array(w * h);
  const rough = new Float32Array(w * h);
  const planks = 8;
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const u = x / w;
      const v = y / h;
      const p = Math.floor(u * planks);
      const pu = u * planks - p;
      const offset = n.value(p * 13.1, 0.5) * 3;
      const seg = Math.floor(v * 2 + offset);
      const tone = 0.75 + 0.35 * n.value(p * 7.3, seg * 3.1);
      const grain = n.fbm(u * 40, v * 3 + p * 5, 4, 40, 3);
      const gap = pu < 0.012 || pu > 0.988 ? 1 : 0;
      const joint = Math.abs((v * 2 + offset) - Math.round(v * 2 + offset)) < 0.004 ? 1 : 0;
      const k = tone * (0.6 + 0.4 * grain);
      const i = (y * w + x) * 4;
      const dark = gap || joint ? 0.25 : 1;
      img.data[i] = clamp01(0.3 * k * dark) * 255;
      img.data[i + 1] = clamp01(0.2 * k * dark) * 255;
      img.data[i + 2] = clamp01(0.13 * k * dark) * 255;
      img.data[i + 3] = 255;
      height[y * w + x] = gap || joint ? 0 : 0.5 + grain * 0.3;
      rough[y * w + x] = 0.55 + grain * 0.25 + (gap ? 0.2 : 0);
    }
  }
  ctx.putImageData(img, 0, 0);
  return {
    map: canvasTexture(c, true, true),
    roughnessMap: grayTexture(rough, w, h),
    normalMap: normalFromHeight(height, w, h, 2),
  };
}

/** Fine brushed-metal / plastic micro normal for the laptop and props. */
export function brushedNormal(w = 256, h = 256, seed = 5): THREE.DataTexture {
  const n = new Noise(seed);
  const height = new Float32Array(w * h);
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) height[y * w + x] = n.fbm((x / w) * 128, (y / h) * 2, 3, 128, 2);
  return normalFromHeight(height, w, h, 0.6);
}

/** Soft radial gradient sprite (glows, steam puffs). */
export function radialSprite(size = 128, inner = 'rgba(255,255,255,1)', outer = 'rgba(255,255,255,0)'): THREE.CanvasTexture {
  const [c, ctx] = makeCanvas(size, size);
  const g = ctx.createRadialGradient(size / 2, size / 2, 0, size / 2, size / 2, size / 2);
  g.addColorStop(0, inner);
  g.addColorStop(1, outer);
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, size, size);
  return canvasTexture(c);
}
