'use strict';
/* =====================================================================
   COSMIC SHOOTER — Ultimate Space Battle
   20 sectors · 4 bosses · levels end ONLY when every alien is destroyed
   ===================================================================== */
(() => {
const $ = id => document.getElementById(id);
const canvas = $('gameCanvas');
const ctx = canvas.getContext('2d');
const TAU = Math.PI * 2;
const rand = (a, b) => a + Math.random() * (b - a);
const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
const lerp = (a, b, t) => a + (b - a) * t;
function seeded(seed) { let s = seed >>> 0; return () => { s = (s + 0x6D2B79F5) >>> 0; let t = s; t = Math.imul(t ^ (t >>> 15), t | 1); t ^= t + Math.imul(t ^ (t >>> 7), t | 61); return ((t ^ (t >>> 14)) >>> 0) / 4294967296; }; }

/* ---------- storage (safe) ---------- */
const store = {
  get(k) { try { return localStorage.getItem(k); } catch (e) { return null; } },
  set(k, v) { try { localStorage.setItem(k, v); } catch (e) {} },
  del(k) { try { localStorage.removeItem(k); } catch (e) {} }
};

/* ---------- canvas ---------- */
let W = 0, H = 0, DPR = 1, bgCanvas = null;
function resize() {
  DPR = Math.min(window.devicePixelRatio || 1, 2);
  W = window.innerWidth; H = window.innerHeight;
  canvas.width = Math.floor(W * DPR); canvas.height = Math.floor(H * DPR);
  ctx.setTransform(DPR, 0, 0, DPR, 0, 0);
  buildStars(); buildBackground();
  if (player) { player.tx = clamp(player.tx, 24, W - 24); player.ty = clamp(player.ty, H * 0.35, H - 50); }
}

/* ---------- audio ---------- */
const Sound = {
  ac: null, muted: store.get('cosmicMuted') === '1',
  init() {
    if (!this.ac) { try { this.ac = new (window.AudioContext || window.webkitAudioContext)(); } catch (e) {} }
    if (this.ac && this.ac.state === 'suspended') this.ac.resume();
  },
  tone(f, d, type = 'sine', v = 0.04, slide = 0, delay = 0) {
    if (this.muted || !this.ac) return;
    const t = this.ac.currentTime + delay, o = this.ac.createOscillator(), g = this.ac.createGain();
    o.type = type; o.frequency.setValueAtTime(f, t);
    if (slide) o.frequency.exponentialRampToValueAtTime(Math.max(30, f + slide), t + d);
    g.gain.setValueAtTime(v, t); g.gain.exponentialRampToValueAtTime(0.0001, t + d);
    o.connect(g); g.connect(this.ac.destination); o.start(t); o.stop(t + d + 0.02);
  },
  noise(d = 0.25, v = 0.08, freq = 900) {
    if (this.muted || !this.ac) return;
    const n = Math.floor(this.ac.sampleRate * d), buf = this.ac.createBuffer(1, n, this.ac.sampleRate), data = buf.getChannelData(0);
    for (let i = 0; i < n; i++) data[i] = (Math.random() * 2 - 1) * (1 - i / n);
    const s = this.ac.createBufferSource(), g = this.ac.createGain(), f = this.ac.createBiquadFilter();
    f.type = 'lowpass'; f.frequency.value = freq; g.gain.value = v;
    s.buffer = buf; s.connect(f); f.connect(g); g.connect(this.ac.destination); s.start();
  },
  shoot() { this.tone(900, 0.07, 'square', 0.012, -600); },
  hit() { this.tone(260, 0.05, 'triangle', 0.025, -80); },
  boom(big) { this.noise(big ? 0.7 : 0.25, big ? 0.16 : 0.07, big ? 500 : 1100); this.tone(big ? 90 : 140, big ? 0.5 : 0.2, 'sawtooth', 0.05, -60); },
  hurt() { this.noise(0.3, 0.12, 700); this.tone(120, 0.3, 'square', 0.06, -70); },
  power() { [520, 780, 1040].forEach((f, i) => this.tone(f, 0.12, 'sine', 0.05, 0, i * 0.07)); },
  warn() { this.tone(300, 0.18, 'sawtooth', 0.05, 0, 0); this.tone(300, 0.18, 'sawtooth', 0.05, 0, 0.25); },
  clear() { [523, 659, 784, 1046].forEach((f, i) => this.tone(f, 0.2, 'triangle', 0.06, 0, i * 0.12)); },
  over() { [400, 320, 240, 160].forEach((f, i) => this.tone(f, 0.3, 'sawtooth', 0.06, -30, i * 0.18)); },
  shield() { this.tone(700, 0.12, 'sine', 0.04, 300); }
};

/* ---------- game data ---------- */
const MAX_LEVELS = 20;
const SECTORS = ['Earth Orbit', 'Lunar Shadow', 'Kepler Belt', 'Crimson Drift', 'Hive Gate',
  'Ion Storm', 'Nebula Reach', 'Titan Rift', 'Quasar Fields', 'The Void Maw',
  'Helix Expanse', 'Iron Graveyard', 'Nova Frontier', 'Rift Station', 'Omega Core',
  'Singularity Edge', 'Eclipse Gulf', 'Dread Corridor', 'Armada Wake', 'Overlord Throne'];
const SPECIES = ['Zorgian Scouts', 'Vulkar Raiders', 'Nebulon Drones', 'Krylix Swarm', 'The World Eater',
  'Xarn Hunters', 'Plasmid Wraiths', 'Orion Marauders', 'Quasar Reapers', 'Void Leviathan',
  'Helix Stalkers', 'Tyrant Guards', 'Nova Banshees', 'Rift Spawn', 'Omega Destroyer',
  'Singularity Knights', 'Eclipse Fangs', 'Dread Legion', 'Armada Remnants', 'Galactic Overlord'];
const BOSSES = [
  { name: 'THE WORLD EATER', hp: 150, hue: 340, size: 95, rate: 1.0 },
  { name: 'VOID LEVIATHAN', hp: 300, hue: 275, size: 108, rate: 0.9 },
  { name: 'OMEGA DESTROYER', hp: 500, hue: 190, size: 118, rate: 0.8 },
  { name: 'GALACTIC OVERLORD', hp: 800, hue: 48, size: 128, rate: 0.7 }
];
const TYPES = {
  drone:   { hp: 1, r: 20, speed: 70,  score: 10, shoot: 0 },
  fighter: { hp: 2, r: 22, speed: 85,  score: 20, shoot: 3.2 },
  gunner:  { hp: 3, r: 24, speed: 55,  score: 30, shoot: 2.4 },
  diver:   { hp: 2, r: 19, speed: 90,  score: 25, shoot: 0 },
  tank:    { hp: 6, r: 32, speed: 42,  score: 60, shoot: 2.8 }
};

function levelConfig(n) {
  const cfg = { n, sector: SECTORS[n - 1], species: SPECIES[n - 1], hue: (205 + n * 31) % 360, boss: null };
  if (n % 5 === 0) {
    cfg.boss = BOSSES[n / 5 - 1];
    cfg.total = 1;
    cfg.objective = 'Destroy ' + cfg.boss.name;
  } else {
    cfg.total = 10 + n * 2;
    cfg.speed = 1 + n * 0.035;
    cfg.cap = Math.min(5 + Math.floor(n / 2), 11);
    cfg.interval = Math.max(0.55, 1.5 - n * 0.05);
    cfg.mix = [['drone', Math.max(2, 6 - n * 0.2)]];
    if (n >= 2) cfg.mix.push(['fighter', Math.min(4, n)]);
    if (n >= 3) cfg.mix.push(['diver', 1.5 + n / 5]);
    if (n >= 4) cfg.mix.push(['gunner', 1 + n / 5]);
    if (n >= 6) cfg.mix.push(['tank', 0.8 + n / 8]);
    cfg.objective = `Destroy all ${cfg.total} ${cfg.species}`;
  }
  return cfg;
}

/* ---------- state ---------- */
let state = 'menu';            // menu | intro | play | paused | clear | over | win
let prevState = 'play';
let score = 0, levelNo = 1, level = null, levelStartScore = 0, levelStartHull = 100;
let best = parseInt(store.get('cosmicBest') || '0', 10) || 0;
let player = null, boss = null;
let bullets = [], ebullets = [], enemies = [], particles = [], rings = [], texts = [], powerups = [];
let kills = 0, spawned = 0, escapes = 0, spawnTimer = 0, introTimer = 0, endTimer = -1;
let shake = 0, combo = 0, comboTimer = 0, flashRed = 0;
let stars = [], shooting = [];
const keys = { left: false, right: false, up: false, down: false };
const mob = { left: false, right: false };

/* ---------- saved progress ---------- */
function getSave() { try { return JSON.parse(store.get('cosmicShooterSave2')); } catch (e) { return null; } }
function writeSave(lvl, sc) { if (lvl > 1) store.set('cosmicShooterSave2', JSON.stringify({ level: lvl, score: sc })); }
function clearSave() { store.del('cosmicShooterSave2'); }

/* =====================================================================
   BACKGROUND (procedural space scene, cached per level)
   ===================================================================== */
function buildStars() {
  stars = [];
  const layers = [{ n: 70, s: 14, r: 0.7, a: 0.45 }, { n: 45, s: 34, r: 1.1, a: 0.7 }, { n: 22, s: 70, r: 1.7, a: 1 }];
  layers.forEach(l => { for (let i = 0; i < l.n; i++) stars.push({ x: Math.random() * W, y: Math.random() * H, r: l.r * rand(0.7, 1.2), s: l.s * rand(0.8, 1.2), a: l.a, tw: Math.random() * TAU }); });
}

function buildBackground() {
  bgCanvas = document.createElement('canvas');
  bgCanvas.width = Math.max(1, W); bgCanvas.height = Math.max(1, H);
  const g = bgCanvas.getContext('2d');
  const n = level ? level.n : 1;
  const hue = level ? level.hue : 215;
  const R = seeded(n * 7919 + 13);

  const base = g.createLinearGradient(0, 0, 0, H);
  base.addColorStop(0, `hsl(${hue},55%,5%)`); base.addColorStop(1, `hsl(${(hue + 40) % 360},60%,9%)`);
  g.fillStyle = base; g.fillRect(0, 0, W, H);

  // nebula clouds
  for (let i = 0; i < 6; i++) {
    const x = R() * W, y = R() * H, r = (0.25 + R() * 0.45) * Math.max(W, H);
    const h = (hue + (R() - 0.5) * 90 + 360) % 360;
    const gr = g.createRadialGradient(x, y, 0, x, y, r);
    gr.addColorStop(0, `hsla(${h},80%,50%,${0.12 + R() * 0.1})`);
    gr.addColorStop(0.5, `hsla(${h},70%,35%,0.05)`);
    gr.addColorStop(1, 'transparent');
    g.fillStyle = gr; g.fillRect(0, 0, W, H);
  }

  // planet
  const pr = Math.min(W, H) * (0.2 + R() * 0.14);
  const px = W * (0.22 + R() * 0.56), py = H * (0.22 + R() * 0.3);
  const isEarth = n === 1;
  const ph = isEarth ? 210 : (hue + 120) % 360;
  // atmosphere glow
  const atm = g.createRadialGradient(px, py, pr * 0.9, px, py, pr * 1.35);
  atm.addColorStop(0, `hsla(${ph},90%,60%,0.35)`); atm.addColorStop(1, 'transparent');
  g.fillStyle = atm; g.beginPath(); g.arc(px, py, pr * 1.35, 0, TAU); g.fill();
  g.save(); g.beginPath(); g.arc(px, py, pr, 0, TAU); g.clip();
  const sph = g.createRadialGradient(px - pr * 0.4, py - pr * 0.4, pr * 0.1, px, py, pr);
  sph.addColorStop(0, `hsl(${ph},70%,${isEarth ? 48 : 42}%)`); sph.addColorStop(0.6, `hsl(${ph},65%,24%)`); sph.addColorStop(1, `hsl(${ph},60%,6%)`);
  g.fillStyle = sph; g.fillRect(px - pr, py - pr, pr * 2, pr * 2);
  if (isEarth) {
    g.fillStyle = 'hsla(125,45%,32%,0.85)';
    for (let i = 0; i < 7; i++) { g.beginPath(); g.ellipse(px + (R() - 0.5) * pr * 1.3, py + (R() - 0.5) * pr * 1.3, pr * (0.1 + R() * 0.22), pr * (0.07 + R() * 0.16), R() * 3, 0, TAU); g.fill(); }
    g.fillStyle = 'rgba(255,255,255,0.28)';
    for (let i = 0; i < 9; i++) { g.beginPath(); g.ellipse(px + (R() - 0.5) * pr * 1.5, py + (R() - 0.5) * pr * 1.5, pr * (0.14 + R() * 0.26), pr * 0.04, R() * 0.6, 0, TAU); g.fill(); }
  } else {
    for (let i = 0; i < 6; i++) { g.fillStyle = `hsla(${ph + (R() - 0.5) * 40},60%,${30 + R() * 25}%,0.22)`; g.fillRect(px - pr, py - pr + (i / 6) * pr * 2 + R() * 10, pr * 2, pr * 0.13 + R() * 14); }
  }
  // night shadow
  const sh = g.createRadialGradient(px - pr * 0.55, py - pr * 0.55, pr * 0.3, px + pr * 0.2, py + pr * 0.2, pr * 1.25);
  sh.addColorStop(0, 'transparent'); sh.addColorStop(1, 'rgba(0,0,8,0.85)');
  g.fillStyle = sh; g.fillRect(px - pr, py - pr, pr * 2, pr * 2);
  g.restore();
  // ring on some planets
  if (!isEarth && n % 3 === 0) {
    g.save(); g.translate(px, py); g.rotate(-0.35); g.strokeStyle = `hsla(${ph},60%,70%,0.4)`; g.lineWidth = pr * 0.09;
    g.beginPath(); g.ellipse(0, 0, pr * 1.6, pr * 0.35, 0, Math.PI, TAU); g.stroke(); g.restore();
  }
  // moon
  const mx = px + pr * 1.25 * (R() > 0.5 ? 1 : -1), my = py + pr * 0.9, mr = pr * 0.16;
  const mg = g.createRadialGradient(mx - mr * 0.3, my - mr * 0.3, 1, mx, my, mr);
  mg.addColorStop(0, '#cfd6e6'); mg.addColorStop(1, '#2a3040');
  g.fillStyle = mg; g.beginPath(); g.arc(mx, my, mr, 0, TAU); g.fill();

  // darken so the action stays readable
  g.fillStyle = 'rgba(0,4,14,0.28)'; g.fillRect(0, 0, W, H);
}

function updateStars(dt) {
  for (const s of stars) { s.y += s.s * dt; s.tw += dt * 2; if (s.y > H + 2) { s.y = -2; s.x = Math.random() * W; } }
  if (Math.random() < dt * 0.35) shooting.push({ x: rand(W * 0.2, W), y: rand(-20, H * 0.4), vx: -rand(500, 800), vy: rand(300, 500), life: 0.7 });
  for (let i = shooting.length - 1; i >= 0; i--) { const s = shooting[i]; s.x += s.vx * dt; s.y += s.vy * dt; s.life -= dt; if (s.life <= 0) shooting.splice(i, 1); }
}
function drawBackground() {
  if (bgCanvas) ctx.drawImage(bgCanvas, 0, 0, W, H); else { ctx.fillStyle = '#02040d'; ctx.fillRect(0, 0, W, H); }
  for (const s of stars) { ctx.globalAlpha = s.a * (0.65 + Math.sin(s.tw) * 0.35); ctx.fillStyle = '#fff'; ctx.beginPath(); ctx.arc(s.x, s.y, s.r, 0, TAU); ctx.fill(); }
  ctx.globalAlpha = 1;
  ctx.lineCap = 'round';
  for (const s of shooting) {
    const a = clamp(s.life / 0.7, 0, 1); const gr = ctx.createLinearGradient(s.x, s.y, s.x - s.vx * 0.09, s.y - s.vy * 0.09);
    gr.addColorStop(0, `rgba(255,255,255,${a})`); gr.addColorStop(1, 'transparent');
    ctx.strokeStyle = gr; ctx.lineWidth = 2; ctx.beginPath(); ctx.moveTo(s.x, s.y); ctx.lineTo(s.x - s.vx * 0.09, s.y - s.vy * 0.09); ctx.stroke();
  }
}

/* =====================================================================
   EFFECTS
   ===================================================================== */
function explode(x, y, hue, n = 16, speed = 1) {
  for (let i = 0; i < n && particles.length < 700; i++) {
    const a = Math.random() * TAU, sp = rand(40, 260) * speed;
    particles.push({ x, y, vx: Math.cos(a) * sp, vy: Math.sin(a) * sp, life: 1, decay: rand(1.3, 2.6), size: rand(1.5, 4.5), hue: hue + rand(-20, 20) });
  }
  rings.push({ x, y, r: 4, max: 24 + n * 1.8, life: 1, hue });
}
function floatText(x, y, txt, color = '#fff', size = 16) { texts.push({ x, y, txt, color, size, life: 1 }); }

/* =====================================================================
   PLAYER
   ===================================================================== */
function newPlayer(hull) {
  return { x: W / 2, y: H - 120, tx: W / 2, ty: H - 120, hull, r: 12, inv: 0, shield: 0, weapon: 1, weaponTimer: 0, fire: 0.2, bank: 0 };
}
const WEAPONS = {
  1: [[0, 0]],
  2: [[-9, 0], [9, 0]],
  3: [[0, 0], [-13, -110], [13, 110]],
  4: [[0, 0], [-14, -100], [14, 100], [-24, -230], [24, 230]]
};
function playerShoot() {
  const p = player;
  for (const [dx, vx] of WEAPONS[p.weapon]) bullets.push({ x: p.x + dx, y: p.y - 22, vx, vy: -820, r: 5 });
  Sound.shoot();
}
function damagePlayer(amount, cause) {
  const p = player;
  if (p.inv > 0 || state !== 'play') return;
  if (p.shield > 0) { rings.push({ x: p.x, y: p.y, r: 20, max: 52, life: 1, hue: 195 }); Sound.shield(); p.inv = 0.25; return; }
  p.hull = Math.max(0, p.hull - amount);
  p.inv = 1.0; shake = Math.max(shake, 10); flashRed = 0.35; combo = 0; comboTimer = 0;
  Sound.hurt();
  explode(p.x, p.y, 10, 12, 0.7);
  if (p.hull <= 0) gameOver();
}
function drawPlayer() {
  const p = player;
  ctx.save(); ctx.translate(p.x, p.y);
  if (p.inv > 0 && Math.floor(p.inv * 18) % 2 === 0) ctx.globalAlpha = 0.4;
  // engine flame
  const fl = 16 + Math.random() * 14;
  ctx.globalCompositeOperation = 'lighter';
  let g = ctx.createLinearGradient(0, 16, 0, 16 + fl);
  g.addColorStop(0, 'rgba(120,230,255,.95)'); g.addColorStop(0.5, 'rgba(60,120,255,.5)'); g.addColorStop(1, 'transparent');
  ctx.fillStyle = g; ctx.beginPath(); ctx.moveTo(-7, 16); ctx.lineTo(0, 16 + fl); ctx.lineTo(7, 16); ctx.fill();
  ctx.beginPath(); ctx.moveTo(-16, 20); ctx.lineTo(-14, 20 + fl * 0.5); ctx.lineTo(-12, 20); ctx.moveTo(16, 20); ctx.lineTo(14, 20 + fl * 0.5); ctx.lineTo(12, 20); ctx.fill();
  ctx.globalCompositeOperation = 'source-over';
  // wings
  const accent = p.weapon > 1 ? '#ffb020' : '#35e0ff';
  g = ctx.createLinearGradient(-26, 0, 26, 0);
  g.addColorStop(0, '#26304a'); g.addColorStop(0.5, '#8fa3c7'); g.addColorStop(1, '#26304a');
  ctx.fillStyle = g; ctx.strokeStyle = '#05070f'; ctx.lineWidth = 2.5; ctx.lineJoin = 'round';
  ctx.beginPath();
  ctx.moveTo(0, -28); ctx.lineTo(7, -8); ctx.lineTo(26, 16); ctx.lineTo(27, 22); ctx.lineTo(12, 16); ctx.lineTo(7, 22); ctx.lineTo(0, 17);
  ctx.lineTo(-7, 22); ctx.lineTo(-12, 16); ctx.lineTo(-27, 22); ctx.lineTo(-26, 16); ctx.lineTo(-7, -8); ctx.closePath();
  ctx.fill(); ctx.stroke();
  ctx.strokeStyle = accent; ctx.lineWidth = 1.2; ctx.globalAlpha *= 0.9; ctx.stroke(); ctx.globalAlpha = p.inv > 0 && Math.floor(p.inv * 18) % 2 === 0 ? 0.4 : 1;
  // wing stripes
  ctx.fillStyle = accent; ctx.fillRect(-22, 15, 7, 2); ctx.fillRect(15, 15, 7, 2);
  // cockpit
  g = ctx.createLinearGradient(0, -16, 0, 4);
  g.addColorStop(0, '#d8fbff'); g.addColorStop(1, accent);
  ctx.fillStyle = g; ctx.beginPath(); ctx.ellipse(0, -6, 4.5, 11, 0, 0, TAU); ctx.fill();
  ctx.strokeStyle = '#05070f'; ctx.lineWidth = 1.5; ctx.stroke();
  ctx.restore();
  // shield
  if (p.shield > 0) {
    const pulse = 0.6 + Math.sin(performance.now() / 120) * 0.2;
    ctx.save(); ctx.translate(p.x, p.y);
    const sg = ctx.createRadialGradient(0, 0, 18, 0, 0, 38);
    sg.addColorStop(0, 'rgba(80,190,255,0)'); sg.addColorStop(1, `rgba(80,190,255,${0.35 * pulse})`);
    ctx.fillStyle = sg; ctx.beginPath(); ctx.arc(0, 0, 38, 0, TAU); ctx.fill();
    ctx.strokeStyle = `rgba(150,225,255,${pulse})`; ctx.lineWidth = 2; ctx.beginPath(); ctx.arc(0, 0, 36, 0, TAU); ctx.stroke();
    ctx.restore();
  }
}

/* =====================================================================
   ENEMIES
   ===================================================================== */
function pickType() {
  const mix = level.mix; let total = 0; mix.forEach(m => total += m[1]);
  let r = Math.random() * total;
  for (const m of mix) { r -= m[1]; if (r <= 0) return m[0]; }
  return 'drone';
}
function spawnEnemy(type, minion = false, atX) {
  const T = TYPES[type], r = T.r;
  const e = {
    type, r, minion,
    x: atX !== undefined ? atX : rand(r + 12, W - r - 12), y: -r * 2,
    hp: T.hp + Math.floor(levelNo / 8), t: 0, phase: rand(0, TAU), amp: rand(30, 90),
    speed: T.speed * (level.speed || 1), fire: rand(0.8, 2.2), hue: (level.hue + 150 + rand(-25, 25) + 360) % 360,
    flash: 0, st: 0, hover: 0, hoverY: 0, vx: 0, vy: 0
  };
  e.maxHp = e.hp; e.baseX = e.x;
  if (type === 'gunner') e.hoverY = rand(H * 0.14, H * 0.28);
  if (type === 'diver') e.hoverY = rand(H * 0.16, H * 0.3);
  if (type === 'tank') e.hoverY = rand(H * 0.16, H * 0.24);
  enemies.push(e);
}
function ebullet(x, y, ang, speed, hue = 345, r = 5) {
  ebullets.push({ x, y, vx: Math.cos(ang) * speed, vy: Math.sin(ang) * speed, r, hue });
}
function enemyFire(e) {
  const toP = Math.atan2(player.y - e.y, player.x - e.x);
  const sp = 200 + levelNo * 4;
  if (e.type === 'fighter') ebullet(e.x, e.y + e.r, Math.PI / 2, sp + 20);
  else if (e.type === 'gunner') { ebullet(e.x - 12, e.y + e.r, toP, sp); ebullet(e.x + 12, e.y + e.r, toP, sp); }
  else if (e.type === 'tank') { for (const o of [-0.3, 0, 0.3]) ebullet(e.x, e.y + e.r, Math.PI / 2 + o, sp - 20, 20, 6); }
}
function enemyEscape(e, i) {
  if (e.minion) { enemies.splice(i, 1); return; }
  escapes++;
  e.y = -e.r * 2; e.x = rand(e.r + 12, W - e.r - 12); e.baseX = e.x; e.t = 0; e.st = 0; e.hover = 0;
  floatText(clamp(e.x, 60, W - 60), H - 40, 'ESCAPED!', '#ff3d71', 15);
  damagePlayer(4, 'escape');
}
function updateEnemy(e, i, dt) {
  e.t += dt; e.flash = Math.max(0, e.flash - dt);
  switch (e.type) {
    case 'drone': e.y += e.speed * dt; e.x = e.baseX + Math.sin(e.t * 1.6 + e.phase) * e.amp; break;
    case 'fighter': e.y += e.speed * 1.1 * dt; e.x = e.baseX + Math.sin(e.t * 2.6 + e.phase) * e.amp * 1.4; break;
    case 'gunner':
      if (e.y < e.hoverY) e.y += e.speed * 1.8 * dt;
      else { e.hover += dt; e.x += Math.sin(e.t * 1.3 + e.phase) * 70 * dt; if (e.hover > 7) e.y += e.speed * 2 * dt; }
      break;
    case 'tank':
      if (e.y < e.hoverY) e.y += e.speed * 1.5 * dt;
      else { e.hover += dt; e.x += Math.sin(e.t * 0.8 + e.phase) * 40 * dt; if (e.hover > 9) e.y += e.speed * 1.4 * dt; }
      break;
    case 'diver':
      if (e.st === 0) { e.y += e.speed * 1.6 * dt; if (e.y >= e.hoverY) { e.st = 1; e.hover = 0; } }
      else if (e.st === 1) { e.hover += dt; e.x += Math.sin(e.t * 40) * 40 * dt; if (e.hover > 0.9) { e.st = 2; const a = Math.atan2(player.y - e.y, player.x - e.x); e.vx = Math.cos(a) * 330; e.vy = Math.sin(a) * 330; } }
      else { e.x += e.vx * dt; e.y += e.vy * dt; if (e.x < -40 || e.x > W + 40) { e.y = H + 100; } }
      break;
  }
  if (e.st !== 2) e.x = clamp(e.x, e.r, W - e.r);
  const T = TYPES[e.type];
  if (T.shoot && e.y > 20 && e.y < H * 0.78) {
    e.fire -= dt;
    if (e.fire <= 0) { e.fire = T.shoot * rand(0.8, 1.3) / (1 + levelNo * 0.03); enemyFire(e); }
  }
  if (e.y - e.r > H) enemyEscape(e, i);
}

function drawEnemy(e) {
  const h = e.hue, r = e.r;
  const c = `hsl(${h},85%,58%)`, dk = `hsl(${h},70%,16%)`, lt = `hsl(${h},95%,78%)`;
  ctx.save(); ctx.translate(e.x, e.y);
  const flash = e.flash > 0;
  const bodyFill = (grad) => { ctx.fillStyle = flash ? '#fff' : grad; };
  ctx.lineJoin = 'round'; ctx.lineWidth = 2;
  const pulse = 0.6 + Math.sin(e.t * 6 + e.phase) * 0.4;

  if (e.type === 'drone') {
    // flying saucer
    let g = ctx.createRadialGradient(0, 0, 2, 0, 3, r * 1.1);
    g.addColorStop(0, c); g.addColorStop(1, dk);
    bodyFill(g); ctx.strokeStyle = lt;
    ctx.beginPath(); ctx.ellipse(0, 4, r * 1.15, r * 0.45, 0, 0, TAU); ctx.fill(); ctx.stroke();
    g = ctx.createRadialGradient(-4, -8, 1, 0, -4, r * 0.6);
    g.addColorStop(0, 'rgba(255,255,255,.9)'); g.addColorStop(0.4, `hsla(${h},90%,70%,.6)`); g.addColorStop(1, `hsla(${h},80%,30%,.8)`);
    bodyFill(g); ctx.beginPath(); ctx.arc(0, 0, r * 0.58, Math.PI, 0); ctx.closePath(); ctx.fill(); ctx.stroke();
    ctx.fillStyle = '#ff2a4d'; ctx.beginPath(); ctx.ellipse(0, -3, 3, 5, 0, 0, TAU); ctx.fill();
    for (let k = -1; k <= 1; k++) { ctx.fillStyle = `rgba(255,240,150,${0.4 + pulse * 0.6})`; ctx.beginPath(); ctx.arc(k * r * 0.6, 7, 2.2, 0, TAU); ctx.fill(); }
  } else if (e.type === 'fighter') {
    const g = ctx.createLinearGradient(-r, 0, r, 0); g.addColorStop(0, dk); g.addColorStop(0.5, c); g.addColorStop(1, dk);
    bodyFill(g); ctx.strokeStyle = lt;
    ctx.beginPath();
    ctx.moveTo(0, r * 1.15); ctx.lineTo(r * 0.45, r * 0.1); ctx.lineTo(r * 1.25, -r * 0.55); ctx.lineTo(r * 0.55, -r * 0.7);
    ctx.lineTo(0, -r * 0.3); ctx.lineTo(-r * 0.55, -r * 0.7); ctx.lineTo(-r * 1.25, -r * 0.55); ctx.lineTo(-r * 0.45, r * 0.1); ctx.closePath();
    ctx.fill(); ctx.stroke();
    ctx.fillStyle = '#ff2a4d'; ctx.beginPath(); ctx.moveTo(-6, 2); ctx.lineTo(6, 2); ctx.lineTo(0, 12); ctx.closePath(); ctx.fill();
    ctx.fillStyle = lt; ctx.fillRect(-r * 1.0, -r * 0.55, 5, 3); ctx.fillRect(r * 1.0 - 5, -r * 0.55, 5, 3);
  } else if (e.type === 'gunner') {
    const g = ctx.createRadialGradient(0, -2, 2, 0, 0, r * 1.1); g.addColorStop(0, c); g.addColorStop(1, dk);
    bodyFill(g); ctx.strokeStyle = lt;
    // cannons
    ctx.fillRect(-r * 1.05, -2, 8, r * 0.95); ctx.fillRect(r * 1.05 - 8, -2, 8, r * 0.95);
    ctx.strokeRect(-r * 1.05, -2, 8, r * 0.95); ctx.strokeRect(r * 1.05 - 8, -2, 8, r * 0.95);
    ctx.beginPath(); ctx.ellipse(0, 0, r * 0.95, r * 0.8, 0, 0, TAU); ctx.fill(); ctx.stroke();
    // mandibles
    ctx.strokeStyle = lt; ctx.lineWidth = 3; ctx.beginPath(); ctx.moveTo(-8, r * 0.6); ctx.lineTo(-12, r * 1.05); ctx.moveTo(8, r * 0.6); ctx.lineTo(12, r * 1.05); ctx.stroke();
    ctx.fillStyle = '#ff2a4d';
    ctx.beginPath(); ctx.ellipse(-7, -4, 4.5, 6.5, 0.4, 0, TAU); ctx.fill();
    ctx.beginPath(); ctx.ellipse(7, -4, 4.5, 6.5, -0.4, 0, TAU); ctx.fill();
  } else if (e.type === 'diver') {
    if (e.st === 1) { ctx.fillStyle = `rgba(255,40,70,${0.15 + pulse * 0.3})`; ctx.beginPath(); ctx.arc(0, 0, r * 1.9, 0, TAU); ctx.fill(); }
    if (e.st === 2) { const a = Math.atan2(e.vy, e.vx) - Math.PI / 2; ctx.rotate(a); }
    const g = ctx.createLinearGradient(-r, 0, r, 0); g.addColorStop(0, dk); g.addColorStop(0.5, c); g.addColorStop(1, dk);
    bodyFill(g); ctx.strokeStyle = lt;
    ctx.beginPath();
    ctx.moveTo(0, r * 1.35); ctx.lineTo(r * 0.55, -r * 0.2); ctx.lineTo(r * 0.85, -r * 0.95); ctx.lineTo(0, -r * 0.55); ctx.lineTo(-r * 0.85, -r * 0.95); ctx.lineTo(-r * 0.55, -r * 0.2); ctx.closePath();
    ctx.fill(); ctx.stroke();
    ctx.fillStyle = e.st ? '#fff' : '#ff2a4d'; ctx.beginPath(); ctx.arc(0, 2, 4.5, 0, TAU); ctx.fill();
    if (e.st === 2) { ctx.globalCompositeOperation = 'lighter'; const gg = ctx.createLinearGradient(0, -r, 0, -r * 3); gg.addColorStop(0, `hsla(${h},100%,70%,.8)`); gg.addColorStop(1, 'transparent'); ctx.fillStyle = gg; ctx.beginPath(); ctx.moveTo(-6, -r * 0.6); ctx.lineTo(0, -r * 3); ctx.lineTo(6, -r * 0.6); ctx.fill(); }
  } else if (e.type === 'tank') {
    const poly = (rad, n, rot = 0) => { ctx.beginPath(); for (let k = 0; k < n; k++) { const a = rot + k * TAU / n; ctx.lineTo(Math.cos(a) * rad, Math.sin(a) * rad); } ctx.closePath(); };
    let g = ctx.createRadialGradient(-6, -6, 3, 0, 0, r * 1.1); g.addColorStop(0, c); g.addColorStop(1, dk);
    bodyFill(g); ctx.strokeStyle = lt; ctx.lineWidth = 2.5;
    // cannon
    ctx.fillRect(-6, 0, 12, r * 1.25); ctx.strokeRect(-6, 0, 12, r * 1.25);
    poly(r, 6, Math.PI / 6); ctx.fill(); ctx.stroke();
    ctx.fillStyle = flash ? '#fff' : dk; poly(r * 0.62, 6, Math.PI / 6); ctx.fill(); ctx.stroke();
    ctx.fillStyle = `rgba(255,50,80,${0.55 + pulse * 0.45})`; ctx.beginPath(); ctx.arc(0, 0, r * 0.3, 0, TAU); ctx.fill();
    ctx.fillStyle = '#fff'; ctx.beginPath(); ctx.arc(-3, -3, 3, 0, TAU); ctx.fill();
  }
  ctx.restore();
  // hp pip bar for tough enemies
  if (e.maxHp > 1 && e.hp < e.maxHp) {
    const w = r * 1.6; ctx.fillStyle = 'rgba(0,0,0,.55)'; ctx.fillRect(e.x - w / 2, e.y - r - 12, w, 4);
    ctx.fillStyle = '#ff3d71'; ctx.fillRect(e.x - w / 2, e.y - r - 12, w * (e.hp / e.maxHp), 4);
  }
}

function killEnemy(e, i, byRam) {
  explode(e.x, e.y, e.hue, e.type === 'tank' ? 30 : 16, e.type === 'tank' ? 1.3 : 1);
  Sound.boom(e.type === 'tank');
  enemies.splice(i, 1);
  if (!e.minion) kills++;
  combo++; comboTimer = 2.6;
  const mult = Math.min(5, 1 + Math.floor(combo / 5));
  const pts = TYPES[e.type].score * mult;
  score += pts; floatText(e.x, e.y, '+' + pts, mult > 1 ? '#ffd24a' : '#ffffff', 15);
  if (combo > 0 && combo % 5 === 0 && mult <= 5) floatText(player.x, player.y - 50, 'COMBO x' + mult, '#ffd24a', 20);
  if (score > best) { best = score; store.set('cosmicBest', String(best)); }
  const chance = e.type === 'tank' ? 0.4 : 0.09;
  if (!byRam && powerups.length < 2 && Math.random() < chance) {
    const pr = Math.random(); powerups.push({ x: e.x, y: e.y, kind: pr < 0.45 ? 'W' : pr < 0.75 ? 'S' : 'R', t: 0 });
  }
}

/* =====================================================================
   BOSS
   ===================================================================== */
function spawnBoss() {
  const d = level.boss, r = Math.min(d.size, W * 0.2);
  boss = { name: d.name, hue: d.hue, r, maxHp: d.hp, hp: d.hp, rate: d.rate, x: W / 2, y: -r * 1.6, targetY: Math.max(r + 105, H * 0.22),
    t: 0, fire: 1.6, minionT: 4, flash: 0, dying: false, dieT: 0, ang: 0, entered: false, phase: 1 };
  $('bossName').textContent = d.name; $('bossFill').style.width = '100%'; $('bossBar').classList.remove('hidden');
}
function updateBoss(dt) {
  const b = boss; b.t += dt; b.ang += dt * 0.9; b.flash = Math.max(0, b.flash - dt);
  if (b.dying) {
    b.dieT += dt;
    if (Math.random() < dt * 16) explode(b.x + rand(-b.r, b.r) * 0.8, b.y + rand(-b.r, b.r) * 0.6, b.hue, 14, 1.1);
    if (Math.random() < dt * 6) Sound.boom(false);
    shake = Math.max(shake, 7);
    if (b.dieT > 1.9) {
      explode(b.x, b.y, b.hue, 70, 2); explode(b.x, b.y, (b.hue + 40) % 360, 50, 1.4); Sound.boom(true);
      score += 500 + levelNo * 50; floatText(b.x, b.y, '+' + (500 + levelNo * 50), '#ffd24a', 26);
      if (score > best) { best = score; store.set('cosmicBest', String(best)); }
      boss = null; $('bossBar').classList.add('hidden'); kills = 1; endTimer = 1.0; shake = 14;
    }
    return;
  }
  if (!b.entered) { b.y += 110 * dt; if (b.y >= b.targetY) { b.y = b.targetY; b.entered = true; } return; }
  const amp = Math.max(20, W / 2 - b.r - 12);
  b.x = W / 2 + Math.sin(b.t * (0.5 + levelNo * 0.012)) * amp; b.y = b.targetY + Math.sin(b.t * 1.1) * 14;
  const ratio = b.hp / b.maxHp, ph = ratio > 0.66 ? 1 : ratio > 0.33 ? 2 : 3;
  if (ph !== b.phase) { b.phase = ph; floatText(W / 2, H * 0.42, 'PHASE ' + ph, '#ff3d71', 28); Sound.warn(); shake = 12; }
  b.fire -= dt;
  if (b.fire <= 0) {
    const base = Math.atan2(player.y - b.y, player.x - b.x), by = b.y + b.r * 0.5;
    if (ph === 1) { for (let i = -1; i <= 1; i++) ebullet(b.x, by, base + i * 0.22, 250, b.hue); b.fire = 1.3 * b.rate; }
    else if (ph === 2) { for (let i = -3; i <= 3; i++) ebullet(b.x, by, Math.PI / 2 + i * 0.26 + Math.sin(b.t) * 0.2, 225, b.hue); b.fire = 1.1 * b.rate; }
    else { const n = 14; for (let i = 0; i < n; i++) ebullet(b.x, b.y, b.t + i * TAU / n, 195, b.hue, 5); ebullet(b.x, by, base, 300, b.hue, 6); b.fire = 1.05 * b.rate; }
  }
  if (ph >= 2) { b.minionT -= dt; if (b.minionT <= 0 && enemies.length < 5) { spawnEnemy('drone', true, clamp(b.x + rand(-60, 60), 30, W - 30)); b.minionT = ph === 2 ? 6 : 4.5; } }
}
function drawBoss() {
  const b = boss, r = b.r, h = b.hue;
  ctx.save(); ctx.translate(b.x, b.y);
  const pulse = 0.5 + Math.sin(b.t * 4) * 0.5;
  // aura
  let g = ctx.createRadialGradient(0, 0, r * 0.6, 0, 0, r * 2);
  g.addColorStop(0, `hsla(${h},90%,55%,${0.25 + pulse * 0.1})`); g.addColorStop(1, 'transparent');
  ctx.fillStyle = g; ctx.beginPath(); ctx.arc(0, 0, r * 2, 0, TAU); ctx.fill();
  // spike ring
  ctx.save(); ctx.rotate(b.ang);
  ctx.fillStyle = `hsl(${h},70%,22%)`; ctx.strokeStyle = `hsl(${h},90%,65%)`; ctx.lineWidth = 2; ctx.lineJoin = 'round';
  for (let i = 0; i < 12; i++) { ctx.save(); ctx.rotate(i * TAU / 12); ctx.beginPath(); ctx.moveTo(r * 0.92, -r * 0.1); ctx.lineTo(r * 1.45, 0); ctx.lineTo(r * 0.92, r * 0.1); ctx.closePath(); ctx.fill(); ctx.stroke(); ctx.restore(); }
  ctx.restore();
  // side cannons
  ctx.fillStyle = `hsl(${h},60%,20%)`; ctx.strokeStyle = `hsl(${h},90%,65%)`;
  for (const s of [-1, 1]) { ctx.beginPath(); ctx.rect(s * r * 0.78 - 9, -r * 0.1, 18, r * 0.75); ctx.fill(); ctx.stroke(); }
  // hull
  g = ctx.createRadialGradient(-r * 0.3, -r * 0.35, r * 0.1, 0, 0, r);
  g.addColorStop(0, `hsl(${h},80%,58%)`); g.addColorStop(0.55, `hsl(${h},65%,24%)`); g.addColorStop(1, '#05060c');
  ctx.fillStyle = g; ctx.beginPath(); ctx.arc(0, 0, r, 0, TAU); ctx.fill();
  ctx.strokeStyle = `hsl(${h},90%,70%)`; ctx.lineWidth = 3; ctx.stroke();
  // armor lines
  ctx.strokeStyle = `hsla(${h},80%,70%,.35)`; ctx.lineWidth = 2;
  for (let i = 1; i <= 3; i++) { ctx.beginPath(); ctx.arc(0, 0, r * i * 0.25, 0.2, Math.PI - 0.2); ctx.stroke(); }
  // eyes
  ctx.globalCompositeOperation = 'lighter';
  const eye = (x, y, er) => { const eg = ctx.createRadialGradient(x, y, 1, x, y, er * 2.2); eg.addColorStop(0, '#fff'); eg.addColorStop(0.3, '#ff2a4d'); eg.addColorStop(1, 'transparent'); ctx.fillStyle = eg; ctx.beginPath(); ctx.arc(x, y, er * 2.2, 0, TAU); ctx.fill(); };
  eye(-r * 0.32, -r * 0.18, r * 0.1); eye(r * 0.32, -r * 0.18, r * 0.1); eye(0, -r * 0.42, r * 0.14 * (0.85 + pulse * 0.3));
  ctx.globalCompositeOperation = 'source-over';
  // mouth
  ctx.strokeStyle = '#ff2a4d'; ctx.lineWidth = 3; ctx.beginPath(); ctx.moveTo(-r * 0.4, r * 0.3); for (let i = 0; i <= 8; i++) ctx.lineTo(-r * 0.4 + i * r * 0.1, r * 0.3 + (i % 2 ? 9 : -2)); ctx.stroke();
  if (b.flash > 0) { ctx.fillStyle = `rgba(255,255,255,${b.flash * 5})`; ctx.beginPath(); ctx.arc(0, 0, r, 0, TAU); ctx.fill(); }
  ctx.restore();
}

/* =====================================================================
   POWER-UPS
   ===================================================================== */
const PU = { W: { c: '#ffb020', n: 'WEAPON UP' }, S: { c: '#5fd4ff', n: 'SHIELD' }, R: { c: '#3dffa6', n: 'REPAIR +25' } };
function collectPowerup(pu) {
  const p = player; Sound.power();
  if (pu.kind === 'W') { p.weapon = Math.min(4, p.weapon + 1); p.weaponTimer = 16; }
  else if (pu.kind === 'S') p.shield = 8;
  else p.hull = Math.min(100, p.hull + 25);
  floatText(p.x, p.y - 40, PU[pu.kind].n, PU[pu.kind].c, 17);
  rings.push({ x: p.x, y: p.y, r: 10, max: 60, life: 1, hue: pu.kind === 'W' ? 40 : pu.kind === 'S' ? 195 : 150 });
}
function drawPowerup(pu) {
  const d = PU[pu.kind]; ctx.save(); ctx.translate(pu.x, pu.y);
  const g = ctx.createRadialGradient(0, 0, 2, 0, 0, 26); g.addColorStop(0, d.c + 'aa'); g.addColorStop(1, 'transparent');
  ctx.fillStyle = g; ctx.beginPath(); ctx.arc(0, 0, 26, 0, TAU); ctx.fill();
  ctx.rotate(pu.t * 1.5); ctx.strokeStyle = d.c; ctx.lineWidth = 2.5; ctx.fillStyle = 'rgba(5,8,20,.85)';
  ctx.beginPath(); for (let i = 0; i < 6; i++) { const a = i * TAU / 6; ctx.lineTo(Math.cos(a) * 15, Math.sin(a) * 15); } ctx.closePath(); ctx.fill(); ctx.stroke();
  ctx.rotate(-pu.t * 1.5); ctx.fillStyle = d.c; ctx.font = '800 14px Orbitron, sans-serif'; ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
  ctx.fillText(pu.kind === 'R' ? '+' : pu.kind, 0, 1);
  ctx.restore();
}

/* =====================================================================
   LEVEL FLOW
   ===================================================================== */
const screens = ['startScreen', 'pauseScreen', 'clearScreen', 'overScreen', 'winScreen'];
function showScreen(id) { screens.forEach(s => $(s).classList.add('hidden')); if (id) $(id).classList.remove('hidden'); }
function setHud(on) { $('hud').classList.toggle('hidden', !on); $('hudBtns').classList.toggle('hidden', !on); }

function startLevel(n) {
  levelNo = n; level = levelConfig(n);
  levelStartScore = score; levelStartHull = player.hull;
  bullets = []; ebullets = []; enemies = []; particles = []; rings = []; texts = []; powerups = []; boss = null;
  kills = 0; spawned = 0; escapes = 0; spawnTimer = 0.6; endTimer = -1; combo = 0; comboTimer = 0; shake = 0;
  Object.assign(player, { x: W / 2, y: H - 120, tx: W / 2, ty: H - 120, inv: 1.2, shield: 0, weapon: 1, weaponTimer: 0, fire: 0.3 });
  buildBackground();
  writeSave(n, score);
  $('bossBar').classList.add('hidden');
  $('progWrap').classList.toggle('hidden', !!level.boss);
  $('killChip').classList.toggle('hidden', !!level.boss);
  $('levelText').textContent = n; $('sectorText').textContent = level.sector.toUpperCase();
  const isBoss = !!level.boss;
  $('levelAlert').classList.toggle('boss', isBoss);
  $('alertTag').textContent = isBoss ? '⚠ BOSS APPROACHING ⚠' : 'INCOMING WAVE';
  $('alertTitle').textContent = 'LEVEL ' + n;
  $('alertSector').textContent = 'Sector · ' + level.sector;
  $('alertEnemy').textContent = (isBoss ? 'BOSS: ' : 'Enemy: ') + level.species;
  $('alertObj').textContent = level.objective;
  $('levelAlert').classList.remove('hidden');
  showScreen(null); setHud(true);
  state = 'intro'; introTimer = 2.8;
  if (isBoss) Sound.warn();
}

function beginPlay() {
  $('levelAlert').classList.add('hidden');
  state = 'play';
  if (level.boss) spawnBoss();
}

function checkLevelDone() {
  if (endTimer >= 0) return;
  if (level.boss) return;                       // boss flow handles itself
  // Level ends ONLY when every alien is destroyed.
  if (spawned >= level.total && enemies.length === 0 && kills >= level.total) endTimer = 0.9;
}

function completeLevel() {
  state = 'clear';
  Sound.clear();
  const p = player;
  const stars = p.hull >= 70 && escapes === 0 ? 3 : p.hull >= 40 && escapes <= 3 ? 2 : 1;
  const bonus = stars * 100 + Math.round(p.hull) * 2 + (escapes === 0 ? 150 : 0);
  score += bonus;
  if (score > best) { best = score; store.set('cosmicBest', String(best)); }
  p.hull = Math.min(100, p.hull + 25);
  $('clearSector').textContent = `Level ${levelNo} · ${level.sector}`;
  $('clearStars').innerHTML = [1, 2, 3].map(i => `<span class="${i <= stars ? 'on' : ''}">★</span>`).join('');
  $('clKills').textContent = level.boss ? 'BOSS' : level.total;
  $('clEsc').textContent = level.boss ? '—' : escapes;
  $('clHull').textContent = Math.round(p.hull) + '%';
  $('clBonus').textContent = '+' + bonus;
  $('clScore').textContent = score.toLocaleString();
  if (levelNo >= MAX_LEVELS) { $('nextInfo').textContent = 'Final sector liberated. One last step...'; document.querySelector('[data-act="next"]').textContent = 'CLAIM VICTORY'; }
  else {
    const nx = levelConfig(levelNo + 1);
    $('nextInfo').textContent = `Next: Level ${nx.n} · ${nx.sector} — ${nx.boss ? 'BOSS: ' : 'Enemy: '}${nx.species}`;
    document.querySelector('[data-act="next"]').textContent = 'NEXT LEVEL ' + nx.n;
    writeSave(levelNo + 1, score);
  }
  setHud(false); showScreen('clearScreen');
}

function gameOver() {
  if (state === 'over') return;
  state = 'over'; Sound.over();
  explode(player.x, player.y, 195, 50, 1.6); explode(player.x, player.y, 20, 30, 1.2);
  if (score > best) { best = score; store.set('cosmicBest', String(best)); }
  $('overLevel').textContent = levelNo; $('retryLevel').textContent = levelNo;
  $('overScore').textContent = score.toLocaleString(); $('overBest').textContent = best.toLocaleString();
  setTimeout(() => { if (state === 'over') { setHud(false); $('bossBar').classList.add('hidden'); showScreen('overScreen'); } }, 1100);
}

function victory() {
  state = 'win'; clearSave(); level = null;
  if (score > best) { best = score; store.set('cosmicBest', String(best)); }
  $('winScore').textContent = score.toLocaleString(); $('winBest').textContent = best.toLocaleString();
  showScreen('winScreen');
}

function showMenu() {
  state = 'menu'; setHud(false); $('levelAlert').classList.add('hidden'); $('bossBar').classList.add('hidden');
  const sv = getSave();
  if (sv && sv.level > 1 && sv.level <= MAX_LEVELS) { $('resumeLevel').textContent = sv.level; $('resumeBtn').classList.remove('hidden'); }
  else $('resumeBtn').classList.add('hidden');
  $('startBest').textContent = best.toLocaleString();
  showScreen('startScreen');
}

function newGame() { clearSave(); score = 0; player = newPlayer(100); startLevel(1); }

/* ---------- actions ---------- */
const actions = {
  start: newGame,
  resume() { const sv = getSave(); if (!sv) return newGame(); score = sv.score || 0; player = newPlayer(100); startLevel(clamp(sv.level, 1, MAX_LEVELS)); },
  next() { if (levelNo >= MAX_LEVELS) return victory(); startLevel(levelNo + 1); },
  retry() { score = levelStartScore; player = newPlayer(100); startLevel(levelNo); },
  menu() { showMenu(); },
  pause: pauseGame,
  unpause: unpauseGame,
  mute() { Sound.muted = !Sound.muted; store.set('cosmicMuted', Sound.muted ? '1' : '0'); $('muteBtn').textContent = Sound.muted ? '✕' : '♪'; }
};
function pauseGame() { if (state !== 'play' && state !== 'intro') return; prevState = state; state = 'paused'; showScreen('pauseScreen'); }
function unpauseGame() { if (state !== 'paused') return; state = prevState; showScreen(null); }

document.addEventListener('click', e => {
  const b = e.target.closest('[data-act]'); if (!b) return;
  Sound.init(); const fn = actions[b.dataset.act]; if (fn) fn();
});

/* ---------- input ---------- */
function pointerMove(e) {
  if (!player || state !== 'play') return;
  player.tx = e.clientX;
  player.ty = e.clientY - (e.pointerType === 'touch' ? 70 : 0);
}
canvas.addEventListener('pointermove', pointerMove);
canvas.addEventListener('pointerdown', e => { Sound.init(); pointerMove(e); });
canvas.addEventListener('touchmove', e => e.preventDefault(), { passive: false });
const keyMap = { ArrowLeft: 'left', KeyA: 'left', ArrowRight: 'right', KeyD: 'right', ArrowUp: 'up', KeyW: 'up', ArrowDown: 'down', KeyS: 'down' };
window.addEventListener('keydown', e => {
  if (keyMap[e.code]) { keys[keyMap[e.code]] = true; if (e.code.startsWith('Arrow')) e.preventDefault(); Sound.init(); }
  if (e.code === 'KeyP' || e.code === 'Escape') { state === 'paused' ? unpauseGame() : pauseGame(); }
  if (e.code === 'KeyM') actions.mute();
});
window.addEventListener('keyup', e => { if (keyMap[e.code]) keys[keyMap[e.code]] = false; });
const bindPad = (id, k) => {
  const el = $(id); const on = e => { e.preventDefault(); Sound.init(); mob[k] = true; }, off = e => { e.preventDefault(); mob[k] = false; };
  el.addEventListener('pointerdown', on); el.addEventListener('pointerup', off); el.addEventListener('pointerleave', off); el.addEventListener('pointercancel', off);
};
bindPad('btnLeft', 'left'); bindPad('btnRight', 'right');
document.addEventListener('visibilitychange', () => { if (document.hidden) pauseGame(); });
window.addEventListener('blur', pauseGame);
window.addEventListener('resize', resize);

/* =====================================================================
   UPDATE
   ===================================================================== */
function updatePlay(dt) {
  const p = player;
  const sp = 560 * dt;
  if (keys.left || mob.left) p.tx -= sp; if (keys.right || mob.right) p.tx += sp;
  if (keys.up) p.ty -= sp; if (keys.down) p.ty += sp;
  p.tx = clamp(p.tx, 24, W - 24); p.ty = clamp(p.ty, H * 0.35, H - 50);
  const k = Math.min(1, dt * 14);
  p.x += (p.tx - p.x) * k; p.y += (p.ty - p.y) * k;
  p.inv = Math.max(0, p.inv - dt);
  if (p.shield > 0) p.shield = Math.max(0, p.shield - dt);
  if (p.weaponTimer > 0) { p.weaponTimer -= dt; if (p.weaponTimer <= 0) p.weapon = 1; }
  if (comboTimer > 0) { comboTimer -= dt; if (comboTimer <= 0) combo = 0; }

  // auto fire
  if (endTimer < 0 && !(boss && boss.dying)) { p.fire -= dt; if (p.fire <= 0) { playerShoot(); p.fire = p.weapon >= 3 ? 0.17 : 0.2; } }

  // spawn
  if (!level.boss && spawned < level.total && enemies.length < level.cap) {
    spawnTimer -= dt;
    if (spawnTimer <= 0) { spawnEnemy(pickType()); spawned++; spawnTimer = level.interval * rand(0.7, 1.2); }
  }

  // player bullets
  for (let i = bullets.length - 1; i >= 0; i--) {
    const b = bullets[i]; b.x += b.vx * dt; b.y += b.vy * dt;
    if (b.y < -20 || b.x < -20 || b.x > W + 20) bullets.splice(i, 1);
  }
  // enemy bullets
  for (let i = ebullets.length - 1; i >= 0; i--) {
    const b = ebullets[i]; b.x += b.vx * dt; b.y += b.vy * dt;
    if (b.y > H + 20 || b.y < -40 || b.x < -30 || b.x > W + 30) { ebullets.splice(i, 1); continue; }
    if (!(boss && boss.dying) && Math.hypot(b.x - p.x, b.y - p.y) < p.r + b.r) { ebullets.splice(i, 1); damagePlayer(10); if (state !== 'play') return; }
  }
  // enemies
  for (let i = enemies.length - 1; i >= 0; i--) {
    const e = enemies[i]; updateEnemy(e, i, dt);
    if (state !== 'play') return;
    if (enemies[i] !== e) continue;
    if (Math.hypot(e.x - p.x, e.y - p.y) < e.r + p.r - 2) {
      if (p.inv <= 0) { damagePlayer(25); killEnemy(e, i, true); if (state !== 'play') return; }
      continue;
    }
    for (let j = bullets.length - 1; j >= 0; j--) {
      const b = bullets[j];
      if (Math.hypot(e.x - b.x, e.y - b.y) < e.r + b.r) {
        bullets.splice(j, 1); e.hp--; e.flash = 0.07; Sound.hit();
        particles.push({ x: b.x, y: b.y, vx: rand(-80, 80), vy: rand(-80, 40), life: 1, decay: 4, size: 2, hue: 50 });
        if (e.hp <= 0) { killEnemy(e, i, false); break; }
      }
    }
  }
  // boss
  if (boss) {
    updateBoss(dt);
    if (boss && !boss.dying) {
      for (let j = bullets.length - 1; j >= 0; j--) {
        const b = bullets[j];
        if (Math.hypot(boss.x - b.x, boss.y - b.y) < boss.r + b.r) {
          bullets.splice(j, 1); boss.hp--; boss.flash = 0.06; Sound.hit();
          particles.push({ x: b.x, y: b.y, vx: rand(-90, 90), vy: rand(-40, 90), life: 1, decay: 4, size: 2.5, hue: boss.hue });
          if (boss.hp <= 0) { boss.hp = 0; boss.dying = true; ebullets.length = 0; enemies.forEach(en => explode(en.x, en.y, en.hue, 10)); enemies.length = 0; Sound.boom(true); }
        }
      }
      $('bossFill').style.width = (boss.hp / boss.maxHp * 100) + '%';
      if (Math.hypot(boss.x - p.x, boss.y - p.y) < boss.r + p.r) damagePlayer(30);
    }
  }
  // powerups
  for (let i = powerups.length - 1; i >= 0; i--) {
    const pu = powerups[i]; pu.y += 90 * dt; pu.t += dt;
    if (Math.hypot(pu.x - p.x, pu.y - p.y) < 30) { collectPowerup(pu); powerups.splice(i, 1); }
    else if (pu.y > H + 30) powerups.splice(i, 1);
  }
  if (state === 'play') checkLevelDone();
  if (endTimer >= 0) { endTimer -= dt; if (endTimer < 0) { endTimer = -2; completeLevel(); } }
}

function updateFx(dt) {
  for (let i = particles.length - 1; i >= 0; i--) { const q = particles[i]; q.x += q.vx * dt; q.y += q.vy * dt; q.vx *= 0.985; q.vy *= 0.985; q.life -= q.decay * dt; if (q.life <= 0) particles.splice(i, 1); }
  for (let i = rings.length - 1; i >= 0; i--) { const r = rings[i]; r.r += (r.max - r.r) * Math.min(1, dt * 7); r.life -= dt * 2.2; if (r.life <= 0) rings.splice(i, 1); }
  for (let i = texts.length - 1; i >= 0; i--) { const t = texts[i]; t.y -= 38 * dt; t.life -= dt * 0.9; if (t.life <= 0) texts.splice(i, 1); }
  shake = Math.max(0, shake - dt * 30); flashRed = Math.max(0, flashRed - dt);
}

/* =====================================================================
   DRAW
   ===================================================================== */
function draw() {
  ctx.setTransform(DPR, 0, 0, DPR, 0, 0);
  ctx.save();
  if (shake > 0) ctx.translate(rand(-shake, shake) * 0.6, rand(-shake, shake) * 0.6);
  drawBackground();
  if (player && state !== 'menu') {
    powerups.forEach(drawPowerup);
    enemies.forEach(drawEnemy);
    if (boss) drawBoss();
    // bullets (glow)
    ctx.globalCompositeOperation = 'lighter';
    for (const b of bullets) {
      const g = ctx.createRadialGradient(b.x, b.y, 0, b.x, b.y, 14); g.addColorStop(0, 'rgba(255,255,255,.95)'); g.addColorStop(0.35, 'rgba(60,220,255,.7)'); g.addColorStop(1, 'transparent');
      ctx.fillStyle = g; ctx.beginPath(); ctx.arc(b.x, b.y, 14, 0, TAU); ctx.fill();
      ctx.fillStyle = 'rgba(90,220,255,.5)'; ctx.fillRect(b.x - 1.5, b.y, 3, 16);
    }
    for (const b of ebullets) {
      const g = ctx.createRadialGradient(b.x, b.y, 0, b.x, b.y, b.r * 3); g.addColorStop(0, '#fff'); g.addColorStop(0.3, `hsl(${b.hue},100%,60%)`); g.addColorStop(1, 'transparent');
      ctx.fillStyle = g; ctx.beginPath(); ctx.arc(b.x, b.y, b.r * 3, 0, TAU); ctx.fill();
    }
    ctx.globalCompositeOperation = 'source-over';
    if (state !== 'over') drawPlayer();
    // particles
    ctx.globalCompositeOperation = 'lighter';
    for (const q of particles) { ctx.globalAlpha = clamp(q.life, 0, 1); ctx.fillStyle = `hsl(${q.hue},100%,60%)`; ctx.beginPath(); ctx.arc(q.x, q.y, q.size, 0, TAU); ctx.fill(); }
    ctx.globalAlpha = 1;
    for (const r of rings) { ctx.strokeStyle = `hsla(${r.hue},100%,70%,${clamp(r.life, 0, 1) * 0.8})`; ctx.lineWidth = 3 * r.life; ctx.beginPath(); ctx.arc(r.x, r.y, r.r, 0, TAU); ctx.stroke(); }
    ctx.globalCompositeOperation = 'source-over';
    ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
    for (const t of texts) { ctx.globalAlpha = clamp(t.life * 1.6, 0, 1); ctx.font = `800 ${t.size}px Orbitron, sans-serif`; ctx.fillStyle = t.color; ctx.strokeStyle = 'rgba(0,0,0,.6)'; ctx.lineWidth = 3; ctx.strokeText(t.txt, t.x, t.y); ctx.fillText(t.txt, t.x, t.y); }
    ctx.globalAlpha = 1;
  }
  ctx.restore();
  if (flashRed > 0) { const g = ctx.createRadialGradient(W / 2, H / 2, Math.min(W, H) * 0.3, W / 2, H / 2, Math.max(W, H) * 0.75); g.addColorStop(0, 'transparent'); g.addColorStop(1, `rgba(255,30,70,${flashRed * 1.2})`); ctx.fillStyle = g; ctx.fillRect(0, 0, W, H); }
}

/* ---------- HUD ---------- */
const hudCache = {};
function setTxt(id, v) { if (hudCache[id] !== v) { hudCache[id] = v; $(id).textContent = v; } }
function updateHud() {
  if (!player || !level) return;
  setTxt('scoreText', score.toLocaleString()); setTxt('bestText', Math.max(best, score).toLocaleString());
  const hp = Math.round(player.hull);
  setTxt('hullText', hp + '%');
  const f = $('hullFill'); f.style.width = hp + '%';
  f.style.background = hp > 60 ? 'linear-gradient(90deg,#1ad1ff,#3dffa6)' : hp > 30 ? 'linear-gradient(90deg,#ffb020,#ffd24a)' : 'linear-gradient(90deg,#ff3d71,#ff7a3d)';
  $('hullText').style.color = hp > 60 ? '#3dffa6' : hp > 30 ? '#ffd24a' : '#ff3d71';
  if (!level.boss) { setTxt('killText', kills + '/' + level.total); $('progFill').style.width = (kills / level.total * 100) + '%'; }
  setTxt('weaponText', 'MK-' + player.weapon);
  const mult = Math.min(5, 1 + Math.floor(combo / 5));
  $('comboChip').classList.toggle('hidden', combo < 3); if (combo >= 3) setTxt('comboText', combo + ' · x' + mult);
  $('shieldChip').classList.toggle('hidden', player.shield <= 0); if (player.shield > 0) setTxt('shieldText', Math.ceil(player.shield) + 's');
}

/* =====================================================================
   MAIN LOOP
   ===================================================================== */
let last = performance.now();
function frame(now) {
  requestAnimationFrame(frame);
  const dt = Math.min(0.05, (now - last) / 1000 || 0); last = now;
  if (state !== 'paused') {
    updateStars(dt);
    if (state === 'intro') { introTimer -= dt; player.inv = Math.max(player.inv, 0.1); const k = Math.min(1, dt * 14); player.x += (player.tx - player.x) * k; player.y += (player.ty - player.y) * k; if (introTimer <= 0) beginPlay(); }
    else if (state === 'play') updatePlay(dt);
    else if ((state === 'over' || state === 'clear') && endTimer !== -2) { /* idle */ }
    updateFx(dt);
    if (state === 'over' || state === 'clear') { for (let i = enemies.length - 1; i >= 0; i--) { enemies[i].t += dt; enemies[i].y += 30 * dt; } }
  }
  draw();
  if (state === 'play' || state === 'intro' || state === 'paused') updateHud();
}

/* ---------- boot ---------- */
$('muteBtn').textContent = Sound.muted ? '✕' : '♪';
resize();
showMenu();
requestAnimationFrame(frame);
})();