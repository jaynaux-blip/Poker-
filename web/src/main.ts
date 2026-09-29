import * as THREE from 'three';
import { Sound } from './audio/sound';
import { H, UI, W } from './client/canvasui';
import { RiverLine } from './client/riverline';
import { setArtScale } from './client/cardart';
import { PhoneScreen } from './game/phone';
import { Session } from './game/session';
import { World } from './scene/world';

/**
 * SHORT STACK: Night One.
 * Boots the apartment scene, the RiverLine client on the laptop, audio, and
 * the game session, and routes input between the room and the screen.
 */

const canvas = document.getElementById('view') as HTMLCanvasElement;
const overlay = document.getElementById('overlay') as HTMLDivElement;
const params = new URLSearchParams(location.search);

const ui = new UI();
const uiTexture = new THREE.CanvasTexture(ui.canvas);
uiTexture.colorSpace = THREE.SRGBColorSpace;
uiTexture.anisotropy = 8;
uiTexture.generateMipmaps = true;
uiTexture.minFilter = THREE.LinearMipmapLinearFilter;
uiTexture.magFilter = THREE.LinearFilter;

const world = new World(canvas, uiTexture);
world.debug = params.get('debug');
const sound = new Sound();
const phone = new PhoneScreen(world.apartment.phone);

let now = 0;
let gameTime = 0;
const timeScale = Number(params.get('speed') ?? 1);
const toastEl = document.createElement('div');
toastEl.className = 'toast';
overlay.appendChild(toastEl);

const session = new Session({
  sound: (name, opts) => sound.play(name, opts),
  text: (from, body) => {
    phone.notify(from, body, now);
    sound.buzz();
    toastEl.innerHTML = `<div class="toast-app">Messages · now</div><div class="toast-from"></div><div class="toast-body"></div>`;
    (toastEl.querySelector('.toast-from') as HTMLElement).textContent = from;
    (toastEl.querySelector('.toast-body') as HTMLElement).textContent = body;
    toastEl.classList.remove('show');
    void toastEl.offsetWidth;
    toastEl.classList.add('show');
    clearTimeout((toastEl as unknown as { t: number }).t);
    (toastEl as unknown as { t: number }).t = window.setTimeout(() => toastEl.classList.remove('show'), 8000);
  },
  heartbeat: (on) => sound.heartbeat(on),
  addCan: () => {
    world.apartment.addCan();
    sound.canOpen();
  },
  celebrate: () => {
    sound.play('cash');
    sound.play('win');
  },
});
const client = new RiverLine(ui, session);
client.onLeanBack = () => toggleLean();
world.onThunder = (delay, strength) => sound.thunder(delay, strength);

// ------------------------------------------------------------------ intro

const intro = document.createElement('div');
intro.className = 'intro';
intro.innerHTML = `
  <div class="intro-inner">
    <div class="kicker">A first-person poker RPG · Prototype</div>
    <h1>SHORT STACK</h1>
    <div class="chapter">Night One</div>
    <p class="lede">2:07 AM. Rain on the window. <b>$2.37</b> in your RiverLine account and a final notice on the door.<br/>Rent is due Friday.</p>
    <label class="name">Screen name <input id="name" maxlength="16" spellcheck="false" autocomplete="off" /></label>
    <button id="begin">Begin</button>
    <div class="controls">
      <span><b>Mouse</b> play on the laptop</span>
      <span><b>Space</b> lean back and look around</span>
      <span><b>F · C · R · A</b> fold, call, raise, all-in</span>
      <span><b>↑ ↓</b> bet size</span>
      <span><b>M</b> mute</span>
    </div>
    <div class="fine">Headphones recommended. Everything you see and hear is generated in code: no photos, no samples.</div>
  </div>`;
overlay.appendChild(intro);
const nameInput = intro.querySelector('#name') as HTMLInputElement;
nameInput.value = session.heroName;

const hint = document.createElement('div');
hint.className = 'hint';
hint.textContent = 'Space: lean back · F C R A: fold / call / raise / all-in · ↑↓ bet size';
overlay.appendChild(hint);

let started = false;
// Opening shot: sitting back, looking at the rain.
world.seat.focus = 0;
world.seat.targetFocus = 0;
world.seat.yaw = 0.12;
world.seat.pitch = 0.05;

function begin(): void {
  if (started) return;
  started = true;
  const n = nameInput.value.trim().replace(/[^A-Za-z0-9_.-]/g, '').slice(0, 16);
  if (n.length >= 3) session.heroName = n;
  session.save();
  sound.start();
  intro.classList.add('gone');
  setTimeout(() => intro.remove(), 1600);
  setTimeout(() => {
    world.seat.targetFocus = 1;
    hint.classList.add('show');
    setTimeout(() => hint.classList.remove('show'), 14000);
  }, 1800);
}
(intro.querySelector('#begin') as HTMLButtonElement).addEventListener('click', begin);
nameInput.addEventListener('keydown', (e) => {
  if (e.key === 'Enter') begin();
  e.stopPropagation();
});

// ------------------------------------------------------------------ input

function toggleLean(): void {
  world.seat.targetFocus = world.seat.targetFocus > 0.5 ? 0 : 1;
  if (world.seat.targetFocus === 0) {
    world.seat.yaw = 0;
    world.seat.pitch = -0.1;
  }
}

let lastMouse = { x: 0, y: 0 };
function updatePointer(e: PointerEvent): void {
  const rect = canvas.getBoundingClientRect();
  lastMouse = { x: e.clientX, y: e.clientY };
  if (world.seat.focus > 0.85) {
    const uv = world.screenUV(e.clientX, e.clientY, rect);
    ui.pointer.active = !!uv;
    if (uv) {
      ui.pointer.x = uv.x * W;
      ui.pointer.y = (1 - uv.y) * H;
    }
  } else {
    ui.pointer.active = false;
    // Look around by pointing.
    const nx = (e.clientX - rect.left) / rect.width - 0.5;
    const ny = (e.clientY - rect.top) / rect.height - 0.5;
    world.seat.yaw = nx * 2.4;
    world.seat.pitch = -ny * 1.1 - 0.05;
  }
  canvas.style.cursor = ui.pointer.active ? 'none' : world.seat.focus > 0.85 ? 'default' : 'crosshair';
}

canvas.addEventListener('pointermove', updatePointer);
canvas.addEventListener('pointerdown', (e) => {
  updatePointer(e);
  if (!started) return;
  if (world.seat.focus < 0.5 && world.seat.targetFocus < 0.5) {
    // Clicking toward the laptop leans back in.
    const uv = world.screenUV(e.clientX, e.clientY, canvas.getBoundingClientRect());
    if (uv) world.seat.targetFocus = 1;
    return;
  }
  ui.pointer.down = true;
  ui.pointer.pressed = true;
  sound.play('click', { volume: 0.5 });
});
window.addEventListener('pointerup', (e) => {
  updatePointer(e);
  if (ui.pointer.down) ui.pointer.released = true;
  ui.pointer.down = false;
});
canvas.addEventListener('wheel', (e) => {
  ui.pointer.wheel += e.deltaY;
  e.preventDefault();
}, { passive: false });
canvas.addEventListener('contextmenu', (e) => e.preventDefault());
window.addEventListener('keydown', (e) => {
  if (!started) return;
  if (e.key === ' ') {
    toggleLean();
    e.preventDefault();
    return;
  }
  if (e.key === 'm' || e.key === 'M') {
    sound.setMuted(!sound.muted);
    return;
  }
  if (e.key.startsWith('Arrow')) e.preventDefault();
  client.key(e.key);
});

// ------------------------------------------------------------------ loop

// Resolution: render at the display's real pixel density (Retina included).
// The adaptive step only lowers it after startup if the GPU really can't keep
// up, never below 1x, and raises it again when there is headroom.
const dpr = window.devicePixelRatio || 1;
const prCap = Math.min(dpr, params.get('hq') ? 3 : 2);
const prFloor = Math.min(1, prCap);
let pixelRatio = prCap;
const resize = () => world.resize(window.innerWidth, window.innerHeight, pixelRatio);
window.addEventListener('resize', resize);
resize();

let last = performance.now();
let lastRaw = last;
let uiAccum = 1;
let perfTime = 0;
let perfFrames = 0;
let perfWarmup = 5;
let perfCooldown = 0;
let frames = 0;

// The laptop client's canvas is sized to how large the screen appears, so
// text maps 1:1 to screen pixels instead of being stretched.
const screenCorner = new THREE.Vector3();
let uiScaleWanted = 1;
let uiScaleSince = 0;
function screenPixelWidth(): number {
  const scr = world.apartment.laptop.screen;
  const half = world.apartment.laptop.screenSize.x / 2;
  const cam = world.seat.camera;
  const buf = world.renderer.domElement.width;
  screenCorner.set(-half, 0, 0);
  scr.localToWorld(screenCorner).project(cam);
  const x0 = screenCorner.x;
  screenCorner.set(half, 0, 0);
  scr.localToWorld(screenCorner).project(cam);
  return (Math.abs(screenCorner.x - x0) / 2) * buf;
}
function updateUiResolution(realDt: number): void {
  const focused = world.seat.focus >= 0.9;
  if (focused) {
    // Draw the client at exactly the size it appears, so one canvas pixel
    // lands on one screen pixel: canvas text stays hinted and crisp at any
    // window size or pixel density.
    const ratio = screenPixelWidth() / W;
    const wanted = Math.min(2, Math.max(0.5, Math.ceil(ratio * 100) / 100));
    if (Math.abs(wanted - uiScaleWanted) > 0.02) {
      uiScaleWanted = wanted;
      uiScaleSince = 0;
    }
    uiScaleSince += realDt;
  }
  // Mipmaps only while leaning back, when the screen is small in view.
  const mip = !focused;
  const resized = focused && uiScaleSince > 0.3 && ui.setScale(uiScaleWanted);
  if (resized || uiTexture.generateMipmaps !== mip) {
    setArtScale(ui.scale);
    uiTexture.generateMipmaps = mip;
    uiTexture.minFilter = mip ? THREE.LinearMipmapLinearFilter : THREE.LinearFilter;
    uiTexture.dispose();
    uiTexture.needsUpdate = true;
    uiAccum = 1; // redraw at the new size right away
  }
}

const screenTint = new THREE.Color();
const smoothstep = (a: number, b: number, x: number) => {
  const t = Math.max(0, Math.min(1, (x - a) / (b - a)));
  return t * t * (3 - 2 * t);
};

// Test hooks for automated screenshots.
const testShot = params.get('shot');
if (testShot === 'room') {
  started = true;
  intro.remove();
  world.seat.yaw = Number(params.get('yaw') ?? 0);
  world.seat.pitch = Number(params.get('pitch') ?? -0.1);
} else if (testShot) {
  started = true;
  intro.remove();
  world.seat.focus = world.seat.targetFocus = 1;
  session.screen = 'lobby';
  if (testShot === 'table') session.register(Number(params.get('event') ?? 0));
  const pace = params.get('pace');
  if (pace === 'full' || pace === 'smart') session.pace = pace;
}
(window as unknown as { __game: unknown }).__game = { world, session, ui };

function loop(t: number): void {
  const realDt = Math.max(0, (t - last) / 1000);
  const dt = Math.min(0.05, realDt);
  last = t;
  now += dt;

  const { beat } = sound.tick(dt);
  gameTime += (timeScale > 1 ? Math.min(0.25, (t - lastRaw) / 1000) : dt) * timeScale;
  lastRaw = t;
  session.update(gameTime);

  // UI at up to 30 fps.
  updateUiResolution(realDt);
  uiAccum += dt;
  if (uiAccum >= 1 / 30) {
    uiAccum = 0;
    ui.begin(gameTime);
    client.draw(gameTime);
    ui.end();
    uiTexture.needsUpdate = true;
  }

  // World reacts to the game.
  const clock = session.clockMinutes();
  world.setDawn(smoothstep(4.6 * 60, 6.3 * 60, clock));
  phone.update(now, clock);
  const lens = world.post.lens.uniforms;
  // Leaned in, keep the screen clean: no color fringing, lighter grain, a touch more sharpening.
  const f = world.seat.focus;
  lens.uAberration.value = 0.0022 * (1 - f);
  lens.uGrain.value = 0.03 - 0.018 * f;
  lens.uSharpen.value = 0.2 + 0.25 * f;
  lens.uTilt.value += (session.heroTilt * 0.85 - lens.uTilt.value) * Math.min(1, dt * 2);
  if (beat) lens.uPulse.value = 1;
  lens.uPulse.value = Math.max(0, lens.uPulse.value - dt * 3.5);
  world.seat.shake = lens.uPulse.value * 1.5;
  screenTint.setRGB(0.72, 0.84, 1.0);
  if (session.screen === 'table') screenTint.setRGB(0.55, 0.9, 0.8);
  world.apartment.setScreenGlow(screenTint, 1);

  world.frame(dt);
  frames++;
  // Adaptive resolution, measured on real frame time after a warm-up
  // (shader compiles and texture generation make the first seconds slow).
  if (perfWarmup > 0) perfWarmup -= realDt;
  else if (!testShot && !document.hidden) {
    perfTime += realDt;
    perfFrames++;
    perfCooldown -= realDt;
    if (perfTime > 3) {
      const fps = perfFrames / perfTime;
      if (fps < 28 && pixelRatio > prFloor) {
        pixelRatio = Math.max(prFloor, pixelRatio - 0.25);
        resize();
        perfCooldown = 6;
      } else if (fps > 55 && pixelRatio < prCap && perfCooldown <= 0) {
        pixelRatio = Math.min(prCap, pixelRatio + 0.25);
        resize();
        perfCooldown = 6;
      }
      perfTime = 0;
      perfFrames = 0;
    }
  }
  if (frames === 10) (window as unknown as { __ready: boolean }).__ready = true;
  requestAnimationFrame(loop);
}
requestAnimationFrame(loop);
void lastMouse;
