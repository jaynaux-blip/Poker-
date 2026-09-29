import * as THREE from 'three';
import { canvasTexture, makeCanvas, radialSprite } from './procedural';

/**
 * Everything beyond the window: sky, the city skyline, the building across
 * the street with its neon laundromat sign, and falling rain. All of it lives
 * on render layer 1 and is drawn into an offscreen target, which the rain-
 * streaked window glass then refracts and blurs.
 */
export const OUTSIDE_LAYER = 1;

export interface Outside {
  group: THREE.Group;
  update(dt: number, time: number): void;
  /** 0 = deep night, 1 = first light. */
  setDawn(v: number): void;
  /** Lightning flash strength 0..1 (decays on its own). */
  flash(strength: number): void;
  /** Neon intensity after flicker, for lighting the room. */
  neonLevel(): number;
}

const skyVert = /* glsl */ `
varying vec3 vDir;
void main() {
  vDir = normalize(position);
  gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
}`;

const skyFrag = /* glsl */ `
uniform float uTime;
uniform float uDawn;
uniform float uFlash;
varying vec3 vDir;
float hash(vec2 p) { vec3 p3 = fract(vec3(p.xyx) * 0.1031); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.x + p3.y) * p3.z); }
float noise(vec2 p) {
  vec2 i = floor(p), f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);
  return mix(mix(hash(i), hash(i + vec2(1, 0)), u.x), mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), u.x), u.y);
}
float fbm(vec2 p) {
  float s = 0.0, a = 0.5;
  for (int i = 0; i < 5; i++) { s += a * noise(p); p *= 2.03; a *= 0.5; }
  return s;
}
void main() {
  float h = clamp(vDir.y, -0.1, 1.0);
  // Night: sodium-orange light pollution at the horizon, deep blue overhead.
  vec3 nightHorizon = vec3(0.23, 0.13, 0.10);
  vec3 nightZenith = vec3(0.012, 0.018, 0.04);
  vec3 dawnHorizon = vec3(0.62, 0.42, 0.44);
  vec3 dawnZenith = vec3(0.10, 0.16, 0.30);
  vec3 horizon = mix(nightHorizon, dawnHorizon, uDawn);
  vec3 zenith = mix(nightZenith, dawnZenith, uDawn);
  vec3 col = mix(horizon, zenith, pow(h, 0.45));
  // Low rain clouds drifting, lit from below by the city.
  vec2 cp = vDir.xz / max(vDir.y, 0.05) * 1.2 + vec2(uTime * 0.012, uTime * 0.004);
  float c = fbm(cp);
  float cloud = smoothstep(0.35, 0.8, c);
  vec3 cloudCol = mix(vec3(0.16, 0.10, 0.09), vec3(0.45, 0.40, 0.45), uDawn);
  col = mix(col, cloudCol * (0.6 + 0.8 * (1.0 - h)), cloud * 0.75);
  col += vec3(0.55, 0.6, 0.75) * uFlash * (0.4 + cloud);
  gl_FragColor = vec4(col, 1.0);
}`;

const buildingVert = /* glsl */ `
attribute float aSeed;
varying vec3 vWorld;
varying vec3 vNormalW;
varying float vSeed;
varying vec3 vLocal;
varying vec3 vScale;
void main() {
  vec4 wp = modelMatrix * instanceMatrix * vec4(position, 1.0);
  vWorld = wp.xyz;
  vNormalW = normalize(mat3(modelMatrix * instanceMatrix) * normal);
  vSeed = aSeed;
  vLocal = position;
  vScale = vec3(length(instanceMatrix[0].xyz), length(instanceMatrix[1].xyz), length(instanceMatrix[2].xyz));
  gl_Position = projectionMatrix * viewMatrix * wp;
}`;

const buildingFrag = /* glsl */ `
uniform float uTime;
uniform float uDawn;
uniform float uFlash;
uniform vec3 uFogColor;
varying vec3 vWorld;
varying vec3 vNormalW;
varying float vSeed;
varying vec3 vLocal;
varying vec3 vScale;
float hash(vec2 p) { vec3 p3 = fract(vec3(p.xyx) * 0.1031); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.x + p3.y) * p3.z); }
void main() {
  float seed = floor(vSeed + 0.5);
  // Facade coordinates in meters.
  vec2 fc = abs(vNormalW.x) > 0.5 ? vec2(vLocal.z * vScale.z, vLocal.y * vScale.y) : vec2(vLocal.x * vScale.x, vLocal.y * vScale.y);
  bool roof = vNormalW.y > 0.5;
  vec2 cell = vec2(3.2, 3.4);
  vec2 id = floor(fc / cell);
  vec2 f = fract(fc / cell);
  float win = step(0.18, f.x) * step(f.x, 0.82) * step(0.25, f.y) * step(f.y, 0.8);
  float r = hash(id + seed * 17.0);
  float lit = step(0.62 - uDawn * 0.25, r);
  // Some windows flicker (TVs) and a few switch over time.
  float tv = step(0.97, hash(id * 1.7 + seed)) * (0.6 + 0.4 * sin(uTime * 7.0 + r * 40.0) * sin(uTime * 2.3 + r * 9.0));
  float slow = step(0.5, fract(r * 13.0 + uTime * 0.002));
  vec3 warm = mix(vec3(1.0, 0.62, 0.3), vec3(1.0, 0.82, 0.55), hash(id + 3.0));
  vec3 cool = vec3(0.55, 0.75, 1.0);
  vec3 wc = mix(warm, cool, step(0.8, hash(id + 5.0)));
  vec3 base = mix(vec3(0.025, 0.027, 0.035), vec3(0.16, 0.17, 0.2), uDawn) * (0.7 + 0.3 * hash(vec2(seed)));
  vec3 col = base;
  if (!roof) col += win * (lit * slow + tv) * wc * mix(1.1, 0.35, uDawn);
  col += vec3(0.5, 0.55, 0.7) * uFlash * 0.25;
  float dist = length(vWorld.xz);
  float fog = 1.0 - exp(-dist * 0.0055);
  col = mix(col, uFogColor, clamp(fog, 0.0, 0.92));
  gl_FragColor = vec4(col, 1.0);
}`;

const rainVert = /* glsl */ `
attribute vec3 aOffset;
attribute float aSpeed;
uniform float uTime;
varying float vAlpha;
void main() {
  vec3 p = aOffset;
  p.y = mod(aOffset.y - uTime * aSpeed, 14.0) - 7.0;
  // Streak: stretch the quad along the fall direction.
  vec3 pos = p + vec3(position.x * 0.0035, position.y * 0.45, 0.0);
  vec4 mv = modelViewMatrix * vec4(pos, 1.0);
  // Fade streaks right in front of the lens and far away.
  float dist = -mv.z;
  vAlpha = (0.35 + 0.65 * fract(aOffset.x * 13.7)) * smoothstep(2.0, 5.0, dist) * (1.0 - smoothstep(12.0, 16.0, dist));
  gl_Position = projectionMatrix * mv;
}`;

const rainFrag = /* glsl */ `
uniform vec3 uTint;
varying float vAlpha;
void main() {
  gl_FragColor = vec4(uTint, 0.1 * vAlpha);
}`;

function neonTexture(): THREE.CanvasTexture {
  const [c, ctx] = makeCanvas(1024, 320);
  ctx.fillStyle = '#000';
  ctx.fillRect(0, 0, 1024, 320);
  const glowText = (text: string, x: number, y: number, size: number, color: string, blur: number) => {
    ctx.font = `700 ${size}px "Arial Rounded MT Bold", "Trebuchet MS", sans-serif`;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.shadowColor = color;
    for (const b of [blur * 2, blur, blur * 0.4]) {
      ctx.shadowBlur = b;
      ctx.fillStyle = color;
      ctx.fillText(text, x, y);
    }
    ctx.shadowBlur = 0;
    ctx.fillStyle = '#fff';
    ctx.globalAlpha = 0.8;
    ctx.fillText(text, x, y);
    ctx.globalAlpha = 1;
  };
  glowText('WASH & FOLD', 512, 120, 130, '#ff2e88', 40);
  glowText('OPEN 24 HRS', 512, 250, 70, '#35d3ff', 26);
  return canvasTexture(c);
}

export function createOutside(): Outside {
  const group = new THREE.Group();
  const setLayer = (o: THREE.Object3D) => o.traverse((c) => c.layers.set(OUTSIDE_LAYER));

  // Sky dome.
  const skyUniforms = { uTime: { value: 0 }, uDawn: { value: 0 }, uFlash: { value: 0 } };
  const sky = new THREE.Mesh(
    new THREE.SphereGeometry(900, 32, 16),
    new THREE.ShaderMaterial({ vertexShader: skyVert, fragmentShader: skyFrag, uniforms: skyUniforms, side: THREE.BackSide, depthWrite: false }),
  );
  group.add(sky);

  // Skyline: instanced boxes with a procedural window shader.
  const count = 520;
  const geo = new THREE.BoxGeometry(1, 1, 1);
  geo.translate(0, 0.5, 0);
  const seeds = new Float32Array(count);
  const bUniforms = {
    uTime: { value: 0 },
    uDawn: { value: 0 },
    uFlash: { value: 0 },
    uFogColor: { value: new THREE.Color(0.2, 0.12, 0.1) },
  };
  const bMat = new THREE.ShaderMaterial({ vertexShader: buildingVert, fragmentShader: buildingFrag, uniforms: bUniforms });
  const buildings = new THREE.InstancedMesh(geo, bMat, count);
  const m = new THREE.Matrix4();
  let placed = 0;
  let s = 12345;
  const rnd = () => ((s = (s * 16807) % 2147483647) / 2147483647);
  const beacons: THREE.Vector3[] = [];
  while (placed < count) {
    const z = -40 - rnd() * 520;
    const x = (rnd() - 0.5) * 900;
    if (Math.abs(x) < 20 && z > -60) continue;
    const tall = rnd() < 0.08;
    const w = 12 + rnd() * 28;
    const d = 12 + rnd() * 28;
    const hgt = (tall ? 80 + rnd() * 160 : 15 + rnd() * 55) * (1 + Math.max(0, -z - 150) / 500);
    m.compose(new THREE.Vector3(x, -12, z), new THREE.Quaternion(), new THREE.Vector3(w, hgt, d));
    buildings.setMatrixAt(placed, m);
    seeds[placed] = rnd() * 100;
    if (tall) beacons.push(new THREE.Vector3(x, -12 + hgt + 1, z));
    placed++;
  }
  geo.setAttribute('aSeed', new THREE.InstancedBufferAttribute(seeds, 1));
  buildings.frustumCulled = false;
  group.add(buildings);

  // Red aviation beacons on the towers.
  const beaconMat = new THREE.MeshBasicMaterial({ color: 0xff2a1a, toneMapped: false });
  const beaconGeo = new THREE.SphereGeometry(0.9, 8, 6);
  const beaconMesh = new THREE.InstancedMesh(beaconGeo, beaconMat, beacons.length);
  beacons.forEach((p, i) => beaconMesh.setMatrixAt(i, new THREE.Matrix4().makeTranslation(p.x, p.y, p.z)));
  group.add(beaconMesh);

  // Building across the street, with the laundromat's neon sign.
  const acrossGeo = new THREE.BoxGeometry(1, 1, 1);
  acrossGeo.translate(0, 0.5, 0);
  acrossGeo.setAttribute('aSeed', new THREE.InstancedBufferAttribute(new Float32Array([42]), 1));
  const across = new THREE.InstancedMesh(
    acrossGeo,
    new THREE.ShaderMaterial({
      vertexShader: buildingVert,
      fragmentShader: buildingFrag,
      uniforms: { ...bUniforms, uFogColor: { value: new THREE.Color(0.05, 0.035, 0.03) } },
    }),
    1,
  );
  across.setMatrixAt(0, new THREE.Matrix4().compose(new THREE.Vector3(6, -16, -22), new THREE.Quaternion(), new THREE.Vector3(60, 40, 8)));
  across.frustumCulled = false;
  group.add(across);

  const neonTex = neonTexture();
  const neonMat = new THREE.MeshBasicMaterial({ map: neonTex, transparent: true, blending: THREE.AdditiveBlending, toneMapped: false, depthWrite: false });
  const neon = new THREE.Mesh(new THREE.PlaneGeometry(6.4, 2), neonMat);
  neon.position.set(3.2, -0.6, -17.9);
  group.add(neon);
  // Glow halo on the wet facade.
  const halo = new THREE.Mesh(
    new THREE.PlaneGeometry(14, 7),
    new THREE.MeshBasicMaterial({ map: radialSprite(128), color: 0xff2e88, transparent: true, opacity: 0.12, blending: THREE.AdditiveBlending, depthWrite: false, toneMapped: false }),
  );
  halo.position.set(3.2, -0.6, -17.95);
  group.add(halo);

  // Street lights far below.
  const lampMat = new THREE.MeshBasicMaterial({ color: 0xffa050, toneMapped: false });
  for (let i = 0; i < 6; i++) {
    const lamp = new THREE.Mesh(new THREE.SphereGeometry(0.25, 8, 6), lampMat);
    lamp.position.set(-18 + i * 9, -6.5, -12);
    group.add(lamp);
  }

  // Falling rain between the window and the building across the street.
  const rainCount = 1600;
  const rainGeo = new THREE.InstancedBufferGeometry();
  rainGeo.setAttribute('position', new THREE.Float32BufferAttribute([-0.5, -0.5, 0, 0.5, -0.5, 0, 0.5, 0.5, 0, -0.5, 0.5, 0], 3));
  rainGeo.setIndex([0, 1, 2, 0, 2, 3]);
  const offsets = new Float32Array(rainCount * 3);
  const speeds = new Float32Array(rainCount);
  for (let i = 0; i < rainCount; i++) {
    offsets[i * 3] = (rnd() - 0.5) * 16;
    offsets[i * 3 + 1] = rnd() * 14;
    offsets[i * 3 + 2] = -1.6 - rnd() * 14;
    speeds[i] = 7 + rnd() * 4;
  }
  rainGeo.setAttribute('aOffset', new THREE.InstancedBufferAttribute(offsets, 3));
  rainGeo.setAttribute('aSpeed', new THREE.InstancedBufferAttribute(speeds, 1));
  rainGeo.instanceCount = rainCount;
  const rainUniforms = { uTime: { value: 0 }, uTint: { value: new THREE.Color(0.75, 0.7, 0.85) } };
  const rain = new THREE.Mesh(
    rainGeo,
    new THREE.ShaderMaterial({ vertexShader: rainVert, fragmentShader: rainFrag, uniforms: rainUniforms, transparent: true, depthWrite: false, blending: THREE.AdditiveBlending }),
  );
  rain.frustumCulled = false;
  rain.position.y = 1;
  group.add(rain);

  setLayer(group);

  let flash = 0;
  let neonLevel = 1;
  let neonGlitch = 0;
  return {
    group,
    update(dt, time) {
      skyUniforms.uTime.value = time;
      bUniforms.uTime.value = time;
      rainUniforms.uTime.value = time;
      flash = Math.max(0, flash - dt * 2.2);
      const f = flash > 0.05 ? flash * (0.6 + 0.4 * Math.sin(time * 60)) : 0;
      skyUniforms.uFlash.value = f;
      bUniforms.uFlash.value = f;
      // The sign buzzes and occasionally drops out, like cheap neon does.
      neonGlitch -= dt;
      if (neonGlitch < -4 - Math.random() * 10) neonGlitch = 0.25 + Math.random() * 0.4;
      neonLevel = neonGlitch > 0 ? (Math.random() < 0.5 ? 0.15 : 1) : 0.92 + 0.08 * Math.sin(time * 120);
      neonMat.opacity = neonLevel;
      (halo.material as THREE.MeshBasicMaterial).opacity = 0.12 * neonLevel;
      const blink = Math.sin(time * 2.2) > 0.6 ? 1 : 0.05;
      beaconMat.color.setRGB(blink, blink * 0.16, blink * 0.1);
    },
    setDawn(v) {
      skyUniforms.uDawn.value = v;
      bUniforms.uDawn.value = v;
      bUniforms.uFogColor.value.setRGB(0.2 + 0.35 * v, 0.12 + 0.3 * v, 0.1 + 0.33 * v);
    },
    flash(strength) {
      flash = Math.max(flash, strength);
    },
    neonLevel: () => neonLevel,
  };
}
