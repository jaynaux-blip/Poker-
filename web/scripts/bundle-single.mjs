/**
 * After `vite build`, inline the JS and CSS into one self-contained HTML file.
 *   dist/short-stack.html   full standalone document (open locally or host anywhere)
 *   dist/artifact.html      same content without the document skeleton, for claude.ai artifacts
 */
import { readFileSync, writeFileSync, readdirSync } from 'node:fs';
import { join } from 'node:path';

const dist = new URL('../dist/', import.meta.url).pathname;
let html = readFileSync(join(dist, 'index.html'), 'utf8');
const assets = readdirSync(join(dist, 'assets'));
for (const f of assets) {
  const body = readFileSync(join(dist, 'assets', f), 'utf8');
  if (f.endsWith('.js')) {
    const re = new RegExp(`<script[^>]*src="\\./assets/${f.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}"[^>]*></script>`);
    const safe = body.replace(/<\/script/gi, '<\\/script');
    html = html.replace(re, () => `<script type="module">\n${safe}\n</script>`);
  } else if (f.endsWith('.css')) {
    const re = new RegExp(`<link[^>]*href="\\./assets/${f.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}"[^>]*>`);
    html = html.replace(re, () => `<style>\n${body}\n</style>`);
  }
}
if (/src="\.\/assets\//.test(html) || /href="\.\/assets\//.test(html)) throw new Error('Unresolved asset reference');
writeFileSync(join(dist, 'short-stack.html'), html);

// Artifact variant: title first, then head content (fonts, styles), then body content.
const title = html.match(/<title>[\s\S]*?<\/title>/)[0];
const head = html.match(/<head>([\s\S]*?)<\/head>/)[1]
  .replace(/<meta[^>]*>/g, '')
  .replace(/<title>[\s\S]*?<\/title>/, '');
const body = html.match(/<body>([\s\S]*?)<\/body>/)[1];
// Scripts in the head must run after the body markup exists.
const scripts = [...head.matchAll(/<script[\s\S]*?<\/script>/g)].map((m) => m[0]);
const headNoScripts = head.replace(/<script[\s\S]*?<\/script>/g, '');
const artifact = `${title}\n${headNoScripts.trim()}\n${body.trim()}\n${scripts.join('\n')}\n`;
writeFileSync(join(dist, 'artifact.html'), artifact);
console.log(`short-stack.html ${(html.length / 1024).toFixed(0)} KB, artifact.html ${(artifact.length / 1024).toFixed(0)} KB`);
