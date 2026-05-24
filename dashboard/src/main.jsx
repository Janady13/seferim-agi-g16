import React from 'react';
import { createRoot } from 'react-dom/client';
import SeferimDashboard from './seferim_dashboard_v3.jsx';

function App() {
  // Determine WebSocket URL based on current location
  const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  const wsHost = window.location.host || 'localhost:9306';
  const wsBase = `${wsProtocol}//${wsHost}/ws`;

  return <SeferimDashboard apiBase="/api" wsBase={wsBase} />;
}

const root = createRoot(document.getElementById('root'));
root.render(<App />);

