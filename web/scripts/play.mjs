// Scripted browser playthrough: node scripts/play.mjs <outPrefix> [event] [speed]
import { chromium } from 'playwright';
const [prefix, event = '1', speed = '6'] = process.argv.slice(2);
const browser = await chromium.launch({ args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
const logs = [];
page.on('console', (m) => { if (m.type() === 'error' || m.type() === 'warning') logs.push(`[${m.type()}] ${m.text()}`); });
page.on('pageerror', (e) => logs.push(`[pageerror] ${e.message}`));
await page.goto(`http://localhost:5173/?shot=table&event=${event}&speed=${speed}&pace=full`);
await page.waitForFunction(() => window.__ready === true, null, { timeout: 120000 });
const shots = [];
for (let i = 0; i < 3; i++) {
  const ok = await page.waitForFunction(() => !!window.__game.session.prompt, null, { timeout: 240000 }).then(() => true).catch(() => false);
  if (!ok) { logs.push('no prompt'); break; }
  await page.waitForTimeout(900);
  const file = `${prefix}-prompt${i}.png`;
  await page.screenshot({ path: file });
  shots.push(file);
  // Call or check.
  await page.evaluate(() => { const s = window.__game.session; const p = s.prompt; if (p) s.heroAct(p.canCheck ? { type: 'check' } : { type: 'call' }); });
  await page.waitForTimeout(1500);
  const f2 = `${prefix}-after${i}.png`;
  await page.screenshot({ path: f2 });
  shots.push(f2);
}
const state = await page.evaluate(() => { const s = window.__game.session; return { hands: s.handsPlayed, grades: s.grades.map((g) => g.grade + ':' + g.note), chat: s.chat.slice(-8).map((c) => (c.who ? c.who + ': ' : '') + c.text) }; });
console.log(JSON.stringify(state, null, 1));
console.log(shots.join('\n'));
console.log(logs.filter((l) => !l.includes('CERT') && !l.includes('404')).slice(0, 20).join('\n'));
await browser.close();
