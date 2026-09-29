import * as THREE from 'three';

/**
 * Rain-streaked window glass.
 *
 * The outside world is rendered to an offscreen target with mipmaps. The glass
 * samples it in screen space: fogged condensation reads from a blurry mip,
 * while water drops refract a sharp, offset view, and sliding drops wipe clear
 * trails through the fog. The droplet field is procedural (static beads that
 * swell and fade, plus stick-slip runners in columns).
 */

const vert = /* glsl */ `
varying vec2 vUv;
void main() {
  vUv = uv;
  gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
}`;

export const dropFieldGLSL = /* glsl */ `
float h11(float p) { p = fract(p * 0.1031); p *= p + 33.33; p *= p + p; return fract(p); }
vec2 h22(vec2 p) {
  vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.xx + p3.yz) * p3.zy);
}

// Static beads: returns (offset.xy, mask).
vec3 beads(vec2 uv, float t, float density) {
  vec2 g = uv * density;
  vec2 id = floor(g);
  vec3 acc = vec3(0.0);
  for (int j = -1; j <= 1; j++) {
    for (int i = -1; i <= 1; i++) {
      vec2 cid = id + vec2(float(i), float(j));
      vec2 r = h22(cid);
      vec2 c = cid + 0.5 + (r - 0.5) * 0.8;
      float life = fract(t * (0.05 + 0.08 * r.x) + r.y);
      float size = (0.06 + 0.3 * r.x * r.x) * smoothstep(0.0, 0.15, life) * (1.0 - smoothstep(0.85, 1.0, life)) * step(0.35, r.y);
      // No bead in this cell (also avoids smoothstep with equal edges).
      if (size < 0.01) continue;
      vec2 d = g - c;
      float dist = length(d);
      float m = smoothstep(size, size * 0.55, dist);
      acc.xy += d / max(size, 1e-3) * m;
      acc.z = max(acc.z, m);
    }
  }
  return acc;
}

// Runners: one drop per column that slides in jerks and leaves a clear trail.
// Returns (offset.xy, mask) in .xyz and trail mask in .w.
vec4 runners(vec2 uv, float t, float columns, float aspect) {
  float colW = 1.0 / columns;
  float ci = floor(uv.x / colW);
  vec4 acc = vec4(0.0);
  for (int k = -1; k <= 1; k++) {
    float c = ci + float(k);
    float r1 = h11(c * 3.7 + 1.3);
    float r2 = h11(c * 7.1 + 4.2);
    float period = 6.0 + 10.0 * r1;
    float p = fract(t / period + r2);
    // Stick-slip: hold, then lurch down.
    float steps = 5.0 + floor(r1 * 5.0);
    float sp = p * steps;
    float stepped = (floor(sp) + smoothstep(0.55, 1.0, fract(sp))) / steps;
    float y = 1.15 - 1.4 * stepped;
    float x = (c + 0.5 + (r2 - 0.5) * 0.5) * colW + 0.004 * sin(y * 40.0 + c);
    float rad = (0.008 + 0.01 * r2);
    vec2 d = vec2((uv.x - x) * aspect, (uv.y - y) * 1.0);
    d.y *= 0.8 + 0.4 * step(0.0, d.y); // teardrop: flatter on top
    float dist = length(d);
    float m = smoothstep(rad, rad * 0.5, dist);
    acc.xy += d / rad * m;
    acc.z = max(acc.z, m);
    // Trail above the drop, fading with height and time.
    float above = uv.y - y;
    float trailW = rad * 0.45 / aspect;
    float inTrail = smoothstep(trailW, trailW * 0.3, abs(uv.x - x)) * step(0.0, above) * (1.0 - smoothstep(0.0, 0.35, above));
    acc.w = max(acc.w, inTrail);
    // Tiny droplets left behind in the trail.
    vec2 tg = vec2((uv.x - x) * aspect, uv.y) * 140.0;
    vec2 tid = floor(tg);
    vec2 tf = fract(tg) - 0.5;
    float tr = h11(tid.y * 13.1 + c);
    float td = length(tf - vec2(0.0, (tr - 0.5) * 0.6));
    float tm = smoothstep(0.28, 0.12, td) * inTrail * step(0.55, tr);
    acc.xy += tf * 0.4 * tm;
    acc.z = max(acc.z, tm * 0.8);
  }
  return acc;
}
`;

const frag = /* glsl */ `
uniform sampler2D uOutside;
uniform vec2 uResolution;
uniform float uTime;
uniform float uAspect;
uniform float uFog;
uniform float uFlash;
varying vec2 vUv;
${dropFieldGLSL}
void main() {
  vec2 uv = vUv;
  vec3 b1 = beads(vec2(uv.x * uAspect, uv.y), uTime, 20.0);
  vec3 b2 = beads(vec2(uv.x * uAspect, uv.y) + 7.3, uTime * 0.7, 48.0) * 0.5;
  vec4 r1 = runners(uv, uTime, 22.0, uAspect);
  vec4 r2 = runners(uv + vec2(0.013, 0.0), uTime * 0.83 + 11.0, 37.0, uAspect);
  vec2 offs = b1.xy * b1.z + b2.xy * b2.z + r1.xy * r1.z + r2.xy * r2.z;
  float water = clamp(b1.z + b2.z + r1.z + r2.z, 0.0, 1.0);
  float clear = clamp(water + max(r1.w, r2.w), 0.0, 1.0);
  vec2 suv = gl_FragCoord.xy / uResolution;
  // Drops act like tiny lenses: invert and magnify what's behind them.
  vec2 refr = suv - offs * 0.011;
  float fogLod = uFog * (1.0 - clear);
  vec3 col = textureLod(uOutside, refr, fogLod).rgb;
  // Condensation scatters light: lift the fogged areas slightly.
  vec3 haze = textureLod(uOutside, suv, 5.5).rgb;
  col = mix(col, col * 0.75 + haze * 0.55, (1.0 - clear) * 0.55);
  // Drops pick up a little of the room's light on their lower edge.
  float rim = smoothstep(0.2, 0.6, water) * (1.0 - smoothstep(0.6, 1.0, water));
  col += rim * vec3(0.025, 0.03, 0.04);
  // Grime toward the frame.
  vec2 e = min(uv, 1.0 - uv);
  float edge = smoothstep(0.0, 0.06, min(e.x * uAspect, e.y));
  col *= mix(0.55, 1.0, edge);
  col += vec3(0.7, 0.75, 0.9) * uFlash * 0.3;
  gl_FragColor = vec4(col, 1.0);
}`;

export interface RainGlass {
  mesh: THREE.Mesh;
  target: THREE.WebGLRenderTarget;
  resize(w: number, h: number): void;
  update(time: number, flash: number): void;
}

export function createRainGlass(width: number, height: number): RainGlass {
  const target = new THREE.WebGLRenderTarget(512, 512, {
    type: THREE.HalfFloatType,
    generateMipmaps: true,
    minFilter: THREE.LinearMipmapLinearFilter,
    magFilter: THREE.LinearFilter,
  });
  target.texture.colorSpace = THREE.LinearSRGBColorSpace;
  const uniforms = {
    uOutside: { value: target.texture },
    uResolution: { value: new THREE.Vector2(1, 1) },
    uTime: { value: 0 },
    uAspect: { value: width / height },
    uFog: { value: 2.6 },
    uFlash: { value: 0 },
  };
  const mesh = new THREE.Mesh(
    new THREE.PlaneGeometry(width, height),
    new THREE.ShaderMaterial({ vertexShader: vert, fragmentShader: frag, uniforms }),
  );
  return {
    mesh,
    target,
    resize(w, h) {
      target.setSize(Math.max(64, Math.floor(w / 2)), Math.max(64, Math.floor(h / 2)));
      uniforms.uResolution.value.set(w, h);
    },
    update(time, flash) {
      uniforms.uTime.value = time;
      uniforms.uFlash.value = flash;
    },
  };
}

/**
 * The same droplet field rendered as a light cookie: projected through the
 * window by the neon spotlight so rain shadows crawl across the room.
 */
export function createRainCookie(renderer: THREE.WebGLRenderer): { texture: THREE.Texture; update(time: number): void } {
  const rt = new THREE.WebGLRenderTarget(256, 256, { type: THREE.UnsignedByteType });
  rt.texture.colorSpace = THREE.SRGBColorSpace;
  const uniforms = { uTime: { value: 0 } };
  const scene = new THREE.Scene();
  const cam = new THREE.OrthographicCamera(-1, 1, 1, -1, 0, 1);
  scene.add(
    new THREE.Mesh(
      new THREE.PlaneGeometry(2, 2),
      new THREE.ShaderMaterial({
        vertexShader: vert,
        fragmentShader: /* glsl */ `
          uniform float uTime;
          varying vec2 vUv;
          ${dropFieldGLSL}
          void main() {
            vec2 uv = vUv;
            vec3 b = beads(uv * vec2(1.2, 1.0), uTime, 22.0);
            vec4 r = runners(uv, uTime, 18.0, 1.2);
            float water = clamp(b.z + r.z, 0.0, 1.0);
            // Drops focus light into bright caustic points with dark rims.
            float caustic = smoothstep(0.6, 1.0, water) * 0.9;
            float rimShadow = smoothstep(0.05, 0.4, water) * (1.0 - smoothstep(0.5, 0.9, water));
            float v = 0.8 - rimShadow * 0.55 + caustic * 0.6 - r.w * 0.1;
            // Window frame: cross mullion and borders.
            vec2 e = min(uv, 1.0 - uv);
            float frame = step(0.035, e.x) * step(0.035, e.y) * step(0.012, abs(uv.x - 0.5));
            gl_FragColor = vec4(vec3(clamp(v, 0.0, 1.2) * frame), 1.0);
          }`,
        uniforms,
      }),
    ),
  );
  return {
    texture: rt.texture,
    update(time) {
      uniforms.uTime.value = time;
      const prev = renderer.getRenderTarget();
      renderer.setRenderTarget(rt);
      renderer.render(scene, cam);
      renderer.setRenderTarget(prev);
    },
  };
}
