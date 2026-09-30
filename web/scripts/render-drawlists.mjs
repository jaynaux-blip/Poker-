// Replays draw lists written by the C++ UI test (ui_test <dir>) in Chromium and saves PNGs,
// so the Unreal client's screens can be checked without Unreal.
// Usage: node scripts/render-drawlists.mjs <dir> [scale]
// With BG=<image> in the environment, full-screen lists (the menus) are drawn over that image.
import { chromium } from 'playwright';
import { readdirSync, readFileSync } from 'node:fs';
const bg = process.env.BG ? 'data:image/png;base64,' + readFileSync(process.env.BG).toString('base64') : '';
import { join } from 'node:path';
const [dir, scaleArg = '1'] = process.argv.slice(2);
const scale = Number(scaleArg);
const browser = await chromium.launch({
  executablePath: '/opt/pw-browsers/chromium-1194/chrome-linux/chrome',
  args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'],
});
const page = await browser.newPage({ viewport: { width: 2600, height: 1200 } });
page.on('pageerror', (e) => console.log('pageerror', e.message));
await page.setContent('<html><body style="margin:0;background:#000"><canvas id="out"></canvas></body></html>');
for (const file of readdirSync(dir).filter((f) => f.endsWith('.json'))) {
  const json = readFileSync(join(dir, file), 'utf8');
  const size = await page.evaluate(async ({ json, scale, bg, isMenu }) => {
    const d = JSON.parse(json);
    const W = Math.round(d.w * scale);
    const H = Math.round(d.h * scale);
    const out = document.getElementById('out');
    out.width = W;
    out.height = H;
    const ctx = out.getContext('2d');
    ctx.clearRect(0, 0, W, H);
    if (bg && isMenu) {
      const img = new Image();
      img.src = bg;
      await img.decode();
      const k = Math.max(W / img.width, H / img.height);
      ctx.drawImage(img, (W - img.width * k) / 2, (H - img.height * k) / 2, img.width * k, img.height * k);
    }
    const glc = document.createElement('canvas');
    glc.width = W;
    glc.height = H;
    const gl = glc.getContext('webgl', { premultipliedAlpha: true, antialias: false, preserveDrawingBuffer: true });
    const vs = gl.createShader(gl.VERTEX_SHADER);
    gl.shaderSource(vs, 'attribute vec2 p; attribute vec4 c; uniform vec2 s; varying vec4 v; void main(){ v=c; gl_Position=vec4(p.x/s.x*2.0-1.0, 1.0-p.y/s.y*2.0, 0.0, 1.0); }');
    gl.compileShader(vs);
    const fs = gl.createShader(gl.FRAGMENT_SHADER);
    gl.shaderSource(fs, 'precision mediump float; varying vec4 v; void main(){ gl_FragColor=vec4(v.rgb*v.a, v.a); }');
    gl.compileShader(fs);
    const prog = gl.createProgram();
    gl.attachShader(prog, vs);
    gl.attachShader(prog, fs);
    gl.linkProgram(prog);
    gl.useProgram(prog);
    gl.uniform2f(gl.getUniformLocation(prog, 's'), d.w, d.h);
    const vb = gl.createBuffer();
    const ib = gl.createBuffer();
    const pl = gl.getAttribLocation(prog, 'p');
    const cl = gl.getAttribLocation(prog, 'c');
    gl.enable(gl.BLEND);
    gl.blendFunc(gl.ONE, gl.ONE_MINUS_SRC_ALPHA);
    const ext = gl.getExtension('OES_element_index_uint');
    // Same order and faces as ss::ui::Font and web/scripts/gen-test-font-metrics.mjs.
    const sans = 'Roboto, Arial, sans-serif';
    const fonts = [sans, sans, '"Droid Sans Mono", "DejaVu Sans Mono", monospace', sans, sans];
    const weights = [400, 700, 400, 900, 300];
    const clips = [];
    ctx.save();
    ctx.scale(scale, scale);
    for (const cmd of d.cmds) {
      if (cmd.t === 'tri') {
        const n = cmd.v.length / 6;
        const data = new Float32Array(n * 6);
        for (let i = 0; i < n; i++) {
          data[i * 6] = cmd.v[i * 6];
          data[i * 6 + 1] = cmd.v[i * 6 + 1];
          data[i * 6 + 2] = cmd.v[i * 6 + 2] / 255;
          data[i * 6 + 3] = cmd.v[i * 6 + 3] / 255;
          data[i * 6 + 4] = cmd.v[i * 6 + 4] / 255;
          data[i * 6 + 5] = cmd.v[i * 6 + 5];
        }
        gl.viewport(0, 0, W, H);
        gl.clearColor(0, 0, 0, 0);
        gl.disable(gl.SCISSOR_TEST);
        gl.clear(gl.COLOR_BUFFER_BIT);
        if (clips.length) {
          const r = clips[clips.length - 1];
          gl.enable(gl.SCISSOR_TEST);
          gl.scissor(Math.floor(r[0] * scale), Math.floor(H - (r[1] + r[3]) * scale), Math.ceil(r[2] * scale), Math.ceil(r[3] * scale));
        }
        gl.bindBuffer(gl.ARRAY_BUFFER, vb);
        gl.bufferData(gl.ARRAY_BUFFER, data, gl.STREAM_DRAW);
        gl.enableVertexAttribArray(pl);
        gl.vertexAttribPointer(pl, 2, gl.FLOAT, false, 24, 0);
        gl.enableVertexAttribArray(cl);
        gl.vertexAttribPointer(cl, 4, gl.FLOAT, false, 24, 8);
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, ib);
        gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint32Array(cmd.i), gl.STREAM_DRAW);
        gl.drawElements(gl.TRIANGLES, cmd.i.length, ext ? gl.UNSIGNED_INT : gl.UNSIGNED_SHORT, 0);
        ctx.save();
        ctx.setTransform(1, 0, 0, 1, 0, 0);
        ctx.drawImage(glc, 0, 0);
        ctx.restore();
      } else if (cmd.t === 'text') {
        ctx.font = `${weights[cmd.font]} ${cmd.size}px ${fonts[cmd.font]}`;
        ctx.fillStyle = `rgba(${cmd.c[0]},${cmd.c[1]},${cmd.c[2]},${cmd.c[3]})`;
        ctx.textBaseline = 'alphabetic';
        ctx.textAlign = 'left';
        ctx.fillText(cmd.s, cmd.x, cmd.y);
      } else if (cmd.t === 'clip') {
        clips.push(cmd.r);
        ctx.save();
        ctx.beginPath();
        ctx.rect(cmd.r[0], cmd.r[1], cmd.r[2], cmd.r[3]);
        ctx.clip();
      } else if (cmd.t === 'pop') {
        clips.pop();
        ctx.restore();
      }
    }
    ctx.restore();
    return [W, H];
  }, { json, scale, bg, isMenu: file.startsWith('menu_') });
  const png = join(dir, file.replace('.json', '.png'));
  await page.locator('#out').screenshot({ path: png });
  console.log('rendered', png, size.join('x'));
}
await browser.close();
