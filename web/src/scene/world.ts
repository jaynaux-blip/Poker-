import * as THREE from 'three';
import { Apartment, createApartment } from './apartment';
import { SeatCamera } from './camera';
import { OUTSIDE_LAYER, Outside, createOutside } from './outside';
import { Post, createPost } from './post';

/** Renderer, scene graph and frame loop for the apartment. */
export class World {
  readonly renderer: THREE.WebGLRenderer;
  readonly scene = new THREE.Scene();
  readonly apartment: Apartment;
  readonly outside: Outside;
  readonly seat: SeatCamera;
  readonly post: Post;
  time = 0;
  private flash = 0;
  private nextLightning = 25;
  onThunder: ((delay: number, strength: number) => void) | null = null;
  private raycaster = new THREE.Raycaster();
  private width = 1;
  private height = 1;
  pixelRatio = 1;
  debug: string | null = null;

  constructor(canvas: HTMLCanvasElement, uiTexture: THREE.Texture) {
    this.renderer = new THREE.WebGLRenderer({ canvas, antialias: false, powerPreference: 'high-performance' });
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 1.05;
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;
    this.renderer.shadowMap.enabled = true;
    this.renderer.shadowMap.type = THREE.PCFShadowMap;
    this.scene.background = new THREE.Color(0x000000);

    this.apartment = createApartment(this.renderer, uiTexture);
    this.scene.add(this.apartment.group);
    this.outside = createOutside();
    this.scene.add(this.outside.group);
    // Outside objects render only into the window's offscreen target.
    this.outside.group.position.set(0, 0, 0);

    const lap = this.apartment.laptop;
    this.seat = new SeatCamera(this.apartment.eyePosition, lap.screenCenter, lap.screenNormal, lap.screenSize);
    this.post = createPost(this.renderer, this.scene, this.seat.camera);
    this.buildEnvironment();
  }

  /** Dim reflection environment: dark room with the window and screen as emitters. */
  private buildEnvironment(): void {
    const envScene = new THREE.Scene();
    envScene.background = new THREE.Color(0x020203);
    const box = new THREE.Mesh(new THREE.BoxGeometry(4, 2.6, 4), new THREE.MeshBasicMaterial({ color: 0x0b0b0d, side: THREE.BackSide }));
    envScene.add(box);
    const win = new THREE.Mesh(new THREE.PlaneGeometry(1.4, 1.1), new THREE.MeshBasicMaterial({ color: new THREE.Color(1.2, 0.35, 0.7) }));
    win.position.set(0, 0.3, -1.95);
    envScene.add(win);
    const scr = new THREE.Mesh(new THREE.PlaneGeometry(0.5, 0.3), new THREE.MeshBasicMaterial({ color: new THREE.Color(0.9, 1.0, 1.3) }));
    scr.position.set(0, -0.2, -1.0);
    envScene.add(scr);
    const pmrem = new THREE.PMREMGenerator(this.renderer);
    const env = pmrem.fromScene(envScene, 0.04).texture;
    this.scene.environment = env;
    this.scene.environmentIntensity = 0.5;
    pmrem.dispose();
  }

  resize(w: number, h: number, maxPixelRatio = 1.5): void {
    this.width = w;
    this.height = h;
    this.pixelRatio = Math.min(window.devicePixelRatio || 1, maxPixelRatio);
    this.renderer.setPixelRatio(this.pixelRatio);
    this.renderer.setSize(w, h, false);
    this.seat.setAspect(w / h);
    this.post.setSize(w * this.pixelRatio, h * this.pixelRatio);
    this.apartment.glass.resize(w * this.pixelRatio, h * this.pixelRatio);
  }

  /** Ray from a canvas-space pointer to the laptop screen; returns UV or null. */
  screenUV(clientX: number, clientY: number, rect: DOMRect): THREE.Vector2 | null {
    const ndc = new THREE.Vector2(((clientX - rect.left) / rect.width) * 2 - 1, -((clientY - rect.top) / rect.height) * 2 + 1);
    this.raycaster.setFromCamera(ndc, this.seat.camera);
    const hit = this.raycaster.intersectObject(this.apartment.laptop.screen, false)[0];
    return hit?.uv ? hit.uv.clone() : null;
  }

  setDawn(v: number): void {
    this.outside.setDawn(v);
  }

  lightning(strength = 1): void {
    this.flash = strength;
    this.outside.flash(strength);
    this.onThunder?.(0.6 + Math.random() * 2.2, strength);
  }

  frame(dt: number): void {
    dt = Math.max(0, dt);
    this.time += dt;
    const t = this.time;
    this.nextLightning -= dt;
    if (this.nextLightning <= 0) {
      this.nextLightning = 40 + Math.random() * 80;
      this.lightning(0.5 + Math.random() * 0.5);
    }
    this.flash = Math.max(0, this.flash - dt * 2.5);
    const flicker = this.flash > 0.05 ? this.flash * (0.5 + 0.5 * Math.sin(t * 55)) : 0;
    this.outside.update(dt, t);
    this.apartment.update(dt, t, this.outside.neonLevel(), flicker);
    this.seat.update(dt, t);

    // Pass 1: the world outside, into the window's target.
    const cam = this.seat.camera;
    const r = this.renderer;
    cam.layers.set(OUTSIDE_LAYER);
    r.setRenderTarget(this.apartment.glass.target);
    r.setClearColor(0x000000, 1);
    r.clear();
    r.render(this.scene, cam);
    r.setRenderTarget(null);
    cam.layers.set(0);

    // Pass 2: the room, through the post chain.
    if (this.debug === 'nopost') {
      r.render(this.scene, cam);
    } else if (this.debug === 'outside') {
      cam.layers.set(OUTSIDE_LAYER);
      r.render(this.scene, cam);
      cam.layers.set(0);
    } else this.post.render(t);
    void this.width;
    void this.height;
  }
}
