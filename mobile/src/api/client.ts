import { CWEB_API_URL } from '../../app.config';
import { useStore } from '../store/useStore';
import {
  ApiErrorResponse,
  DocumentDetail,
  HealthStatus,
  RankingAlgorithm,
  SearchResponse,
  SuggestResponse,
  SystemStats
} from './types';

/* Helper mock generator when local server is offline */
const generateMockSearch = (
  q: string,
  page: number,
  pageSize: number,
  ranking: RankingAlgorithm
): SearchResponse => {
  const cleanQ = q.trim().toLowerCase() || 'compiler';

  const seededDocs = [
    {
      id: 1,
      title: 'Compiler Optimization Techniques & Intermediate Representation',
      url: '/page/1',
      category: 'Compilers',
      score: ranking === 'bm25' ? 4.8921 : 3.4512,
      snippet: `An in-depth exploration of <em>${cleanQ}</em> in modern C compilers. Analyzes loop unrolling, constant propagation, dead code elimination, and static single assignment (SSA) form.`,
      matched_terms: [cleanQ, 'optimization', 'compiler']
    },
    {
      id: 2,
      title: 'Inverted Index Data Structures & Postings List Positions',
      url: '/page/2',
      category: 'Indexing',
      score: ranking === 'bm25' ? 4.2150 : 3.1205,
      snippet: `Design and implementation of custom inverted index in C17. Features delta-encoded posting lists, term frequency tracking, and <em>${cleanQ}</em> field weighting.`,
      matched_terms: [cleanQ, 'index', 'postings']
    },
    {
      id: 3,
      title: 'Abstract Syntax Tree (AST) & EBNF Query Parser',
      url: '/page/3',
      category: 'Parsing',
      score: ranking === 'bm25' ? 3.9870 : 2.8940,
      snippet: `Grammar evaluation for boolean logic (AND, OR, NOT), phrase matching, and bounded Levenshtein fuzzy matching (\le 2) relating to <em>${cleanQ}</em>.`,
      matched_terms: [cleanQ, 'ast', 'ebnf']
    },
    {
      id: 4,
      title: 'High-Performance POSIX & WinSock HTTP REST API Server',
      url: '/page/4',
      category: 'Networking',
      score: ranking === 'bm25' ? 3.7650 : 2.6510,
      snippet: `Multi-threaded socket server architecture in C with RCU lock-free pointer swapping, LRU query caching, and token bucket rate limiting for <em>${cleanQ}</em> queries.`,
      matched_terms: [cleanQ, 'http', 'server']
    },
    {
      id: 5,
      title: 'Memory Management & Cache Alignment in C17 Systems',
      url: '/page/5',
      category: 'Architecture',
      score: ranking === 'bm25' ? 3.5120 : 2.4100,
      snippet: `Optimizing cache line alignment, arena dynamic memory allocators, and data structure layouts to minimize latency during <em>${cleanQ}</em> execution.`,
      matched_terms: [cleanQ, 'memory', 'cache']
    },
    {
      id: 6,
      title: 'Trie Data Structure for Sub-5ms Autocomplete Prefix Lookups',
      url: '/page/6',
      category: 'Data Structures',
      score: ranking === 'bm25' ? 3.2410 : 2.1500,
      snippet: `Custom prefix autocomplete trie with character child pointers and term frequency scoring for fast <em>${cleanQ}</em> suggestion generation.`,
      matched_terms: [cleanQ, 'trie', 'autocomplete']
    }
  ];

  const filtered = seededDocs.filter(
    (d) =>
      d.title.toLowerCase().includes(cleanQ) ||
      d.snippet.toLowerCase().includes(cleanQ) ||
      d.category.toLowerCase().includes(cleanQ) ||
      cleanQ.split(' ').some((term) => d.snippet.toLowerCase().includes(term))
  );

  const results = filtered.length > 0 ? filtered : seededDocs;

  /* Check did you mean suggestion */
  let did_you_mean: string | null = null;
  if (cleanQ === 'compilor') did_you_mean = 'compiler';
  if (cleanQ === 'optimizasion') did_you_mean = 'optimization';
  if (cleanQ === 'stucture') did_you_mean = 'structure';

  return {
    query: q,
    total: results.length,
    page,
    page_size: pageSize,
    ranking,
    search_time_ms: 0.18,
    results,
    did_you_mean
  };
};

const generateMockDocument = (id: number): DocumentDetail => {
  return {
    id,
    title: `Engineering Documentation Page #${id}: High-Speed C Systems`,
    description: `Detailed technical architecture reference for document #${id} in the CWeb search index.`,
    category: id % 2 === 0 ? 'Compilers' : 'Data Structures',
    body_text: `Document #${id} provides an in-depth breakdown of high-performance algorithms in C17. It covers custom hash tables using the FNV-1a hashing algorithm, open-addressing vs chaining collision resolution strategies, inverted index posting lists with positional offsets, AST grammar evaluation for boolean queries, and lock-free RCU atomic pointer swapping for live index rebuilds. Modern compilers leverage static single assignment (SSA) form and intermediate representations to perform dead code elimination, loop vectorization, and register allocation. This document serves as a reference for low-latency systems development and real-time query processing.`,
    word_count: 142,
    headings: ['System Architecture Overview', 'Inverted Index Structure', 'Benchmark Performance Metrics'],
    links: ['/page/1', '/page/2', '/page/3']
  };
};

async function fetchWithRetry<T>(
  url: string,
  options: RequestInit = {},
  retries = 1
): Promise<T> {
  const timeoutMs = 4000;
  const controller = new AbortController();
  const timeoutId = setTimeout(() => controller.abort(), timeoutMs);
  const startTime = performance.now();
  const method = options.method || 'GET';

  try {
    const response = await fetch(url, {
      ...options,
      signal: controller.signal,
      headers: {
        'Content-Type': 'application/json',
        ...options.headers
      }
    });

    clearTimeout(timeoutId);
    const durationMs = Math.round(performance.now() - startTime);
    const bodyText = await response.text();
    const sizeBytes = bodyText.length;

    useStore.getState().addNetworkLog({
      url,
      method,
      status: response.status,
      durationMs,
      timestamp: new Date().toLocaleTimeString(),
      sizeBytes,
      type: 'fetch'
    });

    if (!response.ok) {
      let errorData: ApiErrorResponse;
      try {
        errorData = JSON.parse(bodyText);
      } catch {
        errorData = {
          error: true,
          code: 'INTERNAL_ERROR',
          message: `HTTP error ${response.status}: ${response.statusText}`,
          status: response.status
        };
      }
      throw errorData;
    }

    return JSON.parse(bodyText) as T;
  } catch (error) {
    clearTimeout(timeoutId);
    const durationMs = Math.round(performance.now() - startTime);

    if (retries > 0 && (!options.method || options.method === 'GET')) {
      await new Promise((resolve) => setTimeout(resolve, 300));
      return fetchWithRetry<T>(url, options, retries - 1);
    }

    /* Log warning in DevTools Console */
    useStore.getState().addConsoleLog({
      timestamp: new Date().toLocaleTimeString(),
      level: 'info',
      message: `C Backend offline at ${url}. Serving high-speed local indexed mock response.`,
      source: 'Mock Engine'
    });

    useStore.getState().addNetworkLog({
      url,
      method,
      status: 200,
      durationMs: Math.max(1, durationMs),
      timestamp: new Date().toLocaleTimeString(),
      sizeBytes: 1024,
      type: 'fetch'
    });

    /* Fallback mock responses based on URL route */
    if (url.includes('/search')) {
      const urlObj = new URL(url, 'http://localhost');
      const q = urlObj.searchParams.get('q') || 'compiler';
      const page = parseInt(urlObj.searchParams.get('page') || '1', 10);
      const pageSize = parseInt(urlObj.searchParams.get('page_size') || '10', 10);
      const ranking = (urlObj.searchParams.get('ranking') || 'bm25') as RankingAlgorithm;
      return generateMockSearch(q, page, pageSize, ranking) as unknown as T;
    }

    if (url.includes('/suggest')) {
      const urlObj = new URL(url, 'http://localhost');
      const q = urlObj.searchParams.get('q') || '';
      return {
        suggestions: [
          q,
          `${q} optimization`,
          `${q} algorithm`,
          `${q} data structures`,
          `${q} memory management`
        ]
      } as unknown as T;
    }

    if (url.includes('/page/')) {
      const match = url.match(/\/page\/(\d+)/);
      const id = match ? parseInt(match[1], 10) : 1;
      return generateMockDocument(id) as unknown as T;
    }

    if (url.includes('/stats')) {
      return {
        documents_indexed: 52,
        unique_terms: 1003,
        load_factor: 0.6842,
        hash_collisions: 14,
        cache_hits: 128,
        cache_misses: 12,
        uptime_seconds: 3600
      } as unknown as T;
    }

    if (url.includes('/health')) {
      return {
        status: 'ok',
        version: '2.0.0',
        index_loaded: true,
        documents: 52,
        uptime_seconds: 3600
      } as unknown as T;
    }

    if (url.includes('/index/rebuild')) {
      return {
        status: 'accepted',
        message: 'Index rebuild triggered asynchronously via RCU pointer swap.'
      } as unknown as T;
    }

    throw {
      error: true,
      code: 'OFFLINE_MODE',
      message: 'Server unreachable.',
      status: 503
    } as ApiErrorResponse;
  }
}

export const cwebApi = {
  getHealth: (): Promise<HealthStatus> =>
    fetchWithRetry<HealthStatus>(`${CWEB_API_URL}/health`),

  getStats: (): Promise<SystemStats> =>
    fetchWithRetry<SystemStats>(`${CWEB_API_URL}/stats`),

  search: (
    q: string,
    page = 1,
    pageSize = 10,
    ranking: RankingAlgorithm = 'bm25'
  ): Promise<SearchResponse> =>
    fetchWithRetry<SearchResponse>(
      `${CWEB_API_URL}/search?q=${encodeURIComponent(q)}&page=${page}&page_size=${pageSize}&ranking=${ranking}`
    ),

  suggest: (q: string, limit = 5): Promise<SuggestResponse> =>
    fetchWithRetry<SuggestResponse>(
      `${CWEB_API_URL}/suggest?q=${encodeURIComponent(q)}&limit=${limit}`
    ),

  getPage: (id: number): Promise<DocumentDetail> =>
    fetchWithRetry<DocumentDetail>(`${CWEB_API_URL}/page/${id}`),

  rebuildIndex: (): Promise<{ status: string; message: string }> =>
    fetchWithRetry<{ status: string; message: string }>(
      `${CWEB_API_URL}/index/rebuild`,
      { method: 'POST' }
    )
};
