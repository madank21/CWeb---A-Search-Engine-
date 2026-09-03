import React, { useState, useEffect } from 'react';
import { useQuery } from '@tanstack/react-query';
import { cwebApi } from '../api/client';
import { useStore } from '../store/useStore';
import { ResultCard } from '../components/ResultCard';
import { EmptyState } from '../components/EmptyState';
import { ErrorState } from '../components/ErrorState';
import { Zap, SlidersHorizontal, Columns, LayoutList } from 'lucide-react';

export const SearchScreen: React.FC = () => {
  const { tabs, activeTabId, rankingAlgorithm, activeCategoryFilter, performSearchInActiveTab } = useStore();
  const activeTab = tabs.find((t) => t.id === activeTabId) || tabs[0];
  const queryToSearch = activeTab?.query || 'compiler optimization';

  const [page, setPage] = useState(1);
  const [isDualCompareMode, setIsDualCompareMode] = useState(false);

  useEffect(() => {
    setPage(1);
  }, [queryToSearch, activeCategoryFilter, rankingAlgorithm]);

  /* Main Algorithm Query */
  const { data, isLoading, isError, error, refetch } = useQuery({
    queryKey: ['search', queryToSearch, page, rankingAlgorithm, activeCategoryFilter],
    queryFn: () => cwebApi.search(queryToSearch, page, 10, rankingAlgorithm),
    enabled: queryToSearch.length > 0
  });

  /* Dual Compare Algorithm Query (Opposite algorithm) */
  const compareAlgo = rankingAlgorithm === 'bm25' ? 'tfidf' : 'bm25';
  const { data: compareData } = useQuery({
    queryKey: ['search-compare', queryToSearch, page, compareAlgo],
    queryFn: () => cwebApi.search(queryToSearch, page, 10, compareAlgo),
    enabled: isDualCompareMode && queryToSearch.length > 0
  });

  /* Category Filter Logic */
  const filteredResults = data?.results.filter((item) =>
    activeCategoryFilter === 'all'
      ? true
      : item.category.toLowerCase().includes(activeCategoryFilter.toLowerCase())
  );

  return (
    <div className="chrome-search-screen">
      {/* Top Search Controls Bar */}
      <div className="search-header-meta">
        <div className="meta-info">
          {data && (
            <span className="results-count">
              Found <strong>{filteredResults?.length || 0}</strong> results ({data.total} total) for "<strong>{data.query}</strong>"
            </span>
          )}
        </div>

        <div className="header-actions">
          <button
            className={`view-toggle-btn ${isDualCompareMode ? 'active' : ''}`}
            onClick={() => setIsDualCompareMode(!isDualCompareMode)}
            title="Toggle Split-Screen BM25 vs TF-IDF Dual Comparison"
          >
            {isDualCompareMode ? <LayoutList size={14} /> : <Columns size={14} />}
            <span>{isDualCompareMode ? 'Single View' : 'Compare BM25 vs TF-IDF'}</span>
          </button>

          {data && (
            <div className="latency-badge">
              <Zap size={14} />
              <span>{data.search_time_ms.toFixed(2)} ms</span>
            </div>
          )}
        </div>
      </div>

      {/* Did You Mean Banner */}
      {data?.did_you_mean && (
        <div className="did-you-mean-banner">
          <span>💡 Did you mean: </span>
          <button
            className="did-you-mean-link"
            onClick={() => performSearchInActiveTab(data.did_you_mean!)}
          >
            "{data.did_you_mean}"
          </button>
        </div>
      )}

      {/* Loading & Error States */}
      {isLoading && (
        <div className="loading-state">
          <div className="spinner"></div>
          <p>Scanning C Inverted Index & Evaluating AST Query...</p>
        </div>
      )}

      {isError && <ErrorState error={error} onRetry={refetch} />}

      {/* Main Results View */}
      {data && (
        <div className={`results-view-layout ${isDualCompareMode ? 'dual-split' : 'single'}`}>
          {/* Primary Results Column */}
          <div className="results-column primary-column">
            {isDualCompareMode && (
              <div className="column-header">
                <h3>Primary Engine: {rankingAlgorithm.toUpperCase()}</h3>
                <span className="timing">{data.search_time_ms.toFixed(2)} ms</span>
              </div>
            )}

            {filteredResults && filteredResults.length === 0 ? (
              <EmptyState
                query={data.query}
                didYouMean={data.did_you_mean}
                onApplySuggestion={(sug) => performSearchInActiveTab(sug)}
              />
            ) : (
              <div className="results-list">
                {filteredResults?.map((res) => (
                  <ResultCard key={res.id} result={res} />
                ))}
              </div>
            )}
          </div>

          {/* Dual Comparison Column */}
          {isDualCompareMode && compareData && (
            <div className="results-column compare-column">
              <div className="column-header">
                <h3>Comparison Engine: {compareAlgo.toUpperCase()}</h3>
                <span className="timing">{compareData.search_time_ms.toFixed(2)} ms</span>
              </div>

              <div className="results-list">
                {compareData.results.map((res) => (
                  <ResultCard key={`comp-${res.id}`} result={res} />
                ))}
              </div>
            </div>
          )}
        </div>
      )}

      {/* Pagination */}
      {data && data.total > data.page_size && !isDualCompareMode && (
        <div className="pagination">
          <button
            disabled={page <= 1}
            onClick={() => setPage((p) => Math.max(1, p - 1))}
            className="btn-secondary"
          >
            Previous
          </button>
          <span className="page-indicator">
            Page {page} of {Math.ceil(data.total / data.page_size)}
          </span>
          <button
            disabled={page * data.page_size >= data.total}
            onClick={() => setPage((p) => p + 1)}
            className="btn-secondary"
          >
            Next
          </button>
        </div>
      )}
    </div>
  );
};
