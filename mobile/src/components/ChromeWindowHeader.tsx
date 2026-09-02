import React from 'react';
import { Plus, X, Globe, EyeOff, Search, Cpu, Bookmark, FileText } from 'lucide-react';
import { useStore } from '../store/useStore';

export const ChromeWindowHeader: React.FC = () => {
  const { tabs, activeTabId, switchTab, closeTab, createNewTab, isIncognito } = useStore();

  const getTabIcon = (screen: string) => {
    switch (screen) {
      case 'ntp':
        return <Globe size={13} className="tab-icon-svg" />;
      case 'search':
        return <Search size={13} className="tab-icon-svg" />;
      case 'document':
        return <FileText size={13} className="tab-icon-svg" />;
      case 'bookmarks':
        return <Bookmark size={13} className="tab-icon-svg" />;
      case 'admin':
        return <Cpu size={13} className="tab-icon-svg" />;
      default:
        return <Globe size={13} className="tab-icon-svg" />;
    }
  };

  return (
    <header className={`chrome-window-header ${isIncognito ? 'incognito-mode' : ''}`}>
      {/* Window Controls */}
      <div className="chrome-controls-group">
        <span className="dot dot-close" title="Close Window" />
        <span className="dot dot-minimize" title="Minimize Window" />
        <span className="dot dot-maximize" title="Maximize Window" />
      </div>

      {/* Tabs Strip */}
      <div className="chrome-tab-strip">
        {tabs.map((tab) => {
          const isActive = tab.id === activeTabId;
          return (
            <div
              key={tab.id}
              className={`chrome-tab ${isActive ? 'active' : ''}`}
              onClick={() => switchTab(tab.id)}
            >
              <div className="chrome-tab-content">
                {getTabIcon(tab.activeScreen)}
                <span className="chrome-tab-title">{tab.title}</span>
              </div>
              <button
                className="chrome-tab-close"
                onClick={(e) => {
                  e.stopPropagation();
                  closeTab(tab.id);
                }}
                title="Close Tab (Ctrl+W)"
              >
                <X size={12} />
              </button>
            </div>
          );
        })}

        <button
          className="chrome-add-tab-btn"
          onClick={() => createNewTab()}
          title="New Tab (Ctrl+T)"
        >
          <Plus size={15} />
        </button>
      </div>

      {/* Incognito Indicator Badge */}
      {isIncognito && (
        <div className="incognito-badge" title="Incognito Mode Active - No Search History Saved">
          <EyeOff size={14} />
          <span>Incognito</span>
        </div>
      )}
    </header>
  );
};
