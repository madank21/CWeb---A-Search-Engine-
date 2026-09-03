import { create } from 'zustand';
import { ChromeTab, DevToolsConsoleLog, DevToolsNetworkLog, DevToolsTab, RankingAlgorithm, SidePanelTab } from '../api/types';

export interface BookmarkItem {
  id: number;
  title: string;
  category: string;
  savedAt: string;
  url: string;
}

export interface HistoryItem {
  id: string;
  title: string;
  url: string;
  timestamp: string;
  query: string;
}

interface AppState {
  /* Theme & Mode */
  theme: 'dark' | 'light';
  isIncognito: boolean;
  isReaderMode: boolean;
  showBookmarksBar: boolean;
  showDevTools: boolean;
  showSidePanel: boolean;
  sidePanelTab: SidePanelTab;
  devToolsTab: DevToolsTab;

  /* Search Engine Config */
  rankingAlgorithm: RankingAlgorithm;
  activeCategoryFilter: string;

  /* Chrome Tabs Management */
  tabs: ChromeTab[];
  activeTabId: string;

  /* DevTools Logs */
  networkLogs: DevToolsNetworkLog[];
  consoleLogs: DevToolsConsoleLog[];

  /* History & Bookmarks */
  bookmarks: BookmarkItem[];
  history: HistoryItem[];

  /* Actions */
  toggleTheme: () => void;
  toggleIncognito: () => void;
  toggleReaderMode: () => void;
  toggleBookmarksBar: () => void;
  toggleDevTools: () => void;
  toggleSidePanel: (tab?: SidePanelTab) => void;
  setDevToolsTab: (tab: DevToolsTab) => void;
  setRankingAlgorithm: (algo: RankingAlgorithm) => void;
  setCategoryFilter: (category: string) => void;

  /* Tab Operations */
  createNewTab: (query?: string) => void;
  closeTab: (tabId: string) => void;
  switchTab: (tabId: string) => void;
  updateActiveTab: (updates: Partial<ChromeTab>) => void;
  openDocumentInActiveTab: (id: number, title?: string) => void;
  performSearchInActiveTab: (query: string) => void;

  /* DevTools Logger */
  addNetworkLog: (log: Omit<DevToolsNetworkLog, 'id'>) => void;
  addConsoleLog: (log: Omit<DevToolsConsoleLog, 'id'>) => void;

  /* History & Bookmarks */
  toggleBookmark: (doc: { id: number; title: string; category: string; url?: string }) => void;
  isBookmarked: (id: number) => boolean;
  addToHistory: (title: string, url: string, query: string) => void;
  clearHistory: () => void;
}

const defaultTabs: ChromeTab[] = [
  {
    id: 'tab-1',
    title: 'CWeb Search Engine',
    url: 'cweb://search?q=compiler+optimization',
    query: 'compiler optimization',
    activeScreen: 'search'
  }
];

export const useStore = create<AppState>((set, get) => ({
  theme: 'dark',
  isIncognito: false,
  isReaderMode: false,
  showBookmarksBar: true,
  showDevTools: false,
  showSidePanel: false,
  sidePanelTab: 'bookmarks',
  devToolsTab: 'network',

  rankingAlgorithm: 'bm25',
  activeCategoryFilter: 'all',

  tabs: defaultTabs,
  activeTabId: 'tab-1',

  networkLogs: [],
  consoleLogs: [
    {
      id: 'log-1',
      timestamp: new Date().toLocaleTimeString(),
      level: 'info',
      message: 'CWeb Search Engine v2.0 POSIX/Winsock client initialized.',
      source: 'System'
    }
  ],

  bookmarks: [
    {
      id: 1,
      title: 'Compiler Optimization Techniques',
      category: 'Compilers',
      savedAt: new Date().toLocaleDateString(),
      url: 'cweb://page/1'
    },
    {
      id: 3,
      title: 'Inverted Index Data Structures & Postings',
      category: 'Indexing',
      savedAt: new Date().toLocaleDateString(),
      url: 'cweb://page/3'
    }
  ],

  history: [
    {
      id: 'hist-1',
      title: 'compiler optimization - CWeb Search',
      url: 'cweb://search?q=compiler+optimization',
      timestamp: new Date().toLocaleTimeString(),
      query: 'compiler optimization'
    }
  ],

  toggleTheme: () => set((state) => ({ theme: state.theme === 'dark' ? 'light' : 'dark' })),
  toggleIncognito: () => set((state) => ({ isIncognito: !state.isIncognito })),
  toggleReaderMode: () => set((state) => ({ isReaderMode: !state.isReaderMode })),
  toggleBookmarksBar: () => set((state) => ({ showBookmarksBar: !state.showBookmarksBar })),
  toggleDevTools: () => set((state) => ({ showDevTools: !state.showDevTools })),
  toggleSidePanel: (tab) => set((state) => ({
    showSidePanel: tab ? true : !state.showSidePanel,
    sidePanelTab: tab || state.sidePanelTab
  })),
  setDevToolsTab: (tab) => set({ devToolsTab: tab }),

  setRankingAlgorithm: (algo) => set({ rankingAlgorithm: algo }),
  setCategoryFilter: (category) => set({ activeCategoryFilter: category }),

  /* Tab Operations */
  createNewTab: (query = '') => {
    const newId = `tab-${Date.now()}`;
    const newTab: ChromeTab = {
      id: newId,
      title: query ? `${query} - Search` : 'New Tab',
      url: query ? `cweb://search?q=${encodeURIComponent(query)}` : 'cweb://newtab',
      query: query,
      activeScreen: query ? 'search' : 'ntp'
    };
    set((state) => ({
      tabs: [...state.tabs, newTab],
      activeTabId: newId
    }));
  },

  closeTab: (tabId) => {
    const { tabs, activeTabId } = get();
    if (tabs.length === 1) {
      // Don't close last tab, just reset it to NTP
      set((state) => ({
        tabs: state.tabs.map((t) =>
          t.id === tabId
            ? { ...t, title: 'New Tab', url: 'cweb://newtab', query: '', activeScreen: 'ntp' }
            : t
        )
      }));
      return;
    }

    const filtered = tabs.filter((t) => t.id !== tabId);
    let nextActive = activeTabId;
    if (activeTabId === tabId) {
      const idx = tabs.findIndex((t) => t.id === tabId);
      nextActive = filtered[Math.max(0, idx - 1)].id;
    }

    set({ tabs: filtered, activeTabId: nextActive });
  },

  switchTab: (tabId) => set({ activeTabId: tabId }),

  updateActiveTab: (updates) => {
    const { activeTabId } = get();
    set((state) => ({
      tabs: state.tabs.map((t) => (t.id === activeTabId ? { ...t, ...updates } : t))
    }));
  },

  openDocumentInActiveTab: (id, title = 'Document Viewer') => {
    const { activeTabId, addToHistory } = get();
    const docUrl = `cweb://page/${id}`;
    set((state) => ({
      tabs: state.tabs.map((t) =>
        t.id === activeTabId
          ? {
              ...t,
              title,
              url: docUrl,
              activeScreen: 'document',
              documentId: id
            }
          : t
      )
    }));
    addToHistory(title, docUrl, '');
  },

  performSearchInActiveTab: (query) => {
    const { activeTabId, addToHistory } = get();
    const searchUrl = `cweb://search?q=${encodeURIComponent(query)}`;
    set((state) => ({
      tabs: state.tabs.map((t) =>
        t.id === activeTabId
          ? {
              ...t,
              title: `${query} - Search`,
              url: searchUrl,
              query,
              activeScreen: 'search'
            }
          : t
      )
    }));
    addToHistory(`${query} - Search`, searchUrl, query);
  },

  /* DevTools Logger */
  addNetworkLog: (log) => {
    const id = `net-${Date.now()}-${Math.random().toString(36).substring(2, 6)}`;
    set((state) => ({
      networkLogs: [{ ...log, id }, ...state.networkLogs].slice(0, 100)
    }));
  },

  addConsoleLog: (log) => {
    const id = `con-${Date.now()}-${Math.random().toString(36).substring(2, 6)}`;
    set((state) => ({
      consoleLogs: [{ ...log, id }, ...state.consoleLogs].slice(0, 100)
    }));
  },

  /* History & Bookmarks */
  toggleBookmark: (doc) => {
    set((state) => {
      const exists = state.bookmarks.some((b) => b.id === doc.id);
      if (exists) {
        return { bookmarks: state.bookmarks.filter((b) => b.id !== doc.id) };
      }
      return {
        bookmarks: [
          ...state.bookmarks,
          {
            id: doc.id,
            title: doc.title,
            category: doc.category,
            savedAt: new Date().toLocaleDateString(),
            url: doc.url || `cweb://page/${doc.id}`
          }
        ]
      };
    });
  },

  isBookmarked: (id) => get().bookmarks.some((b) => b.id === id),

  addToHistory: (title, url, query) => {
    const { isIncognito } = get();
    if (isIncognito) return; // Do not record history in Incognito Mode

    const newItem: HistoryItem = {
      id: `hist-${Date.now()}`,
      title,
      url,
      timestamp: new Date().toLocaleTimeString(),
      query
    };

    set((state) => ({
      history: [newItem, ...state.history].slice(0, 100)
    }));
  },

  clearHistory: () => set({ history: [] })
}));
