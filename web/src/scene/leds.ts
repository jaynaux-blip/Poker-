/**
 * The GearDrop LED room kit's seven looks (mirrors gear::LedPresets in ShortStackCore's Gear.cpp):
 * the colours the room's strips and fill light take, and Aurora's slow drift.
 */
export interface LedPreset {
  id: string;
  name: string;
  rgb: number;
  cycles: boolean;
}

export const LED_PRESETS: LedPreset[] = [
  { id: 'felt', name: 'Felt Green', rgb: 0x19e68c, cycles: false },
  { id: 'ice', name: 'Ice Blue', rgb: 0x2fb4ff, cycles: false },
  { id: 'royal', name: 'Royal Violet', rgb: 0x9b5cff, cycles: false },
  { id: 'heater', name: 'Heater Red', rgb: 0xff2d55, cycles: false },
  { id: 'gold', name: 'Gold Rush', rgb: 0xffb21e, cycles: false },
  { id: 'afterhours', name: 'After Hours', rgb: 0xff3fb4, cycles: false },
  { id: 'aurora', name: 'Aurora', rgb: 0x19e68c, cycles: true },
];

const AURORA = [0x19e68c, 0x2fb4ff, 0x9b5cff, 0xff3fb4];

export function mixRgb(a: number, b: number, t: number): number {
  const u = Math.max(0, Math.min(1, t));
  let out = 0;
  for (const shift of [0, 8, 16]) {
    const ca = (a >> shift) & 0xff;
    const cb = (b >> shift) & 0xff;
    out |= Math.round(ca + (cb - ca) * u) << shift;
  }
  return out;
}

/** The colour of a preset at time t (seconds): solid colours hold, Aurora eases through its four stops. */
export function ledColor(index: number, time: number): number {
  const p = LED_PRESETS[Math.max(0, Math.min(LED_PRESETS.length - 1, index))];
  if (!p.cycles) return p.rgb;
  const pos = (Math.max(0, time) / 8) % 4;
  const k = Math.floor(pos);
  const f = pos - k;
  return mixRgb(AURORA[k % 4], AURORA[(k + 1) % 4], f * f * (3 - 2 * f));
}
