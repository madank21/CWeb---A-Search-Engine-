import React, { useState } from 'react';
import { useQuery } from '@tanstack/react-query';
import { cwebApi } from '../api/client';
import { Activity, Database, HardDrive, Hash, RefreshCw, Server, ShieldCheck, Zap } from 'lucide-react';
import { ErrorState } from '../components/ErrorState';

export const AdminStatsScreen: React.FC = () => {
  const [rebuildMsg, setRebuildMsg] = useState<string | null>(null);
  const [isRebuilding, setIsRebuilding] = useState(false);

  const { data: stats, isLoading: statsLoading, isError: statsError, error: statsErrObj, refetch: refetchStats } = useQuery({
    queryKey: ['systemStats'],
    queryFn: cwebApi.getStats,
    refetchInterval: 5000
  });

  const { data: health, isLoading: healthLoading } = useQuery({
    queryKey: ['systemHealth'],
    queryFn: cwebApi.getHealth,
    refetchInterval: 5000
  });

  const handleRebuild = async () => {
    setIsRebuilding(true);
    setRebuildMsg(null);
    try {
      const res = await cwebApi.rebuildIndex();
      setRebuildMsg(res.message);
      refetchStats();
    } catch (err: any) {
      setRebuildMsg(err.message || 'Rebuild failed or concurrent build in progress (409).');
    } finally {
      setIsRebuilding(false);
    }
  };

  return (
    <div className="screen-container admin-screen">
      <div className="screen-header">
        <div className="admin-header-row">
          <div>
            <h1 className="screen-title">Backend Metrics & System Health</h1>
            <p className="screen-subtitle">Live telemetry from the C search engine kernel.</p>
          </div>

          <button
            className="btn-primary rebuild-btn"
            onClick={handleRebuild}
            disabled={isRebuilding}
          >
            <RefreshCw size={16} className={isRebuilding ? 'spin' : ''} />
            <span>{isRebuilding ? 'Rebuilding Index...' : 'Trigger Index Rebuild'}</span>
          </button>
        </div>

        {rebuildMsg && <div className="rebuild-banner">{rebuildMsg}</div>}
      </div>

      {(statsLoading || healthLoading) && (
        <div className="loading-state">
          <div className="spinner"></div>
          <p>Fetching backend stats from :8080/api/v1/stats...</p>
        </div>
      )}

      {statsError && <ErrorState error={statsErrObj} onRetry={refetchStats} />}

      {stats && health && (
        <div className="stats-grid">
          <div className="stat-card">
            <div className="stat-card-header">
              <Database className="stat-icon icon-blue" size={24} />
              <span className="stat-title">Indexed Corpus</span>
            </div>
            <div className="stat-value">{stats.documents_indexed}</div>
            <div className="stat-desc">HTML documents parsed and resident</div>
          </div>

          <div className="stat-card">
            <div className="stat-card-header">
              <Zap className="stat-icon icon-amber" size={24} />
              <span className="stat-title">Unique Vocabulary</span>
            </div>
            <div className="stat-value">{stats.unique_terms.toLocaleString()}</div>
            <div className="stat-desc">Unique terms in Trie and Inverted Index</div>
          </div>

          <div className="stat-card">
            <div className="stat-card-header">
              <Hash className="stat-icon icon-emerald" size={24} />
              <span className="stat-title">Hash Table Load Factor</span>
            </div>
            <div className="stat-value">{stats.load_factor.toFixed(4)}</div>
            <div className="stat-desc">Collisions: {stats.hash_collisions} (FNV-1a chaining)</div>
          </div>

          <div className="stat-card">
            <div className="stat-card-header">
              <Activity className="stat-icon icon-purple" size={24} />
              <span className="stat-title">LRU Query Cache</span>
            </div>
            <div className="stat-value">
              {stats.cache_hits + stats.cache_misses > 0
                ? `${((stats.cache_hits / (stats.cache_hits + stats.cache_misses)) * 100).toFixed(1)}%`
                : '100%'}
            </div>
            <div className="stat-desc">Hits: {stats.cache_hits} | Misses: {stats.cache_misses}</div>
          </div>

          <div className="stat-card">
            <div className="stat-card-header">
              <Server className="stat-icon icon-indigo" size={24} />
              <span className="stat-title">Server Uptime</span>
            </div>
            <div className="stat-value">{stats.uptime_seconds}s</div>
            <div className="stat-desc">Backend version: {health.version}</div>
          </div>

          <div className="stat-card">
            <div className="stat-card-header">
              <ShieldCheck className="stat-icon icon-cyan" size={24} />
              <span className="stat-title">RCU Index Pointer</span>
            </div>
            <div className="stat-value">{health.index_loaded ? 'ACTIVE' : 'IDLE'}</div>
            <div className="stat-desc">Lock-free concurrent reading enabled</div>
          </div>
        </div>
      )}
    </div>
  );
};
