import React, { useState } from 'react';
import { Search, Code2, Cpu, Database, Server, Terminal, Sparkles, Zap, Compass } from 'lucide-react';
import { useStore } from '../store/useStore';
import { cwebApi } from '../api/client';

export const NewTabPage: React.FC = () => {
  const { performSearchInActiveTab, updateActiveTab } = useStore();
  const [searchInput, setSearchInput] = useState('');
  const [suggestions, setSuggestions] = useState<string[]>([]);
  const [showDropdown, setShowDropdown] = useState(false);

  const topShortcuts = [
    { title: 'Compiler Optimization', query: 'compiler optimization', icon: <Code2 size={22} />, color: '#3b82f6' },
    { title: 'Data Structures', query: 'inverted index postings', icon: <Database size={22} />, color: '#8b5cf6' },
    { title: 'System Architecture', query: 'posix winsock socket', icon: <Cpu size={22} />, color: '#10b981' },
    { title: 'AST Parsing Grammar', query: 'ebnf ast query', icon: <Terminal size={22} />, color: '#f59e0b' },
    { title: 'BM25 vs TF-IDF', query: 'ranking bm25 tfidf', icon: <Zap size={22} />, color: '#ec4899' },
    { title: 'Admin Metrics', screen: 'admin', icon: <Server size={22} />, color: '#06b6d4' }
  ];

  const handleSearchSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (searchInput.trim()) {
      performSearchInActiveTab(searchInput.trim());
    }
  };

  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const val = e.target.value;
    setSearchInput(val);
    if (val.trim()) {
      setShowDropdown(true);
      cwebApi.suggest(val.trim(), 5)
        .then((res) => setSuggestions(res.suggestions || []))
        .catch(() => setSuggestions([]));
    } else {
      setShowDropdown(false);
      setSuggestions([]);
    }
  };

  return (
    <div className="new-tab-page">
      {/* Brand Hero */}
      <div className="ntp-brand">
        <div className="ntp-logo">
          <span className="char c-blue">C</span>
          <span className="char c-red">W</span>
          <span className="char c-yellow">e</span>
          <span className="char c-green">b</span>
        </div>
        <div className="ntp-tagline">
          High-Performance C17 Search Engine & Custom Inverted Index
        </div>
      </div>

      {/* Main Search Input */}
      <div className="ntp-search-container">
        <form onSubmit={handleSearchSubmit} className="ntp-search-box">
          <Search size={20} className="ntp-search-icon" />
          <input
            type="text"
            className="ntp-input"
            placeholder="Search documents, phrases or boolean logic..."
            value={searchInput}
            onChange={handleInputChange}
            onFocus={() => searchInput.trim() && setShowDropdown(true)}
            autoFocus
          />
          <button type="submit" className="ntp-submit-btn">
            <span>Search</span>
          </button>
        </form>

        {showDropdown && suggestions.length > 0 && (
          <div className="ntp-dropdown">
            {suggestions.map((sug, idx) => (
              <div
                key={idx}
                className="ntp-dropdown-item"
                onClick={() => performSearchInActiveTab(sug)}
              >
                <Search size={14} />
                <span>{sug}</span>
              </div>
            ))}
          </div>
        )}
      </div>

      {/* Top Visit Shortcuts */}
      <div className="ntp-shortcuts-grid">
        {topShortcuts.map((sc, idx) => (
          <button
            key={idx}
            className="ntp-shortcut-card"
            onClick={() => {
              if (sc.screen === 'admin') {
                updateActiveTab({ activeScreen: 'admin', url: 'cweb://admin', title: 'Admin Stats' });
              } else if (sc.query) {
                performSearchInActiveTab(sc.query);
              }
            }}
          >
            <div className="shortcut-icon" style={{ backgroundColor: `${sc.color}20`, color: sc.color }}>
              {sc.icon}
            </div>
            <span className="shortcut-title">{sc.title}</span>
          </button>
        ))}
      </div>

      {/* Footer Info Badge */}
      <div className="ntp-footer-badge">
        <Sparkles size={14} />
        <span>Self-Hosted C17 Engine • BM25 Ranking • AST Query Grammar • Dual WinSock / POSIX Server</span>
      </div>
    </div>
  );
};
