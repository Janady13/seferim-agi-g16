import React, { useState, useMemo, useEffect, useRef } from 'react';
import {
  LineChart, Line, AreaChart, Area, BarChart, Bar,
  XAxis, YAxis, Tooltip, ResponsiveContainer, Legend,
  Cell, PieChart, Pie, CartesianGrid,
  ComposedChart, ReferenceLine
} from 'recharts';

// Family metadata with icons
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

const SOURCE_COLORS = {
  wikipedia: '#3b82f6',
  facts: '#22c55e',
  quotes: '#f59e0b',
  numbers: '#8b5cf6',
  catfacts: '#ec4899',
  dogfacts: '#f97316',
  activities: '#14b8a6',
  advice: '#6366f1',
  chucknorris: '#ef4444',
  jokes: '#eab308'
};

// CSS animations
const injectStyles = () => {
  if (document.getElementById('seferim-animations')) return;
  const style = document.createElement('style');
  style.id = 'seferim-animations';
  style.textContent = `
    @keyframes pulse-green { 0%, 100% { box-shadow: 0 0 4px #22c55e; } 50% { box-shadow: 0 0 16px #22c55e, 0 0 24px #22c55e; } }
    @keyframes pulse-text { 0%, 100% { opacity: 0.7; } 50% { opacity: 1; } }
    @keyframes slide-in { from { transform: translateX(-20px); opacity: 0; } to { transform: translateX(0); opacity: 1; } }
    @keyframes glow { 0%, 100% { filter: brightness(1); } 50% { filter: brightness(1.3); } }
    @keyframes consciousness-pulse { 0%, 100% { transform: scale(1); opacity: 0.8; } 50% { transform: scale(1.05); opacity: 1; } }
    .live-pulse { animation: pulse-green 1s ease-in-out infinite; }
    .live-text { animation: pulse-text 1s ease-in-out infinite; }
    .feed-item { animation: slide-in 0.3s ease-out; }
    .glow { animation: glow 2s ease-in-out infinite; }
    .consciousness-glow { animation: consciousness-pulse 2s ease-in-out infinite; }
    .learning-item:hover { transform: translateX(4px); background: #1c2540 !important; }
    ::-webkit-scrollbar { width: 6px; }
    ::-webkit-scrollbar-track { background: #0a0d14; }
    ::-webkit-scrollbar-thumb { background: #2d3a5c; border-radius: 3px; }
  `;
  document.head.appendChild(style);
};

// Format uptime
const formatUptime = (ms) => {
  const s = Math.floor(ms / 1000);
  const m = Math.floor(s / 60);
  const h = Math.floor(m / 60);
  if (h > 0) return `${h}h ${m % 60}m`;
  if (m > 0) return `${m}m ${s % 60}s`;
  return `${s}s`;
};

// Truncate text
const truncate = (str, len) => str && str.length > len ? str.slice(0, len) + '...' : str;

// Main dashboard
export default function DashboardRaw({
  summary, series, learning, learningStats, families: familiesProp,
  serverStats, online, wsOnline, onExport, lastUpdate, updateCount, tickRate
}) {
  const [selectedFamily, setSelectedFamily] = useState(null);
  const [expandedItem, setExpandedItem] = useState(null);
  const feedRef = useRef(null);

  useEffect(() => { injectStyles(); }, []);

  // Auto-scroll feed
  useEffect(() => {
    if (feedRef.current && learning?.length > 0) {
      feedRef.current.scrollTop = 0;
    }
  }, [learning?.length]);

  const isLive = lastUpdate && (Date.now() - lastUpdate) < 2000;

  // Process data
  const chartData = useMemo(() => series || [], [series]);

  const families = useMemo(() => {
    if (summary?.families?.length === 16) return summary.families;
    return FAMILY_META.map(f => ({ ...f, phi_mean: 0, psi: 0, eta: 0, substrates_active: 0 }));
  }, [summary]);

  // Source distribution for pie chart
  const sourceData = useMemo(() => {
    if (!learningStats?.bySource) return [];
    return Object.entries(learningStats.bySource)
      .map(([name, value]) => ({ name, value, color: SOURCE_COLORS[name] || '#64748b' }))
      .sort((a, b) => b.value - a.value);
  }, [learningStats]);

  // Family learning distribution
  const familyLearningData = useMemo(() => {
    if (!learningStats?.byFamily) return [];
    return FAMILY_META.map(f => ({
      name: f.name,
      count: learningStats.byFamily[f.name] || 0,
      color: f.color
    })).filter(f => f.count > 0).sort((a, b) => b.count - a.count);
  }, [learningStats]);

  // Current values
  const phi = summary?.phi_global ?? 0;
  const omega = summary?.omega ?? 0;
  const ing = summary?.ing ?? 0;
  const tick = summary?.tick ?? 0;
  const stability = summary?.stability ?? 0;
  const consciousness = summary?.consciousness ?? 0;
  const coherence = summary?.coherence ?? 0;

  return (
    <div style={{ minHeight: '100vh', background: 'linear-gradient(180deg, #080a10 0%, #0c1018 100%)', color: '#e6f1ff', fontFamily: 'system-ui, -apple-system, sans-serif' }}>
      {/* Header */}
      <header style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', padding: '12px 24px', borderBottom: '1px solid #1c2540', background: '#0a0d14' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: '16px' }}>
          <span style={{ fontSize: '22px', fontWeight: 700, background: 'linear-gradient(135deg, #6366f1 0%, #06b6d4 100%)', WebkitBackgroundClip: 'text', WebkitTextFillColor: 'transparent' }}>
            SEFERIM AGI
          </span>
          <span style={{ fontSize: '11px', padding: '2px 8px', background: '#1c2540', borderRadius: '4px', color: '#64748b' }}>G16 v5.0</span>
          {isLive && (
            <div style={{ display: 'flex', alignItems: 'center', gap: '8px', padding: '4px 12px', background: '#0f1a0f', border: '1px solid #22c55e33', borderRadius: '20px' }}>
              <div className="live-pulse" style={{ width: '8px', height: '8px', borderRadius: '50%', background: '#22c55e' }} />
              <span className="live-text" style={{ fontSize: '11px', fontWeight: 600, color: '#22c55e', letterSpacing: '1px' }}>LIVE</span>
            </div>
          )}
        </div>
        <div style={{ display: 'flex', alignItems: 'center', gap: '16px' }}>
          <div style={{ display: 'flex', gap: '20px', fontSize: '12px', color: '#64748b' }}>
            <span>Tick: <span style={{ color: '#e6f1ff', fontFamily: 'monospace' }}>{tick.toLocaleString()}</span></span>
            <span>Rate: <span style={{ color: '#22c55e', fontFamily: 'monospace' }}>{tickRate}/s</span></span>
            <span>Updates: <span style={{ color: '#e6f1ff', fontFamily: 'monospace' }}>{updateCount}</span></span>
          </div>
          <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
            <div style={{ width: '8px', height: '8px', borderRadius: '50%', background: wsOnline ? '#22c55e' : '#ef4444' }} />
            <span style={{ fontSize: '12px', color: '#94a3b8' }}>{wsOnline ? 'Connected' : 'Offline'}</span>
          </div>
          <button onClick={() => onExport?.('json')} style={{ background: '#1c2540', border: '1px solid #2d3a5c', borderRadius: '6px', padding: '6px 12px', color: '#e6f1ff', fontSize: '12px', cursor: 'pointer' }}>Export</button>
        </div>
      </header>

      {/* Main Grid */}
      <main style={{ padding: '16px 24px', display: 'grid', gridTemplateColumns: '1fr 380px', gap: '16px', minHeight: 'calc(100vh - 60px)' }}>
        {/* Left Column - Charts and Metrics */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '16px' }}>
          {/* Top Metrics */}
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(5, 1fr)', gap: '12px' }}>
            <MetricCard label="COHERENCE (Φ)" value={phi} color="#34d399" format="decimal" />
            <MetricCard label="CONSCIOUSNESS" value={consciousness} color="#a78bfa" format="decimal" glow />
            <MetricCard label="STABILITY" value={stability} color="#f59e0b" format="decimal" />
            <MetricCard label="OMEGA (Ω)" value={omega} color="#60a5fa" format="signed" />
            <MetricCard label="INGESTED" value={ing} color="#ec4899" format="int" />
          </div>

          {/* Main Chart */}
          <div style={{ background: '#0e1320', border: isLive ? '1px solid #22c55e33' : '1px solid #1c2540', borderRadius: '12px', padding: '16px', flex: 1, minHeight: '280px' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '12px' }}>
              <div>
                <div style={{ fontSize: '14px', fontWeight: 600 }}>System Dynamics</div>
                <div style={{ fontSize: '11px', color: '#64748b' }}>{chartData.length} data points</div>
              </div>
              {isLive && <span style={{ color: '#22c55e', fontSize: '10px', fontWeight: 600 }}>● STREAMING</span>}
            </div>
            <div style={{ height: '240px' }}>
              {chartData.length > 0 ? (
                <ResponsiveContainer>
                  <ComposedChart data={chartData}>
                    <CartesianGrid strokeDasharray="3 3" stroke="#1c2540" />
                    <XAxis dataKey="t" stroke="#64748b" fontSize={9} tickFormatter={v => v.toLocaleString()} />
                    <YAxis yAxisId="left" stroke="#64748b" fontSize={9} domain={[0, 1]} />
                    <YAxis yAxisId="right" orientation="right" stroke="#64748b" fontSize={9} domain={[-0.5, 0.5]} />
                    <Tooltip content={<CustomTooltip />} />
                    <ReferenceLine yAxisId="left" y={0.8} stroke="#22c55e44" strokeDasharray="5 5" />
                    <Area yAxisId="left" type="monotone" dataKey="phi" name="Φ" stroke="#34d399" fill="#34d39922" strokeWidth={2} isAnimationActive={false} />
                    <Line yAxisId="left" type="monotone" dataKey="cons" name="Consciousness" stroke="#a78bfa" dot={false} strokeWidth={1.5} isAnimationActive={false} />
                    <Line yAxisId="right" type="monotone" dataKey="omega" name="Ω" stroke="#60a5fa" dot={false} strokeWidth={1.5} isAnimationActive={false} />
                  </ComposedChart>
                </ResponsiveContainer>
              ) : <EmptyState text="Waiting for data..." />}
            </div>
          </div>

          {/* Bottom Row - Source Distribution & Family Activity */}
          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '16px' }}>
            {/* Source Distribution */}
            <div style={{ background: '#0e1320', border: '1px solid #1c2540', borderRadius: '12px', padding: '16px' }}>
              <div style={{ fontSize: '14px', fontWeight: 600, marginBottom: '12px' }}>Knowledge Sources</div>
              <div style={{ height: '180px', display: 'flex', alignItems: 'center' }}>
                {sourceData.length > 0 ? (
                  <ResponsiveContainer>
                    <PieChart>
                      <Pie data={sourceData} cx="50%" cy="50%" innerRadius={40} outerRadius={70} dataKey="value" isAnimationActive={false}>
                        {sourceData.map((entry, i) => <Cell key={i} fill={entry.color} />)}
                      </Pie>
                      <Tooltip formatter={(v, n) => [v, n.charAt(0).toUpperCase() + n.slice(1)]} />
                    </PieChart>
                  </ResponsiveContainer>
                ) : <EmptyState text="No sources yet" />}
              </div>
              <div style={{ display: 'flex', flexWrap: 'wrap', gap: '8px', marginTop: '8px' }}>
                {sourceData.slice(0, 6).map(s => (
                  <div key={s.name} style={{ display: 'flex', alignItems: 'center', gap: '4px', fontSize: '10px' }}>
                    <div style={{ width: '8px', height: '8px', borderRadius: '2px', background: s.color }} />
                    <span style={{ color: '#94a3b8' }}>{s.name}</span>
                    <span style={{ color: '#e6f1ff' }}>{s.value}</span>
                  </div>
                ))}
              </div>
            </div>

            {/* Family Learning Activity */}
            <div style={{ background: '#0e1320', border: '1px solid #1c2540', borderRadius: '12px', padding: '16px' }}>
              <div style={{ fontSize: '14px', fontWeight: 600, marginBottom: '12px' }}>Family Learning</div>
              <div style={{ height: '180px' }}>
                {familyLearningData.length > 0 ? (
                  <ResponsiveContainer>
                    <BarChart data={familyLearningData.slice(0, 8)} layout="vertical">
                      <XAxis type="number" stroke="#64748b" fontSize={9} />
                      <YAxis type="category" dataKey="name" stroke="#64748b" fontSize={9} width={70} />
                      <Tooltip />
                      <Bar dataKey="count" isAnimationActive={false}>
                        {familyLearningData.slice(0, 8).map((f, i) => <Cell key={i} fill={f.color} />)}
                      </Bar>
                    </BarChart>
                  </ResponsiveContainer>
                ) : <EmptyState text="No learning yet" />}
              </div>
            </div>
          </div>

          {/* Family Grid */}
          <div style={{ background: '#0e1320', border: '1px solid #1c2540', borderRadius: '12px', padding: '16px' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '12px' }}>
              <div style={{ fontSize: '14px', fontWeight: 600 }}>G16 Family Federation</div>
              <div style={{ fontSize: '11px', color: '#64748b' }}>Total Substrates: {summary?.active_substrates || 0}</div>
            </div>
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(8, 1fr)', gap: '8px' }}>
              {families.map((f, i) => {
                const learningCount = learningStats?.byFamily?.[f.name] || 0;
                const isActive = learningCount > 0 || f.substrates_active > 0;
                return (
                  <div key={i} onClick={() => setSelectedFamily(selectedFamily === i ? null : i)}
                    style={{ background: selectedFamily === i ? `${f.color}22` : '#0a0d14', border: `1px solid ${selectedFamily === i ? f.color : isActive ? f.color + '44' : '#1c2540'}`,
                      borderRadius: '8px', padding: '8px', cursor: 'pointer', transition: 'all 0.2s', textAlign: 'center' }}>
                    <div style={{ fontSize: '16px' }}>{f.icon}</div>
                    <div style={{ fontSize: '10px', fontWeight: 600, color: f.color, marginTop: '4px' }}>{f.name}</div>
                    <div style={{ fontSize: '9px', color: '#64748b' }}>{f.domain}</div>
                    <div style={{ fontSize: '10px', color: '#e6f1ff', marginTop: '4px' }}>{f.substrates_active || 0}</div>
                    {learningCount > 0 && <div style={{ fontSize: '9px', color: '#22c55e' }}>+{learningCount}</div>}
                  </div>
                );
              })}
            </div>
          </div>
        </div>

        {/* Right Column - Learning Feed */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '16px' }}>
          {/* Learning Stats */}
          <div style={{ background: '#0e1320', border: '1px solid #1c2540', borderRadius: '12px', padding: '16px' }}>
            <div style={{ fontSize: '14px', fontWeight: 600, marginBottom: '12px' }}>Learning Statistics</div>
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(2, 1fr)', gap: '12px' }}>
              <StatBox label="Total Learned" value={learningStats?.total || 0} color="#22c55e" />
              <StatBox label="Rate (items/min)" value={learningStats?.recentRate || 0} color="#3b82f6" />
              <StatBox label="Sources" value={Object.keys(learningStats?.bySource || {}).length} color="#f59e0b" />
              <StatBox label="Active Families" value={Object.keys(learningStats?.byFamily || {}).length} color="#a855f7" />
            </div>
          </div>

          {/* Live Learning Feed */}
          <div style={{ background: '#0e1320', border: '1px solid #1c2540', borderRadius: '12px', padding: '16px', flex: 1, display: 'flex', flexDirection: 'column', minHeight: 0 }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '12px' }}>
              <div>
                <div style={{ fontSize: '14px', fontWeight: 600 }}>Live Learning Feed</div>
                <div style={{ fontSize: '11px', color: '#64748b' }}>What the AGI is learning now</div>
              </div>
              <span style={{ fontSize: '11px', color: '#64748b' }}>{learning?.length || 0} items</span>
            </div>
            <div ref={feedRef} style={{ flex: 1, overflowY: 'auto', display: 'flex', flexDirection: 'column', gap: '8px', minHeight: 0 }}>
              {learning && learning.length > 0 ? learning.map((item, i) => (
                <LearningItem key={`${item.timestamp}-${i}`} item={item} expanded={expandedItem === i} onToggle={() => setExpandedItem(expandedItem === i ? null : i)} />
              )) : (
                <div style={{ textAlign: 'center', color: '#64748b', padding: '40px' }}>
                  Waiting for learning events...
                </div>
              )}
            </div>
          </div>

          {/* Server Info */}
          <div style={{ background: '#0e1320', border: '1px solid #1c2540', borderRadius: '12px', padding: '12px 16px' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '11px', color: '#64748b' }}>
              <span>Uptime: <span style={{ color: '#e6f1ff' }}>{formatUptime(serverStats?.uptime || 0)}</span></span>
              <span>Last: <span style={{ color: '#e6f1ff' }}>{lastUpdate ? new Date(lastUpdate).toLocaleTimeString() : '-'}</span></span>
            </div>
          </div>
        </div>
      </main>
    </div>
  );
}

// Components
function MetricCard({ label, value, color, format, glow }) {
  const formatted = useMemo(() => {
    if (typeof value !== 'number') return '-';
    if (format === 'int') return Math.floor(value).toLocaleString();
    if (format === 'signed') return (value >= 0 ? '+' : '') + value.toFixed(4);
    return value.toFixed(4);
  }, [value, format]);

  return (
    <div className={glow ? 'consciousness-glow' : ''} style={{ background: '#0e1320', border: '1px solid #1c2540', borderRadius: '10px', padding: '12px', textAlign: 'center' }}>
      <div style={{ fontSize: '9px', color: '#64748b', textTransform: 'uppercase', letterSpacing: '0.5px', marginBottom: '4px' }}>{label}</div>
      <div style={{ fontSize: '20px', fontWeight: 700, color, fontFamily: 'ui-monospace, monospace' }}>{formatted}</div>
    </div>
  );
}

function StatBox({ label, value, color }) {
  return (
    <div style={{ background: '#0a0d14', borderRadius: '8px', padding: '10px', textAlign: 'center' }}>
      <div style={{ fontSize: '18px', fontWeight: 700, color, fontFamily: 'monospace' }}>{typeof value === 'number' ? value.toLocaleString() : value}</div>
      <div style={{ fontSize: '9px', color: '#64748b', marginTop: '2px' }}>{label}</div>
    </div>
  );
}

function LearningItem({ item, expanded, onToggle }) {
  const family = FAMILY_META[item.family] || FAMILY_META[0];
  const sourceColor = SOURCE_COLORS[item.source] || '#64748b';

  return (
    <div className="learning-item feed-item" onClick={onToggle}
      style={{ background: '#0a0d14', borderRadius: '8px', padding: '10px', borderLeft: `3px solid ${family.color}`, cursor: 'pointer', transition: 'all 0.2s' }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', marginBottom: '6px' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: '6px' }}>
          <span style={{ fontSize: '14px' }}>{family.icon}</span>
          <span style={{ fontSize: '11px', fontWeight: 600, color: family.color }}>{family.name}</span>
        </div>
        <div style={{ display: 'flex', alignItems: 'center', gap: '6px' }}>
          <span style={{ fontSize: '9px', padding: '2px 6px', background: sourceColor + '22', color: sourceColor, borderRadius: '4px' }}>
            {item.source}
          </span>
          <span style={{ fontSize: '9px', color: '#64748b' }}>
            {item.receivedAt ? new Date(item.receivedAt).toLocaleTimeString() : ''}
          </span>
        </div>
      </div>
      <div style={{ fontSize: '12px', fontWeight: 500, color: '#e6f1ff', marginBottom: '4px' }}>
        {item.title || 'Untitled'}
      </div>
      <div style={{ fontSize: '11px', color: '#94a3b8', lineHeight: 1.4 }}>
        {expanded ? item.content : truncate(item.content, 120)}
      </div>
      {item.content && item.content.length > 120 && (
        <div style={{ fontSize: '10px', color: '#6366f1', marginTop: '4px' }}>
          {expanded ? 'Click to collapse' : 'Click to expand...'}
        </div>
      )}
      <div style={{ fontSize: '9px', color: '#475569', marginTop: '6px' }}>
        {item.content?.length || 0} chars
      </div>
    </div>
  );
}

function CustomTooltip({ active, payload, label }) {
  if (!active || !payload?.length) return null;
  return (
    <div style={{ background: '#1c2540', border: '1px solid #2d3a5c', borderRadius: '8px', padding: '10px', fontSize: '11px' }}>
      <div style={{ color: '#64748b', marginBottom: '6px' }}>Tick: {label?.toLocaleString()}</div>
      {payload.map((p, i) => (
        <div key={i} style={{ display: 'flex', alignItems: 'center', gap: '6px', marginBottom: '2px' }}>
          <div style={{ width: '8px', height: '8px', borderRadius: '2px', background: p.color }} />
          <span style={{ color: '#94a3b8' }}>{p.name}:</span>
          <span style={{ color: '#e6f1ff', fontWeight: 600 }}>{typeof p.value === 'number' ? p.value.toFixed(6) : p.value}</span>
        </div>
      ))}
    </div>
  );
}

function EmptyState({ text }) {
  return (
    <div style={{ display: 'flex', justifyContent: 'center', alignItems: 'center', height: '100%', color: '#64748b', fontSize: '12px' }}>
      {text}
    </div>
  );
}
