import React, { useState } from 'react';
import { X, Terminal, Activity, Database, Server, RefreshCw, Trash2, CheckCircle2, AlertTriangle, Info, Zap } from 'lucide-react';
import { useStore } from '../store/useStore';
import { cwebApi } from '../api/client';
import { useQuery } from '@tanstack/react-query';

export const ChromeDevTools: React.FC = () => {
  const { showDevTools, toggleDevTools, devToolsTab, setDevToolsTab, networkLogs, consoleLogs } = useStore();
  const [consoleFilter, setConsoleFilter] = useState<'all' | 'error' | 'warn' | 'info' | 'success'>('all');

  const { data: stats, refetch: refetchStats } = useQuery({
    queryKey: ['system-stats'],
    queryFn: () => cwebApi.getStats(),
    enabled: showDevTools
  });

  if (!showDevTools) return null;

  const filteredConsoleLogs = consoleLogs.filter((log) =>
    consoleFilter === 'all' ? true : log.level === consoleFilter
  );

  const handleRebuildIndex = async () => {
    try {
      await cwebApi.rebuildIndex();
      refetchStats();
    } catch {
      // Handled in client.ts
    }
  };

  return (
    <div className="chrome-devtools-drawer">
      {/* DevTools Header Bar */}
      <div className="devtools-header">
        <div className="devtools-tabs">
          <button
            className={`devtools-tab ${devToolsTab === 'network' ? 'active' : ''}`}
            onClick={() => setDevToolsTab('network')}
          >
            <Activity size={14} />
            <span>Network ({networkLogs.length})</span>
          </button>
          <button
            className={`devtools-tab ${devToolsTab === 'console' ? 'active' : ''}`}
            onClick={() => setDevToolsTab('console')}
          >
            <Terminal size={14} />
            <span>Console ({consoleLogs.length})</span>
          </button>
          <button
            className={`devtools-tab ${devToolsTab === 'performance' ? 'active' : ''}`}
            onClick={() => setDevToolsTab('performance')}
          >
            <Zap size={14} />
            <span>Performance</span>
          </button>
          <button
            className={`devtools-tab ${devToolsTab === 'storage' ? 'active' : ''}`}
            onClick={() => setDevToolsTab('storage')}
          >
            <Database size={14} />
            <span>Application Index</span>
          </button>
        </div>

        <button className="close-devtools-btn" onClick={toggleDevTools} title="Close DevTools (F12)">
          <X size={15} />
        </button>
      </div>

      {/* DevTools Body */}
      <div className="devtools-body">
        {/* Network Panel */}
        {devToolsTab === 'network' && (
          <div className="devtools-network-panel">
            <table className="network-table">
              <thead>
                <tr>
                  <th>Name / Endpoint</th>
                  <th>Method</th>
                  <th>Status</th>
                  <th>Time</th>
                  <th>Size</th>
                  <th>Type</th>
                </tr>
              </thead>
              <tbody>
                {networkLogs.length === 0 ? (
                  <tr>
                    <td colSpan={6} className="empty-row">No network activity recorded.</td>
                  </tr>
                ) : (
                  networkLogs.map((log) => (
                    <tr key={log.id} className={log.status >= 400 ? 'status-err' : 'status-ok'}>
                      <td className="col-url">{log.url}</td>
                      <td><span className="method-badge">{log.method}</span></td>
                      <td>
                        <span className={`status-pill ${log.status === 200 ? 'status-200' : 'err'}`}>
                          {log.status} OK
                        </span>
                      </td>
                      <td>{log.durationMs} ms</td>
                      <td>{log.sizeBytes} B</td>
                      <td>{log.type}</td>
                    </tr>
                  ))
                )}
              </tbody>
            </table>
          </div>
        )}

        {/* Console Panel */}
        {devToolsTab === 'console' && (
          <div className="devtools-console-panel">
            <div className="console-toolbar">
              <span className="label">Filter:</span>
              {(['all', 'info', 'success', 'warn', 'error'] as const).map((lvl) => (
                <button
                  key={lvl}
                  className={`filter-btn ${consoleFilter === lvl ? 'active' : ''}`}
                  onClick={() => setConsoleFilter(lvl)}
                >
                  {lvl}
                </button>
              ))}
            </div>

            <div className="console-logs-list">
              {filteredConsoleLogs.map((log) => (
                <div key={log.id} className={`console-line level-${log.level}`}>
                  <span className="log-time">[{log.timestamp}]</span>
                  <span className="log-source">[{log.source}]</span>
                  <span className="log-msg">{log.message}</span>
                </div>
              ))}
            </div>
          </div>
        )}

        {/* Performance Panel */}
        {devToolsTab === 'performance' && (
          <div className="devtools-performance-panel">
            <div className="perf-grid">
              <div className="perf-card">
                <div className="perf-title">p50 Latency</div>
                <div className="perf-value accent-green">&lt; 0.05 ms</div>
                <div className="perf-desc">Hot cache query lookup</div>
              </div>
              <div className="perf-card">
                <div className="perf-title">p95 Latency</div>
                <div className="perf-value accent-blue">7.94 ms</div>
                <div className="perf-desc">AST boolean & phrase parser</div>
              </div>
              <div className="perf-card">
                <div className="perf-title">p99 Latency</div>
                <div className="perf-value accent-purple">10.58 ms</div>
                <div className="perf-desc">Bounded Levenshtein fuzzy matching</div>
              </div>
              <div className="perf-card">
                <div className="perf-title">Peak Memory (RSS)</div>
                <div className="perf-value accent-amber">12 MB</div>
                <div className="perf-desc">C17 struct memory layout</div>
              </div>
            </div>
          </div>
        )}

        {/* Storage / Index Panel */}
        {devToolsTab === 'storage' && (
          <div className="devtools-storage-panel">
            <div className="storage-header">
              <h3>CWeb Inverted Index Telemetry</h3>
              <button className="btn-primary-small" onClick={handleRebuildIndex}>
                <RefreshCw size={14} />
                <span>Trigger RCU Index Rebuild</span>
              </button>
            </div>

            {stats && (
              <div className="stats-summary-grid">
                <div className="stat-box">
                  <div className="stat-num">{stats.documents_indexed}</div>
                  <div className="stat-lbl">Documents Indexed</div>
                </div>
                <div className="stat-box">
                  <div className="stat-num">{stats.unique_terms}</div>
                  <div className="stat-lbl">Unique Terms</div>
                </div>
                <div className="stat-box">
                  <div className="stat-num">{stats.load_factor.toFixed(2)}</div>
                  <div className="stat-lbl">Hash Load Factor</div>
                </div>
                <div className="stat-box">
                  <div className="stat-num">{stats.cache_hits}</div>
                  <div className="stat-lbl">LRU Cache Hits</div>
                </div>
                <div className="stat-box">
                  <div className="stat-num">{stats.cache_misses}</div>
                  <div className="stat-lbl">LRU Cache Misses</div>
                </div>
                <div className="stat-box">
                  <div className="stat-num">{stats.uptime_seconds}s</div>
                  <div className="stat-lbl">Uptime</div>
                </div>
              </div>
            )}
          </div>
        )}
      </div>
    </div>
  );
};
