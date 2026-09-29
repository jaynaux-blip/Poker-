import * as THREE from 'three';

/**
 * Seated first-person camera. Two poses blend smoothly:
 *   - screen: leaned in so the laptop fills the view (playing)
 *   - room: sitting back, free to look around the apartment
 * A little breathing sway keeps it from feeling like a static render.
 */
export class SeatCamera {
  readonly camera: THREE.PerspectiveCamera;
  focus = 1; // 0 = room, 1 = screen
  targetFocus = 1;
  yaw = 0;
  pitch = -0.12;
  private eye: THREE.Vector3;
  private screenCenter: THREE.Vector3;
  private screenNormal: THREE.Vector3;
  private screenSize: THREE.Vector2;
  private tmpPos = new THREE.Vector3();
  private tmpLook = new THREE.Vector3();
  private roomLook = new THREE.Vector3();
  /** Extra shake, e.g. heartbeat during an all-in. */
  shake = 0;
  /** Honors prefers-reduced-motion: no sway or shake. */
  reducedMotion = typeof matchMedia === 'function' && matchMedia('(prefers-reduced-motion: reduce)').matches;

  constructor(eye: THREE.Vector3, screenCenter: THREE.Vector3, screenNormal: THREE.Vector3, screenSize: THREE.Vector2) {
    this.camera = new THREE.PerspectiveCamera(50, 1, 0.02, 1200);
    this.camera.layers.enable(0);
    this.eye = eye.clone();
    this.screenCenter = screenCenter.clone();
    this.screenNormal = screenNormal.clone();
    this.screenSize = screenSize.clone();
  }

  setAspect(aspect: number): void {
    this.camera.aspect = aspect;
    // Wider field of view on tall (phone) screens so the laptop still fits.
    this.camera.fov = aspect < 1 ? 62 : 50;
    this.camera.updateProjectionMatrix();
  }

  /** Distance from the screen that frames it with a small margin. */
  private screenDistance(): number {
    const vfov = THREE.MathUtils.degToRad(this.camera.fov);
    const tan = Math.tan(vfov / 2);
    const byH = (this.screenSize.y * 0.5 * 1.1) / tan;
    const byW = (this.screenSize.x * 0.5 * 1.06) / (tan * this.camera.aspect);
    return Math.max(byH, byW);
  }

  update(dt: number, time: number): void {
    const k = 1 - Math.exp(-dt * 4.5);
    this.focus += (this.targetFocus - this.focus) * k;
    const t = this.focus * this.focus * (3 - 2 * this.focus);

    // Room pose.
    const cp = Math.cos(this.pitch);
    this.roomLook.set(Math.sin(this.yaw) * cp, Math.sin(this.pitch), -Math.cos(this.yaw) * cp).add(this.eye);

    // Screen pose.
    const d = this.screenDistance();
    const sp = this.tmpPos.copy(this.screenCenter).addScaledVector(this.screenNormal, d);

    const m = this.reducedMotion ? 0 : 1;
    const breathe = (Math.sin(time * 1.3) * 0.0022 + Math.sin(time * 0.37) * 0.0015) * m;
    const sway = Math.sin(time * 0.6) * 0.0012 * m;
    const shakeX = this.shake * (Math.sin(time * 37) * 0.0015) * m;
    const shakeY = this.shake * (Math.sin(time * 29 + 1) * 0.0015) * m;

    const pos = this.camera.position;
    pos.lerpVectors(this.eye, sp, t);
    pos.y += breathe * (1 - t * 0.7) + shakeY;
    pos.x += sway * (1 - t * 0.7) + shakeX;
    this.tmpLook.lerpVectors(this.roomLook, this.screenCenter, t);
    this.camera.lookAt(this.tmpLook);
  }
}
