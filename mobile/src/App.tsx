import React, { useEffect } from 'react';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { ChromeWindowHeader } from './components/ChromeWindowHeader';
import { ChromeOmnibox } from './components/ChromeOmnibox';
import { ChromeBookmarksBar } from './components/ChromeBookmarksBar';
import { ChromeSidePanel } from './components/ChromeSidePanel';
import { ChromeDevTools } from './components/ChromeDevTools';
import { NewTabPage } from './components/NewTabPage';
import { SearchScreen } from './screens/SearchScreen';
import { DocumentViewerScreen } from './screens/DocumentViewerScreen';
import { BookmarksScreen } from './screens/BookmarksScreen';
import { AdminStatsScreen } from './screens/AdminStatsScreen';
import { useStore } from './store/useStore';

const queryClient = new QueryClient({
  defaultOptions: {
    queries: {
      retry: 1,
      refetchOnWindowFocus: false,
      staleTime: 30000
    }
  }
});

export const AppContent: React.FC = () => {
  const {
    tabs,
    activeTabId,
    theme,
    isIncognito,
    createNewTab,
    closeTab,
    toggleDevTools,
    toggleIncognito,
    toggleBookmarksBar
  } = useStore();

  const activeTab = tabs.find((t) => t.id === activeTabId) || tabs[0];

  useEffect(() => {
    document.documentElement.setAttribute('data-theme', theme);
    document.documentElement.setAttribute('data-incognito', isIncognito ? 'true' : 'false');
  }, [theme, isIncognito]);

  /* Keyboard Shortcuts listener */
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.ctrlKey || e.metaKey) {
        if (e.key.toLowerCase() === 't') {
          e.preventDefault();
          createNewTab();
        } else if (e.key.toLowerCase() === 'w') {
          e.preventDefault();
          closeTab(activeTabId);
        } else if (e.key.toLowerCase() === 'b') {
          e.preventDefault();
          toggleBookmarksBar();
        } else if (e.shiftKey && e.key.toLowerCase() === 'n') {
          e.preventDefault();
          toggleIncognito();
        }
      } else if (e.key === 'F12') {
        e.preventDefault();
        toggleDevTools();
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [activeTabId, createNewTab, closeTab, toggleDevTools, toggleIncognito, toggleBookmarksBar]);

  return (
    <div className={`chrome-browser-app ${isIncognito ? 'incognito-theme' : ''}`}>
      {/* Chrome Top Bar */}
      <ChromeWindowHeader />

      {/* Chrome Omnibox & Navigation Bar */}
      <ChromeOmnibox />

      {/* Bookmarks Bar */}
      <ChromeBookmarksBar />

      {/* Main Browser Viewport */}
      <div className="chrome-viewport">
        <main className="chrome-content-area">
          {activeTab.activeScreen === 'ntp' && <NewTabPage />}
          {activeTab.activeScreen === 'search' && <SearchScreen />}
          {activeTab.activeScreen === 'document' && <DocumentViewerScreen />}
          {activeTab.activeScreen === 'bookmarks' && <BookmarksScreen />}
          {activeTab.activeScreen === 'admin' && <AdminStatsScreen />}
        </main>

        {/* Chrome Side Panel */}
        <ChromeSidePanel />
      </div>

      {/* Chrome DevTools Drawer */}
      <ChromeDevTools />
    </div>
  );
};

export const App: React.FC = () => {
  return (
    <QueryClientProvider client={queryClient}>
      <AppContent />
    </QueryClientProvider>
  );
};

export default App;
