// End-to-end: real clicks through the intro and the in-world laptop UI.
import { chromium } from 'playwright';
const out = process.argv[2] ?? '/tmp/e2e';
const browser = await chromium.launch({ args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
const logs = [];
page.on('console', (m) => { if (['error', 'warning'].includes(m.type())) logs.push(`[${m.type()}] ${m.text()}`); });
page.on('pageerror', (e) => logs.push(`[pageerror] ${e.message}`));
await page.goto('http://localhost:5173/?speed=8');
await page.waitForFunction(() => window.__ready === true, null, { timeout: 120000 });
await page.fill('#name', 'e2e_tester');
await page.click('#begin');
// Wait for the camera to lean into the screen.
await page.waitForFunction(() => window.__game.world.seat.focus > 0.97, null, { timeout: 120000 });

// Project a UI-canvas point to page coordinates.
async function uiClick(x, y) {
  const pt = await page.evaluate(([x, y]) => {
    const g = window.__game;
    const scr = g.world.apartment.laptop.screen;
    const size = g.world.apartment.laptop.screenSize;
    const THREE = scr.position.constructor;
    const local = new THREE((x / 1600 - 0.5) * size.x, (0.5 - y / 1000) * size.y, 0);
    const world = scr.localToWorld(local);
    world.project(g.world.seat.camera);
    const r = g.world.renderer.domElement.getBoundingClientRect();
    return { x: r.left + (world.x * 0.5 + 0.5) * r.width, y: r.top + (-world.y * 0.5 + 0.5) * r.height };
  }, [x, y]);
  await page.mouse.move(pt.x, pt.y);
  await page.waitForTimeout(300);
  await page.mouse.down();
  await page.waitForTimeout(200);
  await page.mouse.up();
  await page.waitForTimeout(700);
}
await uiClick(800, 552); // Log in (boot card button)
let screen = await page.evaluate(() => window.__game.session.screen);
console.log('after login:', screen);
await page.screenshot({ path: `${out}-lobby.png` });
await uiClick(300, 234 + 62); // select Hyper Sprint row
await uiClick(1310, 88 + 560 + 32); // Register
await uiClick(1180, 88 + 560 + 32); // Confirm
screen = await page.evaluate(() => window.__game.session.screen);
console.log('after register:', screen, await page.evaluate(() => window.__game.session.t?.spec.name));
await page.waitForTimeout(4000);
await page.screenshot({ path: `${out}-table.png` });
// Fast-forward the tournament headlessly to reach results.
const res = await page.evaluate(async () => {
  const s = window.__game.session;
  s.pace = 'full';
  let t = s.now;
  for (let i = 0; i < 200000 && s.screen === 'table'; i++) {
    t += 0.1;
    s.update(t);
    if (s.prompt) s.heroAct(s.prompt.canCheck ? { type: 'check' } : { type: 'fold' });
  }
  return { screen: s.screen, place: s.results?.place, prize: s.results?.prizeCents, acc: s.results?.accuracy, bank: s.bankrollCents };
});
console.log('results:', JSON.stringify(res));
await page.waitForTimeout(3000);
await page.screenshot({ path: `${out}-results.png` });
console.log(logs.filter((l) => !l.includes('CERT') && !l.includes('404')).slice(0, 15).join('\n'));
await browser.close();
