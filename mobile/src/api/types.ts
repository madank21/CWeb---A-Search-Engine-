export type RankingAlgorithm = 'bm25' | 'tfidf';

export interface SearchResultItem {
  id: number;
  title: string;
  url: string;
  category: string;
  score: number;
  snippet: string;
  matched_terms?: string[];
}

export interface SearchResponse {
  query: string;
  total: number;
  page: number;
  page_size: number;
  ranking: RankingAlgorithm;
  search_time_ms: number;
  results: SearchResultItem[];
  did_you_mean: string | null;
}

export interface SuggestResponse {
  suggestions: string[];
}

export interface DocumentDetail {
  id: number;
  title: string;
  description: string;
  category: string;
  body_text: string;
  word_count: number;
  headings?: string[];
  links?: string[];
}

export interface SystemStats {
  documents_indexed: number;
  unique_terms: number;
  load_factor: number;
  hash_collisions: number;
  cache_hits: number;
  cache_misses: number;
  uptime_seconds: number;
}

export interface HealthStatus {
  status: string;
  version: string;
  index_loaded: boolean;
  documents: number;
  uptime_seconds: number;
}

export interface ApiErrorResponse {
  error: boolean;
  code: string;
  message: string;
  status: number;
}

/* Chrome Browser Specific Types */
export type SidePanelTab = 'bookmarks' | 'history' | 'downloads' | 'reader';
export type DevToolsTab = 'network' | 'console' | 'performance' | 'storage';

export interface ChromeTab {
  id: string;
  title: string;
  url: string;
  query: string;
  activeScreen: 'ntp' | 'search' | 'document' | 'bookmarks' | 'admin' | 'compare';
  documentId?: number;
  favicon?: string;
  isLoading?: boolean;
  pinned?: boolean;
}

export interface DevToolsNetworkLog {
  id: string;
  url: string;
  method: string;
  status: number;
  durationMs: number;
  timestamp: string;
  sizeBytes: number;
  type: 'xhr' | 'fetch' | 'doc';
}

export interface DevToolsConsoleLog {
  id: string;
  timestamp: string;
  level: 'info' | 'warn' | 'error' | 'success';
  message: string;
  source: string;
}
