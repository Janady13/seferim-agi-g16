import React, { useEffect, useRef, useState, useCallback } from 'react';
import Dashboard from './seferim_dashboard_v3_raw.jsx';

export default function SeferimDashboard({ apiBase = '/api' }) {
  const [summary, setSummary] = useState(null);
  const [series, setSeries] = useState([]);
  const [learning, setLearning] = useState([]);
  const [learningStats, setLearningStats] = useState(null);
  const [families, setFamilies] = useState([]);
  const [serverStats, setServerStats] = useState(null);
  const [connected, setConnected] = useState(false);
  const [wsConnected, setWsConnected] = useState(false);
  const [lastUpdate, setLastUpdate] = useState(null);
  const [updateCount, setUpdateCount] = useState(0);
  const [tickRate, setTickRate] = useState(0);

  const wsRef = useRef(null);
  const reconnectTimeout = useRef(null);
  const maxSeriesLength = 1000;
  const tickTimes = useRef([]);

  // Calculate tick rate
  const updateTickRate = useCallback((tick) => {
    const now = Date.now();
    tickTimes.current.push({ tick, time: now });
    // Keep last 5 seconds
    const cutoff = now - 5000;
    tickTimes.current = tickTimes.current.filter(t => t.time > cutoff);
    if (tickTimes.current.length >= 2) {
      const first = tickTimes.current[0];
      const last = tickTimes.current[tickTimes.current.length - 1];
      const tickDiff = last.tick - first.tick;
      const timeDiff = (last.time - first.time) / 1000;
      if (timeDiff > 0) {
        setTickRate(Math.round(tickDiff / timeDiff));
      }
    }
  }, []);

  // Connect WebSocket
  const connectWebSocket = useCallback(() => {
    if (wsRef.current?.readyState === WebSocket.OPEN) return;

    const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${wsProtocol}//${window.location.host}/ws`;

    try {
      const ws = new WebSocket(wsUrl);
      wsRef.current = ws;

      ws.onopen = () => {
        console.log('WebSocket connected');
        setWsConnected(true);
        setConnected(true);
      };

      ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data);
          setLastUpdate(Date.now());
          setUpdateCount(c => c + 1);

          if (msg.type === 'init') {
            // Initial data load
            if (msg.series) {
              setSeries(msg.series.slice(-maxSeriesLength));
              if (msg.series.length > 0) {
                updateTickRate(msg.series[msg.series.length - 1].t);
              }
            }
            if (msg.summary) {
              setSummary(msg.summary);
            }
            if (msg.learning) {
              setLearning(msg.learning);
            }
            if (msg.learningStats) {
              setLearningStats(msg.learningStats);
            }
            if (msg.families) {
              setFamilies(msg.families);
            }
            if (msg.serverStats) {
              setServerStats(msg.serverStats);
            }
          } else if (msg.type === 'update') {
            // Real-time update
            if (msg.series && msg.series.length > 0) {
              setSeries(prev => {
                const combined = [...prev, ...msg.series];
                return combined.slice(-maxSeriesLength);
              });
              updateTickRate(msg.series[msg.series.length - 1].t);
            }
            if (msg.latest) {
              setSummary(prev => ({
                ...prev,
                tick: msg.latest.t,
                phi_global: msg.latest.phi,
                omega: msg.latest.omega,
                ing: msg.latest.ing,
                stability: msg.latest.stab,
                coherence: msg.latest.coh,
                consciousness: msg.latest.cons
              }));
            }
            if (msg.learning && msg.learning.length > 0) {
              setLearning(prev => [...msg.learning, ...prev].slice(0, 200));
            }
            if (msg.learningStats) {
              setLearningStats(msg.learningStats);
            }
          } else if (msg.type === 'summary') {
            setSummary(msg.summary);
            if (msg.serverStats) {
              setServerStats(msg.serverStats);
            }
          }
        } catch (e) {
          console.error('WebSocket message error:', e);
        }
      };

      ws.onclose = () => {
        console.log('WebSocket disconnected');
        setWsConnected(false);
        wsRef.current = null;
        // Reconnect after 1 second
        reconnectTimeout.current = setTimeout(connectWebSocket, 1000);
      };

      ws.onerror = () => {
        setWsConnected(false);
      };
    } catch (e) {
      console.error('WebSocket connection error:', e);
      reconnectTimeout.current = setTimeout(connectWebSocket, 2000);
    }
  }, [updateTickRate]);

  // Initial connection
  useEffect(() => {
    connectWebSocket();

    return () => {
      clearTimeout(reconnectTimeout.current);
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, [connectWebSocket]);

  // Export handler
  const handleExport = useCallback((format) => {
    window.open(`${apiBase}/export?format=${format}`, '_blank');
  }, [apiBase]);

  return (
    <Dashboard
      summary={summary}
      series={series}
      learning={learning}
      learningStats={learningStats}
      families={families}
      serverStats={serverStats}
      online={connected}
      wsOnline={wsConnected}
      onExport={handleExport}
      lastUpdate={lastUpdate}
      updateCount={updateCount}
      tickRate={tickRate}
    />
  );
}
