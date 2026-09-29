import * as THREE from 'three';
import { RoundedBoxGeometry } from 'three/examples/jsm/geometries/RoundedBoxGeometry.js';
import { Noise, brushedNormal, canvasTexture, makeCanvas } from './procedural';

/** Desk props: laptop, cans, mug, noodle cup, phone, papers. */

const HAND = '"Caveat", "Segoe Print", "Comic Sans MS", cursive';

// ------------------------------------------------------------------ laptop

function keyboardTextures(): { map: THREE.Texture; emissive: THREE.Texture } {
  const W = 1024;
  const H = 440;
  const [c, ctx] = makeCanvas(W, H);
  const [e, ectx] = makeCanvas(W, H);
  ctx.fillStyle = '#1b1c1f';
  ctx.fillRect(0, 0, W, H);
  ectx.fillStyle = '#000';
  ectx.fillRect(0, 0, W, H);
  const rows = [
    ['esc', 'F1', 'F2', 'F3', 'F4', 'F5', 'F6', 'F7', 'F8', 'F9', 'F10', 'F11', 'F12', 'del'],
    ['`', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '⌫'],
    ['tab', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\\'],
    ['caps', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', "'", 'enter'],
    ['shift', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 'shift'],
    ['ctrl', 'fn', 'alt', ' ', 'alt', '◀', '▲▼', '▶'],
  ];
  const wide: Record<string, number> = { tab: 1.5, caps: 1.8, enter: 2.2, shift: 2.4, ' ': 6.2, '⌫': 1.5, '\\': 1.5 };
  const y0 = 20;
  const rowH = [34, 62, 62, 62, 62, 62];
  let y = y0;
  rows.forEach((row, ri) => {
    const units = row.reduce((a, k) => a + (wide[k] ?? 1), 0);
    const unitW = (W - 60) / units;
    let x = 30;
    for (const k of row) {
      const kw = (wide[k] ?? 1) * unitW;
      const kh = rowH[ri] - 8;
      ctx.fillStyle = '#0d0e10';
      ctx.beginPath();
      ctx.roundRect(x + 3, y + 3, kw - 6, kh, 6);
      ctx.fill();
      ctx.fillStyle = '#26282c';
      ctx.beginPath();
      ctx.roundRect(x + 4, y + 2, kw - 8, kh - 4, 5);
      ctx.fill();
      const label = k === ' ' ? '' : k;
      const size = label.length > 2 ? 13 : 18;
      for (const [g, col] of [[ctx, '#8d9096'], [ectx, '#c9e1ff']] as const) {
        g.font = `500 ${size}px Inter, Arial, sans-serif`;
        g.fillStyle = col;
        g.textAlign = 'left';
        g.textBaseline = 'top';
        g.fillText(label, x + 11, y + 9);
      }
      x += kw;
    }
    y += rowH[ri];
  });
  // Trackpad outline area below keys.
  ctx.strokeStyle = '#2d2f33';
  ctx.lineWidth = 2;
  return { map: canvasTexture(c), emissive: canvasTexture(e) };
}

export interface Laptop {
  group: THREE.Group;
  screen: THREE.Mesh; // UI surface, UVs 0..1 map to the client canvas
  screenCenter: THREE.Vector3;
  screenNormal: THREE.Vector3;
  screenSize: THREE.Vector2;
}

export function createLaptop(uiTexture: THREE.Texture): Laptop {
  const group = new THREE.Group();
  const bodyMat = new THREE.MeshPhysicalMaterial({
    color: 0x2b2d31,
    metalness: 0.85,
    roughness: 0.38,
    normalMap: brushedNormal(),
    normalScale: new THREE.Vector2(0.4, 0.4),
    clearcoat: 0.2,
    clearcoatRoughness: 0.4,
  });
  const W = 0.34;
  const D = 0.235;
  const T = 0.016;
  const base = new THREE.Mesh(new RoundedBoxGeometry(W, T, D, 3, 0.006), bodyMat);
  base.position.y = T / 2;
  base.castShadow = true;
  base.receiveShadow = true;
  group.add(base);

  const kb = keyboardTextures();
  const deck = new THREE.Mesh(
    new THREE.PlaneGeometry(W - 0.03, 0.118),
    new THREE.MeshStandardMaterial({ map: kb.map, emissiveMap: kb.emissive, emissive: new THREE.Color(0.35, 0.5, 0.7), emissiveIntensity: 0.25, roughness: 0.7 }),
  );
  deck.rotation.x = -Math.PI / 2;
  deck.position.set(0, T + 0.0006, -0.035);
  group.add(deck);
  const pad = new THREE.Mesh(
    new THREE.PlaneGeometry(0.11, 0.07),
    new THREE.MeshPhysicalMaterial({ color: 0x303236, metalness: 0.6, roughness: 0.25, clearcoat: 0.6 }),
  );
  pad.rotation.x = -Math.PI / 2;
  pad.position.set(0, T + 0.0005, 0.068);
  group.add(pad);

  // Lid, hinged at the back edge and tilted back.
  const lidPivot = new THREE.Group();
  lidPivot.position.set(0, T, -D / 2);
  lidPivot.rotation.x = -0.32; // lean back from vertical
  group.add(lidPivot);
  const LH = 0.225;
  const lid = new THREE.Mesh(new RoundedBoxGeometry(W, LH, 0.007, 3, 0.003), bodyMat);
  lid.position.set(0, LH / 2, -0.0035);
  lid.castShadow = true;
  lidPivot.add(lid);
  const bezel = new THREE.Mesh(new THREE.PlaneGeometry(W - 0.006, LH - 0.006), new THREE.MeshStandardMaterial({ color: 0x050506, roughness: 0.35, metalness: 0.2 }));
  bezel.position.set(0, LH / 2, 0.0002);
  lidPivot.add(bezel);
  const sw = 0.316;
  const sh = sw / 1.6;
  // Slightly below full white so only the brightest pixels bloom, like a real panel.
  const screen = new THREE.Mesh(new THREE.PlaneGeometry(sw, sh), new THREE.MeshBasicMaterial({ map: uiTexture, toneMapped: false, color: new THREE.Color(0.86, 0.86, 0.86) }));
  screen.position.set(0, LH / 2 + 0.004, 0.0005);
  lidPivot.add(screen);
  // Webcam dot.
  const cam = new THREE.Mesh(new THREE.CircleGeometry(0.0018, 12), new THREE.MeshBasicMaterial({ color: 0x0b0d12 }));
  cam.position.set(0, LH - 0.006, 0.0004);
  lidPivot.add(cam);

  group.updateMatrixWorld(true);
  return {
    group,
    screen,
    screenCenter: new THREE.Vector3(),
    screenNormal: new THREE.Vector3(),
    screenSize: new THREE.Vector2(sw, sh),
  };
}

// ------------------------------------------------------------------ cans

function canLabel(): THREE.Texture {
  const [c, ctx] = makeCanvas(1024, 512);
  const g = ctx.createLinearGradient(0, 0, 0, 512);
  g.addColorStop(0, '#0f1a12');
  g.addColorStop(1, '#07100a');
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, 1024, 512);
  // Lightning stripe.
  ctx.fillStyle = '#b8ff2e';
  ctx.beginPath();
  ctx.moveTo(420, 30);
  ctx.lineTo(560, 30);
  ctx.lineTo(500, 230);
  ctx.lineTo(600, 230);
  ctx.lineTo(430, 490);
  ctx.lineTo(480, 290);
  ctx.lineTo(390, 290);
  ctx.closePath();
  ctx.fill();
  ctx.font = '900 150px Impact, "Arial Black", sans-serif';
  ctx.textAlign = 'center';
  ctx.fillStyle = '#e9ffe0';
  ctx.save();
  ctx.translate(200, 260);
  ctx.rotate(-Math.PI / 2);
  ctx.fillText('VOLT', 0, 50);
  ctx.restore();
  ctx.font = '700 36px Arial, sans-serif';
  ctx.fillStyle = '#b8ff2e';
  ctx.fillText('ZERO SUGAR · MAX FOCUS', 760, 250);
  ctx.font = '500 26px Arial, sans-serif';
  ctx.fillStyle = '#7f9a78';
  ctx.fillText('160 MG CAFFEINE', 760, 300);
  const t = canvasTexture(c);
  return t;
}

let sharedCanMat: THREE.Material[] | null = null;
let sharedCanGeo: THREE.BufferGeometry | null = null;

export function createCan(open = true): THREE.Group {
  if (!sharedCanGeo) {
    // Profile: base bevel, straight body, tapered neck.
    const pts = [
      [0, 0], [0.026, 0], [0.0325, 0.006], [0.0332, 0.012], [0.0332, 0.106], [0.029, 0.118], [0.027, 0.122], [0.0275, 0.1235], [0, 0.1235],
    ].map(([x, y]) => new THREE.Vector2(x, y));
    sharedCanGeo = new THREE.LatheGeometry(pts, 40);
    const label = canLabel();
    sharedCanMat = [new THREE.MeshPhysicalMaterial({ map: label, metalness: 0.75, roughness: 0.28, clearcoat: 0.5, clearcoatRoughness: 0.2 })];
  }
  const group = new THREE.Group();
  const body = new THREE.Mesh(sharedCanGeo, sharedCanMat![0]);
  body.castShadow = true;
  body.receiveShadow = true;
  group.add(body);
  const lid = new THREE.Mesh(
    new THREE.CircleGeometry(0.0272, 32),
    new THREE.MeshStandardMaterial({ color: 0xb9bcc2, metalness: 1, roughness: 0.3 }),
  );
  lid.rotation.x = -Math.PI / 2;
  lid.position.y = 0.1236;
  group.add(lid);
  if (open) {
    const hole = new THREE.Mesh(new THREE.CircleGeometry(0.007, 16), new THREE.MeshBasicMaterial({ color: 0x050505 }));
    hole.rotation.x = -Math.PI / 2;
    hole.position.set(0, 0.1238, 0.013);
    hole.scale.set(1, 1.5, 1);
    group.add(hole);
  }
  const tab = new THREE.Mesh(new THREE.BoxGeometry(0.012, 0.0012, 0.02), new THREE.MeshStandardMaterial({ color: 0xc9ccd2, metalness: 1, roughness: 0.25 }));
  tab.position.set(0, 0.1245, open ? -0.004 : 0.004);
  group.add(tab);
  return group;
}

// ------------------------------------------------------------------ mug

export function createMug(): { group: THREE.Group; steamOrigin: THREE.Vector3 } {
  const group = new THREE.Group();
  const outer = [
    [0.0, 0.0], [0.036, 0.0], [0.04, 0.004], [0.041, 0.09], [0.0395, 0.094], [0.036, 0.094], [0.035, 0.012], [0.0, 0.012],
  ].map(([x, y]) => new THREE.Vector2(x, y));
  const mat = new THREE.MeshPhysicalMaterial({ color: 0xd8d2c4, roughness: 0.25, clearcoat: 0.6, clearcoatRoughness: 0.15 });
  const mug = new THREE.Mesh(new THREE.LatheGeometry(outer, 40), mat);
  mug.castShadow = true;
  group.add(mug);
  const handle = new THREE.Mesh(new THREE.TorusGeometry(0.024, 0.0055, 10, 24, Math.PI), mat);
  handle.rotation.z = -Math.PI / 2;
  handle.position.set(0.041, 0.048, 0);
  handle.castShadow = true;
  group.add(handle);
  const coffee = new THREE.Mesh(new THREE.CircleGeometry(0.035, 32), new THREE.MeshPhysicalMaterial({ color: 0x1a0d06, roughness: 0.05, clearcoat: 1 }));
  coffee.rotation.x = -Math.PI / 2;
  coffee.position.y = 0.075;
  group.add(coffee);
  return { group, steamOrigin: new THREE.Vector3(0, 0.08, 0) };
}

// ------------------------------------------------------------------ noodle cup

export function createNoodleCup(): THREE.Group {
  const [c, ctx] = makeCanvas(512, 256);
  ctx.fillStyle = '#d63a1f';
  ctx.fillRect(0, 0, 512, 256);
  ctx.fillStyle = '#fff3d6';
  ctx.fillRect(0, 70, 512, 110);
  ctx.font = '900 64px "Arial Black", Impact, sans-serif';
  ctx.fillStyle = '#d63a1f';
  ctx.textAlign = 'center';
  ctx.fillText('CUP O\' RAMEN', 256, 150);
  ctx.font = '700 22px Arial';
  ctx.fillStyle = '#ffe9b0';
  ctx.fillText('SPICY CHICKEN · 3 MIN', 256, 215);
  const pts = [[0, 0], [0.034, 0], [0.036, 0.004], [0.047, 0.098], [0.049, 0.1], [0, 0.1]].map(([x, y]) => new THREE.Vector2(x, y));
  const group = new THREE.Group();
  const cup = new THREE.Mesh(new THREE.LatheGeometry(pts, 36), new THREE.MeshStandardMaterial({ map: canvasTexture(c), roughness: 0.6 }));
  cup.castShadow = true;
  group.add(cup);
  // Peeled lid curling back.
  const lidGeo = new THREE.CircleGeometry(0.05, 32);
  const pos = lidGeo.attributes.position;
  for (let i = 0; i < pos.count; i++) {
    const x = pos.getX(i);
    const y = pos.getY(i);
    if (y > -0.01) pos.setZ(i, Math.pow(Math.max(0, y + 0.01), 1.6) * 3.5);
    void x;
  }
  lidGeo.computeVertexNormals();
  const lid = new THREE.Mesh(lidGeo, new THREE.MeshStandardMaterial({ color: 0xe9e4da, roughness: 0.4, metalness: 0.3, side: THREE.DoubleSide }));
  lid.rotation.x = -Math.PI / 2;
  lid.position.y = 0.101;
  group.add(lid);
  // Plastic fork.
  const fork = new THREE.Mesh(new THREE.BoxGeometry(0.004, 0.14, 0.002), new THREE.MeshStandardMaterial({ color: 0xf2f2f2, roughness: 0.4 }));
  fork.position.set(0.012, 0.12, 0.01);
  fork.rotation.z = 0.35;
  fork.castShadow = true;
  group.add(fork);
  return group;
}

// ------------------------------------------------------------------ phone

export interface Phone {
  group: THREE.Group;
  canvas: HTMLCanvasElement;
  ctx: CanvasRenderingContext2D;
  texture: THREE.CanvasTexture;
  light: THREE.PointLight;
  screenMat: THREE.MeshBasicMaterial;
}

export function createPhone(): Phone {
  const group = new THREE.Group();
  const body = new THREE.Mesh(
    new RoundedBoxGeometry(0.074, 0.008, 0.155, 4, 0.004),
    new THREE.MeshPhysicalMaterial({ color: 0x15161a, metalness: 0.5, roughness: 0.3, clearcoat: 1, clearcoatRoughness: 0.05 }),
  );
  body.position.y = 0.004;
  body.castShadow = true;
  group.add(body);
  const [canvas, ctx] = makeCanvas(360, 760);
  const texture = canvasTexture(canvas);
  const screenMat = new THREE.MeshBasicMaterial({ map: texture, toneMapped: false, color: 0x000000 });
  const screen = new THREE.Mesh(new THREE.PlaneGeometry(0.068, 0.146), screenMat);
  screen.rotation.x = -Math.PI / 2;
  screen.position.y = 0.0082;
  group.add(screen);
  const light = new THREE.PointLight(0x9fc4ff, 0, 0.8, 2);
  light.position.set(0, 0.05, 0);
  group.add(light);
  return { group, canvas, ctx, texture, light, screenMat };
}

// ------------------------------------------------------------------ paper

/** A sheet of paper with slight curl, drawn by `draw` on a canvas. */
export function createPaper(w: number, h: number, draw: (ctx: CanvasRenderingContext2D, W: number, H: number) => void, curl = 0.01): THREE.Mesh {
  const W = 700;
  const H = Math.round((700 * h) / w);
  const [c, ctx] = makeCanvas(W, H);
  const n = new Noise(19);
  const img = ctx.createImageData(W, H);
  for (let y = 0; y < H; y++)
    for (let x = 0; x < W; x++) {
      const v = 0.9 + 0.05 * n.fbm((x / W) * 20, (y / H) * 20, 3, 20, 20);
      const i = (y * W + x) * 4;
      img.data[i] = 238 * v;
      img.data[i + 1] = 233 * v;
      img.data[i + 2] = 220 * v;
      img.data[i + 3] = 255;
    }
  ctx.putImageData(img, 0, 0);
  draw(ctx, W, H);
  const geo = new THREE.PlaneGeometry(w, h, 12, 12);
  const pos = geo.attributes.position;
  for (let i = 0; i < pos.count; i++) {
    const x = pos.getX(i) / (w / 2);
    const y = pos.getY(i) / (h / 2);
    pos.setZ(i, curl * (x * x * 0.6 + Math.max(0, y) * y * 0.8));
  }
  geo.computeVertexNormals();
  const mesh = new THREE.Mesh(geo, new THREE.MeshStandardMaterial({ map: canvasTexture(c), roughness: 0.92, side: THREE.DoubleSide }));
  mesh.receiveShadow = true;
  mesh.castShadow = true;
  return mesh;
}

export function drawEvictionNotice(ctx: CanvasRenderingContext2D, W: number, H: number): void {
  ctx.fillStyle = '#1a1a1a';
  ctx.textAlign = 'center';
  ctx.font = '700 40px Georgia, serif';
  ctx.fillText('NOTICE TO PAY RENT', W / 2, 80);
  ctx.fillText('OR QUIT', W / 2, 128);
  ctx.font = '400 20px Georgia, serif';
  ctx.textAlign = 'left';
  const lines = [
    'TO THE TENANT IN POSSESSION OF UNIT 3C:',
    '',
    'You are hereby notified that rent is past due',
    'for the premises you occupy, in the amount of',
    '$1,150.00 plus a late fee of $75.00.',
    '',
    'Within FOURTEEN (14) days you must pay the',
    'amount due in full, or vacate and surrender',
    'possession of the premises.',
    '',
    'Failure to do so will result in legal',
    'proceedings to recover possession.',
  ];
  lines.forEach((l, i) => ctx.fillText(l, 60, 200 + i * 30));
  ctx.save();
  ctx.translate(W * 0.62, H * 0.78);
  ctx.rotate(-0.22);
  ctx.strokeStyle = 'rgba(190,20,20,0.85)';
  ctx.lineWidth = 7;
  ctx.strokeRect(-160, -48, 320, 96);
  ctx.fillStyle = 'rgba(190,20,20,0.85)';
  ctx.font = '900 50px Impact, "Arial Black", sans-serif';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  ctx.fillText('FINAL NOTICE', 0, 4);
  ctx.restore();
}

export function createStickyNote(text: string[], color = '#f6e27a', rot = 0): THREE.Mesh {
  const [c, ctx] = makeCanvas(256, 256);
  ctx.fillStyle = color;
  ctx.fillRect(0, 0, 256, 256);
  const g = ctx.createLinearGradient(0, 0, 0, 256);
  g.addColorStop(0, 'rgba(0,0,0,0.08)');
  g.addColorStop(0.2, 'rgba(0,0,0,0)');
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, 256, 256);
  ctx.fillStyle = '#1d2a6b';
  ctx.font = `600 40px ${HAND}`;
  ctx.textAlign = 'left';
  text.forEach((t, i) => ctx.fillText(t, 18, 62 + i * 46));
  const geo = new THREE.PlaneGeometry(0.076, 0.076, 4, 4);
  const pos = geo.attributes.position;
  for (let i = 0; i < pos.count; i++) {
    const y = pos.getY(i) / 0.038;
    pos.setZ(i, Math.max(0, -y) * Math.max(0, -y) * 0.006);
  }
  geo.computeVertexNormals();
  const m = new THREE.Mesh(geo, new THREE.MeshStandardMaterial({ map: canvasTexture(c), roughness: 0.85, side: THREE.DoubleSide }));
  m.rotation.z = rot;
  return m;
}

export function createPoster(): THREE.Mesh {
  const [c, ctx] = makeCanvas(600, 900);
  const g = ctx.createLinearGradient(0, 0, 0, 900);
  g.addColorStop(0, '#0b0b10');
  g.addColorStop(1, '#1a130a');
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, 600, 900);
  // Spotlit table silhouette.
  const rg = ctx.createRadialGradient(300, 520, 20, 300, 520, 300);
  rg.addColorStop(0, 'rgba(255,210,120,0.55)');
  rg.addColorStop(1, 'rgba(255,210,120,0)');
  ctx.fillStyle = rg;
  ctx.fillRect(0, 200, 600, 700);
  ctx.fillStyle = '#0d3b2a';
  ctx.beginPath();
  ctx.ellipse(300, 600, 230, 80, 0, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillStyle = '#c9a44c';
  ctx.textAlign = 'center';
  ctx.font = '700 30px Georgia, serif';
  ctx.fillText('THE GRAND CIRCUIT', 300, 110);
  ctx.font = '900 70px Georgia, serif';
  ctx.fillText('CHAMPIONSHIP', 300, 190);
  ctx.font = '700 44px Georgia, serif';
  ctx.fillText('$10,000 MAIN EVENT', 300, 250);
  ctx.font = '400 22px Georgia, serif';
  ctx.fillStyle = '#b8a47a';
  ctx.fillText('ONE TABLE. NINE SEATS. ONE CHAMPION.', 300, 800);
  // Wear: creases and a torn corner.
  ctx.strokeStyle = 'rgba(255,255,255,0.08)';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(0, 450);
  ctx.lineTo(600, 430);
  ctx.moveTo(300, 0);
  ctx.lineTo(310, 900);
  ctx.stroke();
  ctx.globalCompositeOperation = 'destination-out';
  ctx.beginPath();
  ctx.moveTo(600, 0);
  ctx.lineTo(520, 0);
  ctx.lineTo(600, 90);
  ctx.fill();
  const tex = canvasTexture(c);
  const m = new THREE.Mesh(new THREE.PlaneGeometry(0.6, 0.9), new THREE.MeshStandardMaterial({ map: tex, roughness: 0.55, transparent: true, alphaTest: 0.5 }));
  m.receiveShadow = true;
  return m;
}
