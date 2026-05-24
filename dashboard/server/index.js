import express from 'express';
import fs from 'fs';
import path from 'path';
import url from 'url';
import cors from 'cors';
import { WebSocketServer } from 'ws';
import http from 'http';

const __dirname = path.dirname(url.fileURLToPath(import.meta.url));
const ROOT = path.resolve(__dirname, '../..');
const DASH = path.resolve(__dirname, '..');
const DATA_DIR = path.join(ROOT, 'data');
const SNAP_DIR = path.join(ROOT, 'snapshots');

const app = express();
const PORT = process.env.SEFERIM_DASH_PORT || 9306;

app.use(cors());
app.use(express.json());

// Family metadata
const FAMILY_META = [
  { id: 0, name: 'Athena', domain: 'Strategy', color: '#6366f1', icon: '🦉' },
  { id: 1, name: 'Iris', domain: 'Vision', color: '#ec4899', icon: '🌈' },
  { id: 2, name: 'Hermes', domain: 'Communication', color: '#14b8a6', icon: '📨' },
  { id: 3, name: 'Prometheus', domain: 'Creation', color: '#f97316', icon: '🔥' },
  { id: 4, name: 'Apollo', domain: 'Harmony', color: '#eab308', icon: '☀️' },
  { id: 5, name: 'Hephaestus', domain: 'Craft', color: '#78716c', icon: '🔨' },
  { id: 6, name: 'Demeter', domain: 'Growth', color: '#22c55e', icon: '🌱' },
  { id: 7, name: 'Poseidon', domain: 'Flow', color: '#3b82f6', icon: '🌊' },
  { id: 8, name: 'Hera', domain: 'Governance', color: '#a855f7', icon: '👑' },
  { id: 9, name: 'Ares', domain: 'Competition', color: '#ef4444', icon: '⚔️' },
  { id: 10, name: 'Yad', domain: 'Programming', color: '#06b6d4', icon: '💻' },
  { id: 11, name: 'Lashon', domain: 'Language', color: '#84cc16', icon: '📝' },
  { id: 12, name: 'Chronos', domain: 'Time', color: '#8b5cf6', icon: '⏰' },
  { id: 13, name: 'Mnemosyne', domain: 'Memory', color: '#f472b6', icon: '🧠' },
  { id: 14, name: 'Hypnos', domain: 'Rest', color: '#64748b', icon: '😴' },
  { id: 15, name: 'Thanatos', domain: 'Pruning', color: '#1e293b', icon: '🗑️' }
];

// Serve built dashboard
const distDir = path.join(DASH, 'dist');
if (fs.existsSync(distDir)) {
  app.use(express.static(distDir));
}

// State tracking for real-time updates
let currentLogFile = null;
let lastLogSize = 0;
let seriesBuffer = [];
let lastBroadcastTime = 0;

// Learning content tracking
let currentLearningFile = null;
let lastLearningSize = 0;
let learningBuffer = [];
let learningStats = {
  total: 0,
  bySource: {},
  byFamily: {},
  startTime: Date.now(),
  recentRate: 0
};
let recentLearningTimes = [];

// Server stats
const serverStats = {
  startTime: Date.now(),
  totalUpdates: 0,
  totalLearningItems: 0,
  peakClients: 0
};

// Find latest log file
function findLatestLog() {
  if (!fs.existsSync(DATA_DIR)) return null;
  try {
    const files = fs.readdirSync(DATA_DIR)
      .filter(f => f.endsWith('.log') && !f.startsWith('.'))
      .map(f => {
        try {
          const stat = fs.statSync(path.join(DATA_DIR, f));
          return { f, mtime: stat.mtimeMs, size: stat.size };
        } catch { return null; }
      })
      .filter(Boolean)
      .sort((a, b) => b.mtime - a.mtime);
    return files.length ? path.join(DATA_DIR, files[0].f) : null;
  } catch { return null; }
}

// Find learning content file (jsonl)
function findLearningFile() {
  if (!fs.existsSync(DATA_DIR)) return null;
  try {
    const files = fs.readdirSync(DATA_DIR)
      .filter(f => f.endsWith('.jsonl') && !f.startsWith('.'))
      .map(f => {
        try {
          const stat = fs.statSync(path.join(DATA_DIR, f));
          return { f, mtime: stat.mtimeMs, size: stat.size };
        } catch { return null; }
      })
      .filter(Boolean)
      .sort((a, b) => b.mtime - a.mtime);
    return files.length ? path.join(DATA_DIR, files[0].f) : null;
  } catch { return null; }
}

// Parse log line
const LINE_RE = /t=(\d+)\s+coh=([\d.eE+-]+)\s+cons=([\d.eE+-]+)\s+dx=([\d.eE+-]+)\s+err=([\d.eE+-]+)\s+stab=([\d.eE+-]+)\s*\|\s*Ω=([-.\deE+]+)\s+κ=([-.\deE+]+)\s+b=([\d.eE+-]+)\s+Φ=([\d.eE+-]+)\s+ing=(\d+)/;

function parseLine(ln) {
  const m = LINE_RE.exec(ln);
  if (!m) return null;
  return {
    t: Number(m[1]),
    coh: Number(m[2]),
    cons: Number(m[3]),
    dx: Number(m[4]),
    err: Number(m[5]),
    stab: Number(m[6]),
    omega: Number(m[7]),
    kappa: Number(m[8]),
    b: Number(m[9]),
    phi: Number(m[10]),
    ing: Number(m[11])
  };
}

// Read new lines from log file (tail -f style)
function readNewLines() {
  const logFile = findLatestLog();
  if (!logFile) return [];

  if (logFile !== currentLogFile) {
    currentLogFile = logFile;
    lastLogSize = 0;
    seriesBuffer = [];
  }

  try {
    const stat = fs.statSync(logFile);
    if (stat.size <= lastLogSize) return [];

    const fd = fs.openSync(logFile, 'r');
    const newSize = stat.size - lastLogSize;
    const buffer = Buffer.alloc(Math.min(newSize, 100000));
    fs.readSync(fd, buffer, 0, buffer.length, lastLogSize);
    fs.closeSync(fd);

    lastLogSize = stat.size;

    const newContent = buffer.toString('utf8');
    const lines = newContent.split('\n').filter(l => l.trim());
    const parsed = [];

    for (const ln of lines) {
      const data = parseLine(ln);
      if (data) {
        parsed.push(data);
        seriesBuffer.push(data);
        if (seriesBuffer.length > 5000) {
          seriesBuffer = seriesBuffer.slice(-5000);
        }
      }
    }

    serverStats.totalUpdates += parsed.length;
    return parsed;
  } catch (e) {
    return [];
  }
}

// Read new learning items from jsonl file
function readNewLearningItems() {
  const learningFile = findLearningFile();
  if (!learningFile) return [];

  if (learningFile !== currentLearningFile) {
    currentLearningFile = learningFile;
    lastLearningSize = 0;
    // Don't reset learningBuffer - keep history
  }

  try {
    const stat = fs.statSync(learningFile);
    if (stat.size <= lastLearningSize) return [];

    const fd = fs.openSync(learningFile, 'r');
    const newSize = stat.size - lastLearningSize;
    const buffer = Buffer.alloc(Math.min(newSize, 500000));
    fs.readSync(fd, buffer, 0, buffer.length, lastLearningSize);
    fs.closeSync(fd);

    lastLearningSize = stat.size;

    const newContent = buffer.toString('utf8');
    const lines = newContent.split('\n').filter(l => l.trim());
    const parsed = [];

    for (const ln of lines) {
      try {
        const item = JSON.parse(ln);
        // Parse source and title from path (e.g. "wikipedia/Article_Name.txt")
        if (item.path) {
          const parts = item.path.split('/');
          item.source = parts[0] || 'unknown';
          item.title = parts[1] ? parts[1].replace(/_/g, ' ').replace(/\.txt$/, '') : 'Untitled';
        }
        // Add metadata
        item.receivedAt = Date.now();
        item.familyMeta = FAMILY_META[item.family] || FAMILY_META[0];
        parsed.push(item);
        learningBuffer.push(item);

        // Update stats
        learningStats.total++;
        const src = item.source || 'unknown';
        learningStats.bySource[src] = (learningStats.bySource[src] || 0) + 1;
        const famName = item.familyMeta.name;
        learningStats.byFamily[famName] = (learningStats.byFamily[famName] || 0) + 1;

        // Track timing for rate calculation
        recentLearningTimes.push(Date.now());
      } catch {}
    }

    // Keep buffer limited
    if (learningBuffer.length > 500) {
      learningBuffer = learningBuffer.slice(-500);
    }

    // Keep only last 60 seconds of timing data
    const cutoff = Date.now() - 60000;
    recentLearningTimes = recentLearningTimes.filter(t => t > cutoff);
    learningStats.recentRate = recentLearningTimes.length; // items per minute

    serverStats.totalLearningItems += parsed.length;
    return parsed;
  } catch (e) {
    return [];
  }
}

// Parse binary snapshot
function parseSnapshot() {
  if (!fs.existsSync(SNAP_DIR)) return null;
  try {
    const dirs = fs.readdirSync(SNAP_DIR)
      .filter(d => {
        try { return fs.statSync(path.join(SNAP_DIR, d)).isDirectory(); } catch { return false; }
      })
      .map(d => ({ d, t: fs.statSync(path.join(SNAP_DIR, d)).mtimeMs }))
      .sort((a, b) => b.t - a.t);

    for (const { d } of dirs) {
      const p = path.join(SNAP_DIR, d, 'federation_state.bin');
      if (fs.existsSync(p)) {
        const buf = fs.readFileSync(p);
        if (buf.length < 16) continue;
        const magic = buf.readUInt32LE(0);
        if (magic !== 0x53454647) continue;

        const tick = buf.readUInt32LE(4);
        const active = buf.readUInt16LE(8);
        const phi_global = buf.readUInt8(10) / 255.0;
        const omega = buf.readInt8(11) / 2.55;

        let o = 16;
        const families = [];
        for (let f = 0; f < 16; f++) {
          const psi = buf.readUInt8(o) / 255.0; o++;
          const eta = buf.readUInt8(o) / 255.0; o++;
          families.push({ ...FAMILY_META[f], psi, eta, substrates_active: 0, phi_mean: 0 });
        }

        for (let i = 0; i < active && o + 4 <= buf.length; i++) {
          const fam = buf.readUInt8(o) & 0xF; o += 3;
          const phi = (buf.readUInt8(o - 1) & 0x7F) / 127.0; o++;
          if (fam < 16) {
            families[fam].substrates_active++;
            families[fam].phi_mean += phi;
          }
        }

        for (const f of families) {
          if (f.substrates_active > 0) f.phi_mean /= f.substrates_active;
        }

        return { tick, active, phi_global, omega, families };
      }
    }
  } catch {}
  return null;
}

// WebSocket handling
const server = http.createServer(app);
const wss = new WebSocketServer({ server, path: '/ws' });
const clients = new Set();

function broadcast(data) {
  const msg = JSON.stringify(data);
  for (const client of clients) {
    if (client.readyState === 1) {
      try { client.send(msg); } catch {}
    }
  }
}

wss.on('connection', (ws) => {
  clients.add(ws);
  serverStats.peakClients = Math.max(serverStats.peakClients, clients.size);
  console.log(`WebSocket connected (${clients.size} clients)`);

  // Send initial data immediately
  const latestSeries = seriesBuffer.slice(-200);
  const snap = parseSnapshot();
  const last = latestSeries[latestSeries.length - 1] || {};

  ws.send(JSON.stringify({
    type: 'init',
    series: latestSeries,
    learning: learningBuffer.slice(-100),
    learningStats: learningStats,
    summary: {
      tick: last.t || 0,
      phi_global: snap?.phi_global || last.phi || 0,
      omega: snap?.omega || last.omega || 0,
      ing: last.ing || 0,
      stability: last.stab || 0,
      coherence: last.coh || 0,
      consciousness: last.cons || 0,
      families: snap?.families || FAMILY_META.map(f => ({ ...f, psi: 0, eta: 0, substrates_active: 0, phi_mean: 0 })),
      active_substrates: snap?.active || 0
    },
    serverStats: {
      uptime: Date.now() - serverStats.startTime,
      totalUpdates: serverStats.totalUpdates,
      totalLearningItems: serverStats.totalLearningItems,
      peakClients: serverStats.peakClients
    },
    families: FAMILY_META
  }));

  ws.on('close', () => {
    clients.delete(ws);
    console.log(`WebSocket disconnected (${clients.size} clients)`);
  });
  ws.on('error', () => clients.delete(ws));
});

// REAL-TIME UPDATE LOOP - runs every 50ms (20 updates/sec)
setInterval(() => {
  const newLines = readNewLines();
  const newLearning = readNewLearningItems();

  if (newLines.length > 0 || newLearning.length > 0) {
    const updateMsg = {
      type: 'update',
      timestamp: Date.now()
    };

    if (newLines.length > 0) {
      updateMsg.series = newLines;
      updateMsg.latest = newLines[newLines.length - 1];
    }

    if (newLearning.length > 0) {
      updateMsg.learning = newLearning;
      updateMsg.learningStats = learningStats;
    }

    broadcast(updateMsg);

    // Also broadcast full summary periodically
    const now = Date.now();
    if (now - lastBroadcastTime > 1000) {
      lastBroadcastTime = now;
      const snap = parseSnapshot();
      const last = newLines[newLines.length - 1] || seriesBuffer[seriesBuffer.length - 1] || {};
      broadcast({
        type: 'summary',
        summary: {
          tick: last.t || 0,
          phi_global: snap?.phi_global || last.phi || 0,
          omega: snap?.omega || last.omega || 0,
          ing: last.ing || 0,
          stability: last.stab || 0,
          coherence: last.coh || 0,
          consciousness: last.cons || 0,
          families: snap?.families || FAMILY_META.map(f => ({ ...f, psi: 0, eta: 0, substrates_active: 0, phi_mean: 0 })),
          active_substrates: snap?.active || 0
        },
        serverStats: {
          uptime: Date.now() - serverStats.startTime,
          totalUpdates: serverStats.totalUpdates,
          totalLearningItems: serverStats.totalLearningItems,
          clients: clients.size
        }
      });
    }
  }
}, 50); // 20 updates per second

// API routes
app.get('/api/series', (req, res) => {
  const last = Math.min(5000, Number(req.query.last || 600));
  res.json(seriesBuffer.slice(-last));
});

app.get('/api/learning', (req, res) => {
  const last = Math.min(500, Number(req.query.last || 100));
  res.json({
    items: learningBuffer.slice(-last),
    stats: learningStats
  });
});

app.get('/api/summary', (req, res) => {
  const snap = parseSnapshot();
  const last = seriesBuffer[seriesBuffer.length - 1] || {};
  res.json({
    tick: last.t || 0,
    phi_global: snap?.phi_global || last.phi || 0,
    omega: snap?.omega || last.omega || 0,
    ing: last.ing || 0,
    b: last.b || 1,
    kappa: last.kappa || 0,
    stability: last.stab || 0,
    coherence: last.coh || 0,
    consciousness: last.cons || 0,
    families: snap?.families || FAMILY_META.map(f => ({ ...f, psi: 0, eta: 0, substrates_active: 0, phi_mean: 0 })),
    active_substrates: snap?.active || 0,
    learningStats: learningStats,
    timestamp: Date.now()
  });
});

app.get('/api/families', (req, res) => {
  res.json(FAMILY_META);
});

app.get('/api/health', (req, res) => {
  res.json({
    status: seriesBuffer.length > 0 ? 'healthy' : 'waiting',
    clients: clients.size,
    bufferSize: seriesBuffer.length,
    learningBufferSize: learningBuffer.length,
    logFile: currentLogFile ? path.basename(currentLogFile) : null,
    learningFile: currentLearningFile ? path.basename(currentLearningFile) : null,
    uptime: Date.now() - serverStats.startTime,
    totalUpdates: serverStats.totalUpdates,
    learningRate: learningStats.recentRate
  });
});

app.get('/api/export', (req, res) => {
  const format = req.query.format || 'json';
  const data = seriesBuffer.slice(-5000);
  if (format === 'csv') {
    const headers = ['t', 'coh', 'cons', 'dx', 'err', 'stab', 'omega', 'kappa', 'b', 'phi', 'ing'];
    res.setHeader('Content-Type', 'text/csv');
    res.send([headers.join(','), ...data.map(r => headers.map(h => r[h] ?? '').join(','))].join('\n'));
  } else {
    res.json(data);
  }
});

app.get('/metrics', (req, res) => {
  const last = seriesBuffer[seriesBuffer.length - 1] || {};
  res.type('text/plain').send([
    `seferim_tick ${last.t || 0}`,
    `seferim_phi ${last.phi || 0}`,
    `seferim_omega ${last.omega || 0}`,
    `seferim_ingested ${last.ing || 0}`,
    `seferim_stability ${last.stab || 0}`,
    `seferim_ws_clients ${clients.size}`,
    `seferim_learning_total ${learningStats.total}`,
    `seferim_learning_rate ${learningStats.recentRate}`
  ].join('\n'));
});

app.get('*', (req, res) => {
  const indexPath = path.join(distDir, 'index.html');
  if (fs.existsSync(indexPath)) {
    res.sendFile(indexPath);
  } else {
    res.send(`<html><body style="background:#080a10;color:#fff;font-family:system-ui;display:flex;justify-content:center;align-items:center;height:100vh"><div style="text-align:center"><h2>SEFERIM Dashboard</h2><p>Run: cd dashboard && npm run build</p></div></body></html>`);
  }
});

// Initialize by reading existing data
readNewLines();
readNewLearningItems();

server.listen(PORT, () => {
  console.log(`
╔═══════════════════════════════════════════════════════════╗
║        SEFERIM Real-Time Dashboard Server v5.0            ║
╠═══════════════════════════════════════════════════════════╣
║  Dashboard:  http://localhost:${PORT}/                       ║
║  WebSocket:  ws://localhost:${PORT}/ws (50ms / 20 ups)       ║
║  API:        http://localhost:${PORT}/api/summary            ║
║  Learning:   http://localhost:${PORT}/api/learning           ║
╚═══════════════════════════════════════════════════════════╝
  `);
});

process.on('SIGTERM', () => process.exit(0));
process.on('SIGINT', () => process.exit(0));
