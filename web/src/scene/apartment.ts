import * as THREE from 'three';
import { RectAreaLightUniformsLib } from 'three/examples/jsm/lights/RectAreaLightUniformsLib.js';
import { RoundedBoxGeometry } from 'three/examples/jsm/geometries/RoundedBoxGeometry.js';
import { createRainCookie, createRainGlass, RainGlass } from './glass';
import { Noise, floorTextures, radialSprite, wallTextures, woodTextures } from './procedural';
import {
  Laptop, Phone, createCan, createLaptop, createMug, createNoodleCup, createPaper, createPhone, createPoster,
  createStickyNote, drawEvictionNotice,
} from './props';

/**
 * The studio apartment: a small, cold room on the third floor. The desk sits
 * under the only window; the laptop is the main light source, with pink neon
 * from the laundromat across the street spilling through rain-streaked glass.
 */

export const ROOM = {
  floor: 0,
  ceiling: 2.6,
  front: -0.95, // window wall
  back: 3.1,
  left: -1.75,
  right: 2.35,
  deskTop: 0.75,
  win: { x0: -0.72, x1: 0.72, y0: 0.98, y1: 2.12 },
};

export interface Apartment {
  group: THREE.Group;
  laptop: Laptop;
  phone: Phone;
  glass: RainGlass;
  neonSpot: THREE.SpotLight;
  screenLight: THREE.RectAreaLight;
  eyePosition: THREE.Vector3;
  update(dt: number, time: number, neon: number, flash: number): void;
  /** Put another empty can on the desk (one per hour of grinding). */
  addCan(): void;
  setScreenGlow(color: THREE.Color, brightness: number): void;
}

function planeWithHole(w: number, h: number, hole: { x0: number; x1: number; y0: number; y1: number }): THREE.BufferGeometry {
  const shape = new THREE.Shape();
  shape.moveTo(-w / 2, 0);
  shape.lineTo(w / 2, 0);
  shape.lineTo(w / 2, h);
  shape.lineTo(-w / 2, h);
  shape.lineTo(-w / 2, 0);
  const path = new THREE.Path();
  path.moveTo(hole.x0, hole.y0);
  path.lineTo(hole.x0, hole.y1);
  path.lineTo(hole.x1, hole.y1);
  path.lineTo(hole.x1, hole.y0);
  path.lineTo(hole.x0, hole.y0);
  shape.holes.push(path);
  const geo = new THREE.ShapeGeometry(shape);
  // UVs in meters / 3.5 so the wall texture keeps its scale.
  const pos = geo.attributes.position;
  const uv = geo.attributes.uv;
  for (let i = 0; i < pos.count; i++) uv.setXY(i, (pos.getX(i) + w / 2) / 3.5, pos.getY(i) / 2.6);
  return geo;
}

function wallPlane(w: number, h: number): THREE.PlaneGeometry {
  const geo = new THREE.PlaneGeometry(w, h);
  const uv = geo.attributes.uv;
  for (let i = 0; i < uv.count; i++) uv.setXY(i, uv.getX(i) * (w / 3.5), uv.getY(i));
  return geo;
}

export function createApartment(renderer: THREE.WebGLRenderer, uiTexture: THREE.Texture): Apartment {
  RectAreaLightUniformsLib.init();
  const group = new THREE.Group();
  const R = ROOM;
  const W = R.right - R.left;
  const D = R.back - R.front;
  const H = R.ceiling;
  const cx = (R.left + R.right) / 2;
  const cz = (R.front + R.back) / 2;

  // ---------------------------------------------------------------- shell
  const wallSet = wallTextures();
  const wallMat = new THREE.MeshStandardMaterial({ ...wallSet, color: 0xffffff, roughness: 1 });
  const floorSet = floorTextures();
  for (const t of [floorSet.map, floorSet.roughnessMap, floorSet.normalMap]) t.repeat.set(2, 2);
  const floorMat = new THREE.MeshStandardMaterial({ ...floorSet, roughness: 1 });

  const floor = new THREE.Mesh(new THREE.PlaneGeometry(W, D), floorMat);
  floor.rotation.x = -Math.PI / 2;
  floor.position.set(cx, R.floor, cz);
  floor.receiveShadow = true;
  group.add(floor);

  const ceiling = new THREE.Mesh(new THREE.PlaneGeometry(W, D), new THREE.MeshStandardMaterial({ color: 0x8f8d86, roughness: 0.95 }));
  ceiling.rotation.x = Math.PI / 2;
  ceiling.position.set(cx, H, cz);
  ceiling.receiveShadow = true;
  group.add(ceiling);

  // Front wall with the window opening (shape centered at x = 0).
  const frontW = W;
  const front = new THREE.Mesh(
    planeWithHole(frontW, H, { x0: R.win.x0 - cx, x1: R.win.x1 - cx, y0: R.win.y0, y1: R.win.y1 }),
    wallMat,
  );
  front.position.set(cx, 0, R.front);
  front.receiveShadow = true;
  group.add(front);

  const back = new THREE.Mesh(wallPlane(W, H), wallMat);
  back.rotation.y = Math.PI;
  back.position.set(cx, H / 2, R.back);
  back.receiveShadow = true;
  group.add(back);
  const left = new THREE.Mesh(wallPlane(D, H), wallMat);
  left.rotation.y = Math.PI / 2;
  left.position.set(R.left, H / 2, cz);
  left.receiveShadow = true;
  group.add(left);
  const right = new THREE.Mesh(wallPlane(D, H), wallMat);
  right.rotation.y = -Math.PI / 2;
  right.position.set(R.right, H / 2, cz);
  right.receiveShadow = true;
  group.add(right);

  // Baseboards.
  const trimMat = new THREE.MeshStandardMaterial({ color: 0x6f6a5f, roughness: 0.6 });
  const addTrim = (w: number, x: number, z: number, ry: number) => {
    const b = new THREE.Mesh(new THREE.BoxGeometry(w, 0.1, 0.015), trimMat);
    b.position.set(x, 0.05, z);
    b.rotation.y = ry;
    b.receiveShadow = true;
    group.add(b);
  };
  addTrim(W, cx, R.back - 0.008, 0);
  addTrim(D, R.left + 0.008, cz, Math.PI / 2);
  addTrim(D, R.right - 0.008, cz, Math.PI / 2);

  // ---------------------------------------------------------------- window
  const wx = (R.win.x0 + R.win.x1) / 2;
  const wy = (R.win.y0 + R.win.y1) / 2;
  const ww = R.win.x1 - R.win.x0;
  const wh = R.win.y1 - R.win.y0;
  const depth = 0.16;
  const revealMat = new THREE.MeshStandardMaterial({ ...wallSet, roughness: 1 });
  const reveal = (w: number, h: number, x: number, y: number, z: number, rx: number, ry: number) => {
    const m = new THREE.Mesh(new THREE.PlaneGeometry(w, h), revealMat);
    m.position.set(x, y, z);
    m.rotation.set(rx, ry, 0);
    m.receiveShadow = true;
    group.add(m);
  };
  reveal(ww, depth, wx, R.win.y1, R.front - depth / 2, Math.PI / 2, 0); // head
  reveal(depth, wh, R.win.x0, wy, R.front - depth / 2, 0, Math.PI / 2);
  reveal(depth, wh, R.win.x1, wy, R.front - depth / 2, 0, -Math.PI / 2);
  // Sill.
  const sill = new THREE.Mesh(new THREE.BoxGeometry(ww + 0.12, 0.03, depth + 0.07), new THREE.MeshStandardMaterial({ color: 0x9a968c, roughness: 0.5 }));
  sill.position.set(wx, R.win.y0 - 0.015, R.front - depth / 2 + 0.035);
  sill.castShadow = true;
  sill.receiveShadow = true;
  group.add(sill);
  // Frame and mullion (old painted wood).
  const frameMat = new THREE.MeshStandardMaterial({ color: 0xd8d3c6, roughness: 0.55 });
  const frameZ = R.front - depth + 0.03;
  const bar = (w: number, h: number, x: number, y: number) => {
    const b = new THREE.Mesh(new THREE.BoxGeometry(w, h, 0.05), frameMat);
    b.position.set(x, y, frameZ);
    b.castShadow = true;
    b.receiveShadow = true;
    group.add(b);
  };
  bar(ww, 0.05, wx, R.win.y0 + 0.025);
  bar(ww, 0.05, wx, R.win.y1 - 0.025);
  bar(0.05, wh, R.win.x0 + 0.025, wy);
  bar(0.05, wh, R.win.x1 - 0.025, wy);
  bar(0.04, wh, wx, wy); // mullion
  bar(ww, 0.035, wx, wy + 0.08); // transom rail

  const glass = createRainGlass(ww - 0.06, wh - 0.06);
  glass.mesh.position.set(wx, wy, frameZ - 0.01);
  group.add(glass.mesh);

  // ---------------------------------------------------------------- desk
  const wood = woodTextures();
  const deskMat = new THREE.MeshPhysicalMaterial({ ...wood, roughness: 1, clearcoat: 0.25, clearcoatRoughness: 0.5 });
  const deskW = 1.6;
  const deskD = 0.7;
  const deskZ = R.front + 0.02 + deskD / 2;
  const top = new THREE.Mesh(new RoundedBoxGeometry(deskW, 0.035, deskD, 2, 0.006), deskMat);
  top.position.set(0, R.deskTop - 0.0175, deskZ);
  top.castShadow = true;
  top.receiveShadow = true;
  group.add(top);
  const legMat = new THREE.MeshStandardMaterial({ color: 0x1c1d20, metalness: 0.7, roughness: 0.5 });
  for (const [x, z] of [[-0.76, deskZ - 0.3], [0.76, deskZ - 0.3], [-0.76, deskZ + 0.3], [0.76, deskZ + 0.3]]) {
    const leg = new THREE.Mesh(new THREE.BoxGeometry(0.035, R.deskTop - 0.035, 0.035), legMat);
    leg.position.set(x, (R.deskTop - 0.035) / 2, z);
    leg.castShadow = true;
    group.add(leg);
  }

  // ---------------------------------------------------------------- laptop
  const laptop = createLaptop(uiTexture);
  laptop.group.position.set(0, R.deskTop, deskZ + 0.04);
  group.add(laptop.group);
  group.updateMatrixWorld(true);
  laptop.screen.getWorldPosition(laptop.screenCenter);
  laptop.screenNormal.set(0, 0, 1).applyQuaternion(laptop.screen.getWorldQuaternion(new THREE.Quaternion()));

  // Mouse and pad.
  const padMesh = new THREE.Mesh(new THREE.BoxGeometry(0.26, 0.003, 0.21), new THREE.MeshStandardMaterial({ color: 0x131417, roughness: 0.95 }));
  padMesh.position.set(0.33, R.deskTop + 0.0015, deskZ + 0.13);
  padMesh.receiveShadow = true;
  group.add(padMesh);
  const mouseGeo = new THREE.SphereGeometry(0.03, 24, 16);
  mouseGeo.scale(1, 0.45, 1.65);
  const mouse = new THREE.Mesh(mouseGeo, new THREE.MeshPhysicalMaterial({ color: 0x1d1e22, roughness: 0.35, clearcoat: 0.6 }));
  mouse.position.set(0.34, R.deskTop + 0.012, deskZ + 0.15);
  mouse.castShadow = true;
  group.add(mouse);

  // ---------------------------------------------------------------- props
  const cans: THREE.Group[] = [];
  const canSpots: [number, number, number][] = [
    [0.52, deskZ - 0.2, 0.3], [0.61, deskZ - 0.14, 1.2], [0.47, deskZ - 0.08, 2.1], [0.66, deskZ - 0.26, 0.7],
    [0.56, deskZ - 0.3, 2.9], [0.7, deskZ - 0.05, 1.8], [0.42, deskZ - 0.28, 0.2], [-0.62, deskZ + 0.05, 1.1],
    [-0.7, deskZ - 0.08, 2.4], [0.72, deskZ + 0.1, 0.5],
  ];
  const addCan = () => {
    if (cans.length >= canSpots.length) return;
    const [x, z, r] = canSpots[cans.length];
    const can = createCan(true);
    can.position.set(x, R.deskTop, z);
    can.rotation.y = r;
    group.add(can);
    cans.push(can);
  };
  addCan();

  const mug = createMug();
  mug.group.position.set(-0.4, R.deskTop, deskZ + 0.06);
  mug.group.rotation.y = 2.4;
  group.add(mug.group);

  const noodles = createNoodleCup();
  noodles.position.set(-0.58, R.deskTop, deskZ - 0.18);
  group.add(noodles);

  const notice = createPaper(0.21, 0.297, drawEvictionNotice, 0.012);
  notice.rotation.x = -Math.PI / 2;
  notice.rotation.z = 0.35;
  notice.position.set(-0.33, R.deskTop + 0.002, deskZ + 0.18);
  group.add(notice);
  const bills = createPaper(0.2, 0.26, (ctx, W2) => {
    ctx.fillStyle = '#222';
    ctx.font = '700 30px Arial';
    ctx.fillText('CITY POWER & LIGHT', 40, 70);
    ctx.font = '400 20px Arial';
    ctx.fillText('PAST DUE — $186.42', 40, 120);
    ctx.fillStyle = '#b01717';
    ctx.fillRect(40, 140, W2 - 80, 4);
  }, 0.004);
  bills.rotation.x = -Math.PI / 2;
  bills.rotation.z = -0.2;
  bills.position.set(-0.55, R.deskTop + 0.001, deskZ + 0.14);
  group.add(bills);

  const phone = createPhone();
  phone.group.position.set(0.28, R.deskTop, deskZ - 0.12);
  phone.group.rotation.y = -0.25;
  group.add(phone.group);

  // Sticky notes on the wall and the laptop bezel.
  const notes: [string[], number, number, number, string][] = [
    [['BR: $2.37', 'DON\'T', 'TILT.'], -0.88, 1.38, 0.05, '#f6e27a'],
    [['fold the', 'trash.', 'shove', 'the good'], -0.95, 1.22, -0.08, '#9fe3b6'],
    [['RENT', 'FRIDAY', '$1,225'], 0.88, 1.33, 0.1, '#ff9cb8'],
  ];
  for (const [text, x, y, rot, col] of notes) {
    const n = createStickyNote(text, col, rot);
    n.position.set(x, y, R.front + 0.003);
    group.add(n);
  }

  // Desk lamp (off): a dark silhouette against the window.
  const lampMat = new THREE.MeshStandardMaterial({ color: 0x1a1a1c, metalness: 0.6, roughness: 0.4 });
  const lampBase = new THREE.Mesh(new THREE.CylinderGeometry(0.06, 0.07, 0.02, 24), lampMat);
  lampBase.position.set(-0.66, R.deskTop + 0.01, deskZ - 0.24);
  const arm1 = new THREE.Mesh(new THREE.CylinderGeometry(0.007, 0.007, 0.36, 8), lampMat);
  arm1.position.set(-0.66, R.deskTop + 0.18, deskZ - 0.26);
  arm1.rotation.x = -0.15;
  const arm2 = new THREE.Mesh(new THREE.CylinderGeometry(0.007, 0.007, 0.3, 8), lampMat);
  arm2.position.set(-0.6, R.deskTop + 0.38, deskZ - 0.2);
  arm2.rotation.z = -1.0;
  const shade = new THREE.Mesh(new THREE.ConeGeometry(0.06, 0.1, 24, 1, true), lampMat);
  shade.position.set(-0.49, R.deskTop + 0.42, deskZ - 0.2);
  shade.rotation.z = 2.3;
  for (const m of [lampBase, arm1, arm2, shade]) {
    m.castShadow = true;
    group.add(m);
  }

  // Poster on the left wall: the dream.
  const poster = createPoster();
  poster.position.set(R.left + 0.005, 1.55, 0.55);
  poster.rotation.y = Math.PI / 2;
  poster.rotation.z = 0.02;
  group.add(poster);

  // Mattress on the floor with a rumpled blanket.
  const mattress = new THREE.Mesh(
    new RoundedBoxGeometry(0.95, 0.2, 1.95, 3, 0.05),
    new THREE.MeshStandardMaterial({ color: 0xc9c2b5, roughness: 0.95 }),
  );
  mattress.position.set(R.left + 0.6, 0.1, R.back - 1.1);
  mattress.castShadow = true;
  mattress.receiveShadow = true;
  group.add(mattress);
  const blanketGeo = new THREE.PlaneGeometry(1.0, 1.4, 40, 40);
  const bn = new Noise(31);
  const bp = blanketGeo.attributes.position;
  for (let i = 0; i < bp.count; i++) {
    const x = bp.getX(i);
    const y = bp.getY(i);
    const edge = Math.max(Math.abs(x) - 0.45, 0) * 2;
    bp.setZ(i, bn.fbm(x * 3 + 5, y * 3, 4) * 0.12 - edge * 0.2);
  }
  blanketGeo.computeVertexNormals();
  const blanket = new THREE.Mesh(blanketGeo, new THREE.MeshStandardMaterial({ color: 0x2c3b52, roughness: 1, side: THREE.DoubleSide }));
  blanket.rotation.x = -Math.PI / 2;
  blanket.position.set(R.left + 0.6, 0.22, R.back - 0.95);
  blanket.castShadow = true;
  blanket.receiveShadow = true;
  group.add(blanket);

  // Door on the back wall.
  const doorMat = new THREE.MeshStandardMaterial({ color: 0x4a3a2c, roughness: 0.7 });
  const door = new THREE.Mesh(new THREE.BoxGeometry(0.9, 2.05, 0.05), doorMat);
  door.position.set(1.35, 1.025, R.back - 0.03);
  door.receiveShadow = true;
  group.add(door);
  const knob = new THREE.Mesh(new THREE.SphereGeometry(0.03, 16, 12), new THREE.MeshStandardMaterial({ color: 0xb08d4a, metalness: 1, roughness: 0.3 }));
  knob.position.set(1.0, 1.0, R.back - 0.07);
  group.add(knob);
  const doorNotice = createPaper(0.21, 0.297, drawEvictionNotice, 0.004);
  doorNotice.position.set(1.35, 1.5, R.back - 0.06);
  doorNotice.rotation.y = Math.PI;
  group.add(doorNotice);

  // Bare bulb (off).
  const cord = new THREE.Mesh(new THREE.CylinderGeometry(0.003, 0.003, 0.4, 6), new THREE.MeshStandardMaterial({ color: 0x111111 }));
  cord.position.set(0.3, H - 0.2, 1.1);
  group.add(cord);
  const bulb = new THREE.Mesh(new THREE.SphereGeometry(0.035, 16, 12), new THREE.MeshPhysicalMaterial({ color: 0xffffff, roughness: 0.05, transmission: 0.6, thickness: 0.01 }));
  bulb.position.set(0.3, H - 0.43, 1.1);
  group.add(bulb);

  // ---------------------------------------------------------------- steam
  const steamTex = radialSprite(64, 'rgba(255,255,255,0.45)', 'rgba(255,255,255,0)');
  const steam: THREE.Sprite[] = [];
  const steamBase = new THREE.Vector3().copy(mug.steamOrigin).applyMatrix4(mug.group.matrixWorld);
  mug.group.updateMatrixWorld(true);
  steamBase.copy(mug.steamOrigin).applyMatrix4(mug.group.matrixWorld);
  for (let i = 0; i < 14; i++) {
    const s = new THREE.Sprite(new THREE.SpriteMaterial({ map: steamTex, transparent: true, depthWrite: false, opacity: 0 }));
    s.userData.phase = i / 14;
    steam.push(s);
    group.add(s);
  }

  // ---------------------------------------------------------------- dust
  // Motes drifting through the screen's light.
  const moteCount = 160;
  const moteGeo = new THREE.BufferGeometry();
  const motePos = new Float32Array(moteCount * 3);
  const moteSeed = new Float32Array(moteCount);
  for (let i = 0; i < moteCount; i++) {
    motePos[i * 3] = (Math.random() - 0.5) * 0.9;
    motePos[i * 3 + 1] = R.deskTop + 0.05 + Math.random() * 0.55;
    motePos[i * 3 + 2] = deskZ + 0.05 + Math.random() * 0.55;
    moteSeed[i] = Math.random() * 100;
  }
  moteGeo.setAttribute('position', new THREE.BufferAttribute(motePos, 3));
  moteGeo.setAttribute('aSeed', new THREE.BufferAttribute(moteSeed, 1));
  const moteUniforms = { uTime: { value: 0 }, uScale: { value: 1 } };
  const motes = new THREE.Points(
    moteGeo,
    new THREE.ShaderMaterial({
      uniforms: moteUniforms,
      transparent: true,
      depthWrite: false,
      blending: THREE.AdditiveBlending,
      vertexShader: /* glsl */ `
        attribute float aSeed;
        uniform float uTime;
        uniform float uScale;
        varying float vA;
        void main() {
          vec3 p = position;
          p.x += sin(uTime * 0.07 + aSeed) * 0.05;
          p.y += sin(uTime * 0.05 + aSeed * 1.7) * 0.04;
          p.z += cos(uTime * 0.06 + aSeed * 0.3) * 0.05;
          vec4 mv = modelViewMatrix * vec4(p, 1.0);
          gl_Position = projectionMatrix * mv;
          gl_PointSize = uScale * 2.2 / max(0.2, -mv.z);
          vA = 0.25 + 0.75 * fract(aSeed * 7.3);
        }`,
      fragmentShader: /* glsl */ `
        varying float vA;
        void main() {
          vec2 c = gl_PointCoord - 0.5;
          float a = smoothstep(0.5, 0.0, length(c));
          gl_FragColor = vec4(vec3(0.75, 0.85, 1.0), a * vA * 0.35);
        }`,
    }),
  );
  motes.frustumCulled = false;
  group.add(motes);

  // ---------------------------------------------------------------- lights
  const screenLight = new THREE.RectAreaLight(0xb8cfff, 7, laptop.screenSize.x, laptop.screenSize.y);
  screenLight.position.copy(laptop.screenCenter).addScaledVector(laptop.screenNormal, 0.003);
  screenLight.lookAt(screenLight.position.clone().add(laptop.screenNormal));
  group.add(screenLight);

  // Neon through the window: shadows of the frame and moving rain on the room.
  const neonSpot = new THREE.SpotLight(0xff3a8c, 55, 16, 0.55, 0.65, 1.6);
  neonSpot.position.set(1.3, 1.25, -5.6);
  neonSpot.target.position.set(-0.35, 1.0, 2.0);
  neonSpot.castShadow = true;
  neonSpot.shadow.mapSize.set(1024, 1024);
  neonSpot.shadow.bias = -0.0004;
  neonSpot.shadow.normalBias = 0.02;
  neonSpot.shadow.camera.near = 3;
  neonSpot.shadow.camera.far = 14;
  const cookie = createRainCookie(renderer);
  neonSpot.map = cookie.texture;
  group.add(neonSpot, neonSpot.target);

  // Cool city/sky fill from the window.
  const cityFill = new THREE.RectAreaLight(0x5a78c8, 1.1, ww, wh);
  cityFill.position.set(wx, wy, R.front - 0.05);
  cityFill.lookAt(wx, wy - 0.3, 2);
  group.add(cityFill);
  const blueNeon = new THREE.PointLight(0x35d3ff, 0.25, 5, 2);
  blueNeon.position.set(0.5, 1.6, R.front - 0.4);
  group.add(blueNeon);

  const hemi = new THREE.HemisphereLight(0x223047, 0x0d0907, 0.12);
  group.add(hemi);

  const eyePosition = new THREE.Vector3(0, 1.17, deskZ + 0.72);
  const neonBase = neonSpot.intensity;
  const lampColor = new THREE.Color();

  return {
    group,
    laptop,
    phone,
    glass,
    neonSpot,
    screenLight,
    eyePosition,
    update(dt, time, neon, flash) {
      cookie.update(time);
      moteUniforms.uTime.value = time;
      moteUniforms.uScale.value = renderer.getPixelRatio() * renderer.domElement.height / 800 * 1.4;
      neonSpot.intensity = neonBase * neon + flash * 250;
      neonSpot.color.lerpColors(lampColor.set(0xff3a8c), new THREE.Color(0xd8e4ff), Math.min(1, flash * 1.5));
      hemi.intensity = 0.12 + flash * 1.5;
      glass.update(time, flash);
      for (const s of steam) {
        const p = (s.userData.phase + time * 0.12) % 1;
        s.position.set(
          steamBase.x + Math.sin(time * 1.3 + s.userData.phase * 20) * 0.012 * p * 3,
          steamBase.y + p * 0.18,
          steamBase.z + Math.cos(time * 0.9 + s.userData.phase * 11) * 0.01 * p * 3,
        );
        const sc = 0.03 + p * 0.07;
        s.scale.set(sc, sc, sc);
        (s.material as THREE.SpriteMaterial).opacity = Math.sin(p * Math.PI) * 0.07;
      }
      void dt;
    },
    addCan,
    setScreenGlow(color, brightness) {
      screenLight.color.copy(color);
      screenLight.intensity = 7 * brightness;
    },
  };
}
