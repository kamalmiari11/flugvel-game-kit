#pragma once
// GENERATED from sim/page.html by tools/embed_page.py - do not edit.
namespace sim {
static const char* kPageHtml = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Device simulator</title>
<style>
  :root { --bg:#14161a; --panel:#1d2026; --line:#2c313a; --ink:#e7eaf0; --dim:#98a1b0;
          --ok:#63c98a; --warn:#e2b23c; --bad:#e2574c; }
  * { box-sizing: border-box; }
  body { margin:0; background:var(--bg); color:var(--ink); font:13px/1.5 ui-monospace,SFMono-Regular,Menlo,monospace;
         display:flex; gap:24px; padding:24px; flex-wrap:wrap; align-items:flex-start; }
  #stage { background:#000; padding:12px; border-radius:14px; border:1px solid var(--line); }
  canvas { image-rendering: pixelated; display:block; }
  .panel { background:var(--panel); border:1px solid var(--line); border-radius:10px; padding:14px 16px; min-width:300px; }
  h2 { margin:0 0 10px; font-size:12px; letter-spacing:.14em; text-transform:uppercase; color:var(--dim); font-weight:600; }
  table { border-collapse:collapse; width:100%; }
  td { padding:2px 0; }
  td.k { color:var(--dim); }
  td.v { text-align:right; }
  kbd { background:#2a2f38; border:1px solid #3a414d; border-bottom-width:2px; border-radius:4px;
        padding:1px 6px; font:inherit; font-size:12px; }
  .row { display:flex; justify-content:space-between; gap:12px; padding:3px 0; }
  .ok{color:var(--ok)} .warn{color:var(--warn)} .bad{color:var(--bad)}
  #viol { margin-top:10px; max-height:180px; overflow:auto; font-size:12px; color:var(--bad); white-space:pre-wrap; }
  .hint { color:var(--dim); margin-top:10px; font-size:12px; }
</style>
</head>
<body>
  <div id="stage"><canvas id="screen" width="320" height="240"></canvas></div>

  <div class="panel">
    <h2>Controls</h2>
    <div class="row"><span>Knob clockwise</span><span><kbd>&uarr;</kbd> or <kbd>.</kbd></span></div>
    <div class="row"><span>Knob anticlockwise</span><span><kbd>&darr;</kbd> or <kbd>,</kbd></span></div>
    <div class="row"><span>Button</span><span><kbd>space</kbd></span></div>
    <div class="row"><span>Restart run</span><span><kbd>r</kbd></span></div>
    <div class="row"><span>Switch theme</span><span><kbd>t</kbd></span></div>
    <div class="row"><span>Sound on / off</span><span><kbd>m</kbd></span></div>
    <div class="row"><span>Zoom</span><span><kbd>-</kbd> <kbd>=</kbd></span></div>
    <div class="hint">One key press = one detent, exactly as the real encoder reports it. Hold a key and the repeat rate is your keyboard's, which is faster than anyone can turn a knob - do not tune difficulty against it.</div>
    <div class="hint" id="audiohint">Sound starts on your first key press - browsers will not make noise before one.</div>
  </div>

  <div class="panel">
    <h2>Frame budget</h2>
    <table>
      <tr><td class="k">pixels this frame</td><td class="v" id="px">-</td></tr>
      <tr><td class="k">bus time</td><td class="v" id="ms">-</td></tr>
      <tr><td class="k">worst frame so far</td><td class="v" id="peak">-</td></tr>
      <tr><td class="k">frames</td><td class="v" id="frames">-</td></tr>
      <tr><td class="k">device clock</td><td class="v" id="clock">-</td></tr>
      <tr><td class="k">best score</td><td class="v" id="best">-</td></tr>
      <tr><td class="k">theme</td><td class="v" id="theme">-</td></tr>
      <tr><td class="k">sound</td><td class="v" id="sound">-</td></tr>
      <tr><td class="k">notes played</td><td class="v" id="notes">-</td></tr>
      <tr><td class="k">notes dropped</td><td class="v" id="drops">-</td></tr>
    </table>
    <h2 style="margin-top:14px">Rule breaks</h2>
    <div id="viol">none</div>
  </div>

<script>
const cv = document.getElementById('screen');
const ctx = cv.getContext('2d');
const img = ctx.createImageData(320, 240);
let zoom = 2;
function applyZoom() { cv.style.width = (320 * zoom) + 'px'; cv.style.height = (240 * zoom) + 'px'; }
applyZoom();

async function pump() {
  try {
    const r = await fetch('/frame.raw', { cache: 'no-store' });
    const buf = new Uint8Array(await r.arrayBuffer());
    if (buf.length === 320 * 240 * 3) {
      const d = img.data;
      for (let i = 0, j = 0; i < buf.length; i += 3, j += 4) {
        d[j] = buf[i]; d[j+1] = buf[i+1]; d[j+2] = buf[i+2]; d[j+3] = 255;
      }
      ctx.putImageData(img, 0, 0);
    }
    const s = await (await fetch('/state', { cache: 'no-store' })).json();
    document.getElementById('px').textContent = s.framePixels.toLocaleString();
    const ms = (s.framePixels / 2500);
    const msEl = document.getElementById('ms');
    msEl.textContent = ms.toFixed(1) + ' ms';
    msEl.className = 'v ' + (s.framePixels > s.failAt ? 'bad' : s.framePixels > s.warnAt ? 'warn' : 'ok');
    const pk = document.getElementById('peak');
    pk.textContent = s.peakPixels.toLocaleString() + ' (' + (s.peakPixels / 2500).toFixed(1) + ' ms)';
    pk.className = 'v ' + (s.peakPixels > s.failAt ? 'bad' : s.peakPixels > s.warnAt ? 'warn' : 'ok');
    document.getElementById('frames').textContent = s.frames;
    document.getElementById('clock').textContent = (s.now / 1000).toFixed(1) + ' s';
    document.getElementById('best').textContent = s.best;
    document.getElementById('theme').textContent = s.theme;
    const snd = document.getElementById('sound');
    snd.textContent = s.soundOn ? 'on' : 'muted';
    snd.className = 'v ' + (s.soundOn ? '' : 'warn');
    document.getElementById('notes').textContent = s.notesPlayed;
    const dr = document.getElementById('drops');
    dr.textContent = s.notesDropped;
    dr.className = 'v ' + (s.notesDropped > 0 ? 'bad' : '');
    if (s.notes && s.notes.length) playNotes(s.notes);
    document.getElementById('viol').textContent = s.violations.length ? s.violations.join('\n') : 'none';
  } catch (e) { /* the simulator went away; keep trying */ }
  setTimeout(pump, 40);
}
pump();

// ---- the buzzer -------------------------------------------------------
// A passive buzzer is a square wave and nothing else, so that is exactly what
// gets synthesised here: no envelope, no filter, no reverb. It should sound
// slightly unpleasant - that is the point, it is what the device sounds like.
let audio = null, cursor = 0;
function startAudio() {
  if (audio) return;
  try {
    audio = new (window.AudioContext || window.webkitAudioContext)();
    cursor = audio.currentTime;
    document.getElementById('audiohint').style.display = 'none';
  } catch (e) { /* no audio available; everything else still works */ }
}
function playNotes(notes) {
  if (!audio || !notes.length) return;
  if (audio.state === 'suspended') audio.resume();
  // Keep the cursor ahead of the clock: notes queue back to back on the
  // device, so they do here too, rather than all landing at once.
  if (cursor < audio.currentTime) cursor = audio.currentTime + 0.01;
  for (const n of notes) {
    const dur = n.ms / 1000;
    if (n.f > 0) {
      const osc = audio.createOscillator();
      const gain = audio.createGain();
      osc.type = 'square';
      osc.frequency.value = n.f;
      gain.gain.value = 0.06;                       // a buzzer is loud; this is not
      osc.connect(gain).connect(audio.destination);
      osc.start(cursor);
      osc.stop(cursor + dur);
    }
    cursor += dur;                                   // f === 0 is a rest
  }
}

function send(k) { fetch('/input?k=' + k, { cache: 'no-store' }); }
addEventListener('keydown', e => {
  startAudio();                 // browsers need a gesture before any sound
  switch (e.key) {
    case 'ArrowUp': case '.': send('up'); break;
    case 'ArrowDown': case ',': send('down'); break;
    case ' ': case 'Enter': send('press'); break;
    case 'r': case 'R': send('restart'); break;
    case 't': case 'T': send('theme'); break;
    case 'm': case 'M': send('mute'); break;
    case '-': zoom = Math.max(1, zoom - 1); applyZoom(); return;
    case '=': case '+': zoom = Math.min(4, zoom + 1); applyZoom(); return;
    default: return;
  }
  e.preventDefault();
});
</script>
</body>
</html>
)HTML";
} // namespace sim
