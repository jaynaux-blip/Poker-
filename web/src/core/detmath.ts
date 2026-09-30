/**
 * Deterministic math shared with the C++ port (ShortStackCore).
 *
 * `Math.pow`, `exp` and `log` are computed differently by each JavaScript
 * engine and C runtime, so results can differ in the last bit and seeded
 * simulations would drift apart. These helpers use only multiplication and
 * square root, which IEEE 754 requires to be correctly rounded everywhere, so
 * TypeScript and C++ produce bit-identical results.
 */

/** x^n for a non-negative integer n, by repeated multiplication. */
export function powInt(x: number, n: number): number {
  let r = 1;
  for (let i = 0; i < n; i++) r *= x;
  return r;
}

/**
 * x^y for x >= 0 and y >= 0. The integer part of y uses repeated
 * multiplication; the fractional part uses a chain of square roots over its
 * first 16 binary digits (exponent error below 2^-16).
 */
export function detPow(x: number, y: number): number {
  if (y === 0) return 1;
  if (x <= 0) return 0;
  const whole = Math.floor(y);
  let r = powInt(x, whole);
  let frac = y - whole;
  let root = x;
  for (let i = 0; i < 16 && frac > 0; i++) {
    root = Math.sqrt(root);
    frac *= 2;
    if (frac >= 1) {
      r *= root;
      frac -= 1;
    }
  }
  return r;
}
