import React, { useState, useEffect, useRef } from 'react';
import {
  ArrowLeft,
  ArrowRight,
  RotateCw,
  Home,
  Lock,
  Star,
  Search,
  BookOpen,
  Terminal,
  Sidebar,
  Sun,
  Moon,
  EyeOff,
  SlidersHorizontal,
  X
} from 'lucide-react';
import { useStore } from '../store/useStore';
import { cwebApi } from '../api/client';

export const ChromeOmnibox: React.FC = () => {
  const {
    tabs,
    activeTabId,
    performSearchInActiveTab,
    updateActiveTab,
    isIncognito,
    toggleIncognito,
    isReaderMode,
    toggleReaderMode,
    showDevTools,
    toggleDevTools,
    showSidePanel,
    toggleSidePanel,
    theme,
    toggleTheme,
    bookmarks,
    toggleBookmark,
    rankingAlgorithm,
    setRankingAlgorithm,
    activeCategoryFilter,
    setCategoryFilter
  } = useStore();

  const activeTab = tabs.find((t) => t.id === activeTabId) || tabs[0];
  const [inputValue, setInputValue] = useState(activeTab?.url || '');
  const [suggestions, setSuggestions] = useState<string[]>([]);
  const [showDropdown, setShowDropdown] = useState(false);
  const [selectedIndex, setSelectedIndex] = useState(-1);
  const dropdownRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    setInputValue(activeTab ? activeTab.url : '');
  }, [activeTabId, activeTab?.url]);

  /* Autocomplete fetching */
  useEffect(() => {
    let cancel = false;
    const cleanQuery = inputValue.replace(/^cweb:\/\/search\?q=/, '').replace(/^cweb:\/\/[^/]+\/?/, '');
    
    if (showDropdown && cleanQuery.trim().length >= 1 && !inputValue.startsWith('cweb://page/')) {
      cwebApi.suggest(cleanQuery, 6)
        .then((res) => {
          if (!cancel && res.suggestions) {
            setSuggestions(res.suggestions);
          }
        })
        .catch(() => setSuggestions([]));
    } else {
      setSuggestions([]);
    }

    return () => {
      cancel = true;
    };
  }, [inputValue, showDropdown]);

  /* Outside click listener */
  useEffect(() => {
    const handleClickOutside = (e: MouseEvent) => {
      if (dropdownRef.current && !dropdownRef.current.contains(e.target as Node)) {
        setShowDropdown(false);
      }
    };
    document.addEventListener('mousedown', handleClickOutside);
    return () => document.removeEventListener('mousedown', handleClickOutside);
  }, []);

  const handleFocus = () => {
    setShowDropdown(true);
  };

  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setInputValue(e.target.value);
    setShowDropdown(true);
    setSelectedIndex(-1);
  };

  const executeNavigateOrSearch = (targetQuery: string) => {
    setShowDropdown(false);
    let queryToSearch = targetQuery.trim();

    if (queryToSearch.startsWith('cweb://')) {
      if (queryToSearch.includes('search?q=')) {
        const q = decodeURIComponent(queryToSearch.split('search?q=')[1]);
        performSearchInActiveTab(q);
      } else if (queryToSearch.includes('page/')) {
        const docId = parseInt(queryToSearch.split('page/')[1], 10);
        if (!isNaN(docId)) {
          updateActiveTab({
            activeScreen: 'document',
            documentId: docId,
            url: `cweb://page/${docId}`,
            title: `Document #${docId}`
          });
        }
      } else if (queryToSearch === 'cweb://bookmarks') {
        updateActiveTab({ activeScreen: 'bookmarks', url: 'cweb://bookmarks', title: 'Bookmarks' });
      } else if (queryToSearch === 'cweb://admin') {
        updateActiveTab({ activeScreen: 'admin', url: 'cweb://admin', title: 'Admin Stats' });
      } else {
        updateActiveTab({ activeScreen: 'ntp', url: 'cweb://newtab', title: 'New Tab' });
      }
    } else {
      performSearchInActiveTab(queryToSearch);
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter') {
      e.preventDefault();
      if (selectedIndex >= 0 && suggestions[selectedIndex]) {
        executeNavigateOrSearch(suggestions[selectedIndex]);
      } else {
        executeNavigateOrSearch(inputValue);
      }
    } else if (e.key === 'ArrowDown') {
      e.preventDefault();
      setSelectedIndex((prev) => Math.min(prev + 1, suggestions.length - 1));
    } else if (e.key === 'ArrowUp') {
      e.preventDefault();
      setSelectedIndex((prev) => Math.max(prev - 1, -1));
    } else if (e.key === 'Escape') {
      setShowDropdown(false);
    }
  };

  const isCurrentPageBookmarked = activeTab.documentId
    ? bookmarks.some((b) => b.id === activeTab.documentId)
    : false;

  return (
    <div className="chrome-omnibox-container" ref={dropdownRef}>
      <div className="chrome-navigation-bar">
        {/* Nav Buttons */}
        <div className="chrome-nav-buttons">
          <button className="chrome-icon-btn" title="Back">
            <ArrowLeft size={16} />
          </button>
          <button className="chrome-icon-btn" title="Forward">
            <ArrowRight size={16} />
          </button>
          <button
            className="chrome-icon-btn"
            title="Reload Page"
            onClick={() => {
              if (activeTab.query) performSearchInActiveTab(activeTab.query);
            }}
          >
            <RotateCw size={15} />
          </button>
          <button
            className="chrome-icon-btn"
            title="Home Page"
            onClick={() => updateActiveTab({ activeScreen: 'ntp', url: 'cweb://newtab', title: 'New Tab' })}
          >
            <Home size={16} />
          </button>
        </div>

        {/* Omnibox Input Wrapper */}
        <div className={`chrome-address-bar ${showDropdown ? 'focused' : ''}`}>
          <div className="security-badge" title="Secure CWeb REST Connection">
            <Lock size={14} className="lock-icon" />
          </div>

          <input
            type="text"
            className="omnibox-input"
            value={inputValue}
            onChange={handleInputChange}
            onFocus={handleFocus}
            onKeyDown={handleKeyDown}
            placeholder="Search terms, boolean query, or type cweb:// URL..."
          />

          {inputValue && (
            <button
              className="clear-input-btn"
              onClick={() => {
                setInputValue('');
                setShowDropdown(true);
              }}
              title="Clear input"
            >
              <X size={14} />
            </button>
          )}

          {activeTab.documentId && (
            <button
              className={`bookmark-star-btn ${isCurrentPageBookmarked ? 'active' : ''}`}
              onClick={() =>
                toggleBookmark({
                  id: activeTab.documentId!,
                  title: activeTab.title,
                  category: 'Web Page'
                })
              }
              title={isCurrentPageBookmarked ? 'Remove Bookmark' : 'Bookmark this Page'}
            >
              <Star size={16} fill={isCurrentPageBookmarked ? '#f59e0b' : 'none'} />
            </button>
          )}
        </div>

        {/* Extension Action Toolbar */}
        <div className="chrome-actions-toolbar">
          <button
            className={`chrome-action-btn ${isIncognito ? 'active' : ''}`}
            onClick={toggleIncognito}
            title={isIncognito ? 'Exit Incognito Mode' : 'Toggle Incognito Stealth Mode'}
          >
            <EyeOff size={16} />
          </button>

          <button
            className={`chrome-action-btn ${isReaderMode ? 'active' : ''}`}
            onClick={toggleReaderMode}
            title="Toggle Reader Mode"
          >
            <BookOpen size={16} />
          </button>

          <button
            className={`chrome-action-btn ${showDevTools ? 'active' : ''}`}
            onClick={toggleDevTools}
            title="Toggle DevTools (F12)"
          >
            <Terminal size={16} />
          </button>

          <button
            className={`chrome-action-btn ${showSidePanel ? 'active' : ''}`}
            onClick={() => toggleSidePanel()}
            title="Chrome Side Panel"
          >
            <Sidebar size={16} />
          </button>

          <button className="chrome-action-btn" onClick={toggleTheme} title="Toggle Dark/Light Mode">
            {theme === 'dark' ? <Sun size={16} /> : <Moon size={16} />}
          </button>
        </div>
      </div>

      {/* Omnibox Autocomplete Dropdown */}
      {showDropdown && suggestions.length > 0 && (
        <div className="omnibox-dropdown">
          <div className="dropdown-header">Prefix Autocomplete Suggestions</div>
          {suggestions.map((sug, idx) => (
            <div
              key={idx}
              className={`dropdown-item ${idx === selectedIndex ? 'selected' : ''}`}
              onClick={() => executeNavigateOrSearch(sug)}
            >
              <Search size={14} className="sug-icon" />
              <span>{sug}</span>
            </div>
          ))}
        </div>
      )}

      {/* Chrome Filter & Engine Bar */}
      <div className="chrome-filter-bar">
        <div className="filter-chips">
          {['all', 'Compilers', 'Algorithms', 'Architecture', 'OS', 'Networking'].map((cat) => (
            <button
              key={cat}
              className={`chip ${activeCategoryFilter === cat ? 'active' : ''}`}
              onClick={() => setCategoryFilter(cat)}
            >
              {cat === 'all' ? 'All Results' : cat}
            </button>
          ))}
        </div>

        <div className="ranking-engine-selector">
          <SlidersHorizontal size={13} />
          <span className="label">Algorithm:</span>
          <button
            className={`algo-btn ${rankingAlgorithm === 'bm25' ? 'active' : ''}`}
            onClick={() => setRankingAlgorithm('bm25')}
            title="Best Matching 25 (k1=1.2, b=0.75)"
          >
            BM25 Engine
          </button>
          <button
            className={`algo-btn ${rankingAlgorithm === 'tfidf' ? 'active' : ''}`}
            onClick={() => setRankingAlgorithm('tfidf')}
            title="Term Frequency - Inverse Document Frequency"
          >
            TF-IDF Engine
          </button>
        </div>
      </div>
    </div>
  );
};
