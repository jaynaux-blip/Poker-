import * as THREE from 'three';
import { EffectComposer } from 'three/examples/jsm/postprocessing/EffectComposer.js';
import { OutputPass } from 'three/examples/jsm/postprocessing/OutputPass.js';
import { RenderPass } from 'three/examples/jsm/postprocessing/RenderPass.js';
import { ShaderPass } from 'three/examples/jsm/postprocessing/ShaderPass.js';
import { UnrealBloomPass } from 'three/examples/jsm/postprocessing/UnrealBloomPass.js';

/**
 * Post chain: HDR render → bloom (screen glow, neon, city lights) → filmic
 * tone mapping → a lens pass with chromatic aberration, vignette, grain, and
 * the "tilt" treatment (red-tinged tunnel vision and heartbeat pulse).
 */

const LensShader = {
  uniforms: {
    tDiffuse: { value: null as THREE.Texture | null },
    uTime: { value: 0 },
    uAberration: { value: 0.0022 },
    uVignette: { value: 0.42 },
    uGrain: { value: 0.045 },
    uTilt: { value: 0 },
    uPulse: { value: 0 },
    uFade: { value: 0 },
  },
  vertexShader: /* glsl */ `
    varying vec2 vUv;
    void main() { vUv = uv; gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0); }`,
  fragmentShader: /* glsl */ `
    uniform sampler2D tDiffuse;
    uniform float uTime, uAberration, uVignette, uGrain, uTilt, uPulse, uFade;
    varying vec2 vUv;
    float hash(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233)) + uTime * 3.1) * 43758.5453); }
    void main() {
      vec2 c = vUv - 0.5;
      float r2 = dot(c, c);
      float ab = uAberration * (1.0 + uTilt * 3.0 + uPulse * 2.0);
      vec2 dir = c * r2 * 2.0;
      vec3 col;
      col.r = texture2D(tDiffuse, vUv - dir * ab).r;
      col.g = texture2D(tDiffuse, vUv).g;
      col.b = texture2D(tDiffuse, vUv + dir * ab).b;
      float vig = 1.0 - smoothstep(0.18, 0.9, r2 * (uVignette * 2.2 + uTilt * 1.6 + uPulse * 0.8));
      col *= mix(0.25, 1.0, vig);
      // Tilt: color drains toward a hot red at the edges.
      float edge = smoothstep(0.08, 0.5, r2) * uTilt;
      float lum = dot(col, vec3(0.299, 0.587, 0.114));
      col = mix(col, vec3(lum * 1.2, lum * 0.35, lum * 0.3), edge * 0.8);
      float g = (hash(vUv * 1024.0) - 0.5) * uGrain;
      col += g * (1.0 - lum * 0.6);
      col *= 1.0 - uFade;
      gl_FragColor = vec4(col, 1.0);
    }`,
};

export interface Post {
  composer: EffectComposer;
  bloom: UnrealBloomPass;
  lens: ShaderPass;
  setSize(w: number, h: number): void;
  render(time: number): void;
}

export function createPost(renderer: THREE.WebGLRenderer, scene: THREE.Scene, camera: THREE.Camera): Post {
  const target = new THREE.WebGLRenderTarget(1, 1, { type: THREE.HalfFloatType, samples: 4 });
  const composer = new EffectComposer(renderer, target);
  composer.addPass(new RenderPass(scene, camera));
  const bloom = new UnrealBloomPass(new THREE.Vector2(512, 512), 0.42, 0.55, 0.9);
  composer.addPass(bloom);
  composer.addPass(new OutputPass());
  const lens = new ShaderPass(LensShader);
  composer.addPass(lens);
  return {
    composer,
    bloom,
    lens,
    setSize(w, h) {
      composer.setSize(w, h);
      bloom.resolution.set(w / 2, h / 2);
    },
    render(time) {
      lens.uniforms.uTime.value = time % 100;
      composer.render();
    },
  };
}
