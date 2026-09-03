import React from 'react';
import { useStore } from '../store/useStore';
import { Search, Bookmark, Cpu, Sun, Moon } from 'lucide-react';

export const Navbar: React.FC = () => {
  const { tabs, activeTabId, updateActiveTab, theme, toggleTheme, bookmarks } = useStore();
  const activeTab = tabs.find((t) => t.id === activeTabId) || tabs[0];
  const activeScreen = activeTab.activeScreen;

  return (
    <nav className="navbar">
      <div className="navbar-brand" onClick={() => updateActiveTab({ activeScreen: 'search', url: 'cweb://search?q=' })}>
        <div className="logo-icon">C</div>
        <div className="brand-text">
          <span className="brand-name">CWeb</span>
          <span className="brand-tag">v2.0 Search Engine</span>
        </div>
      </div>

      <div className="navbar-links">
        <button
          className={`nav-btn ${activeScreen === 'search' ? 'active' : ''}`}
          onClick={() => updateActiveTab({ activeScreen: 'search', url: 'cweb://search?q=' })}
        >
          <Search size={18} />
          <span>Search</span>
        </button>

        <button
          className={`nav-btn ${activeScreen === 'bookmarks' ? 'active' : ''}`}
          onClick={() => updateActiveTab({ activeScreen: 'bookmarks', url: 'cweb://bookmarks' })}
        >
          <Bookmark size={18} />
          <span>Bookmarks</span>
          {bookmarks.length > 0 && <span className="badge">{bookmarks.length}</span>}
        </button>

        <button
          className={`nav-btn ${activeScreen === 'admin' ? 'active' : ''}`}
          onClick={() => updateActiveTab({ activeScreen: 'admin', url: 'cweb://admin' })}
        >
          <Cpu size={18} />
          <span>Admin Stats</span>
        </button>
      </div>

      <div className="navbar-actions">
        <button className="theme-toggle" onClick={toggleTheme} title="Toggle Theme">
          {theme === 'dark' ? <Sun size={20} /> : <Moon size={20} />}
        </button>
      </div>
    </nav>
  );
};
