import React, { useState } from 'react';
import { X, Bookmark, History, Download, BookOpen, Trash2, ExternalLink, Search } from 'lucide-react';
import { useStore } from '../store/useStore';
import { SidePanelTab } from '../api/types';

export const ChromeSidePanel: React.FC = () => {
  const {
    showSidePanel,
    toggleSidePanel,
    sidePanelTab,
    bookmarks,
    history,
    toggleBookmark,
    clearHistory,
    openDocumentInActiveTab,
    performSearchInActiveTab
  } = useStore();

  const [filterText, setFilterText] = useState('');

  if (!showSidePanel) return null;

  const filteredBookmarks = bookmarks.filter((b) =>
    b.title.toLowerCase().includes(filterText.toLowerCase()) ||
    b.category.toLowerCase().includes(filterText.toLowerCase())
  );

  const filteredHistory = history.filter((h) =>
    h.title.toLowerCase().includes(filterText.toLowerCase()) ||
    h.url.toLowerCase().includes(filterText.toLowerCase())
  );

  const handleExportJSON = () => {
    const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(bookmarks, null, 2));
    const downloadAnchor = document.createElement('a');
    downloadAnchor.setAttribute("href", dataStr);
    downloadAnchor.setAttribute("download", "cweb_bookmarks.json");
    document.body.appendChild(downloadAnchor);
    downloadAnchor.click();
    downloadAnchor.remove();
  };

  return (
    <aside className="chrome-side-panel">
      {/* Side Panel Header */}
      <div className="side-panel-header">
        <div className="side-panel-tabs">
          <button
            className={`panel-tab-btn ${sidePanelTab === 'bookmarks' ? 'active' : ''}`}
            onClick={() => toggleSidePanel('bookmarks')}
            title="Bookmarks"
          >
            <Bookmark size={15} />
            <span>Bookmarks</span>
          </button>
          <button
            className={`panel-tab-btn ${sidePanelTab === 'history' ? 'active' : ''}`}
            onClick={() => toggleSidePanel('history')}
            title="History"
          >
            <History size={15} />
            <span>History</span>
          </button>
          <button
            className={`panel-tab-btn ${sidePanelTab === 'downloads' ? 'active' : ''}`}
            onClick={() => toggleSidePanel('downloads')}
            title="Exports & Downloads"
          >
            <Download size={15} />
            <span>Export</span>
          </button>
        </div>

        <button className="close-panel-btn" onClick={() => toggleSidePanel()} title="Close Side Panel">
          <X size={16} />
        </button>
      </div>

      {/* Filter Bar */}
      <div className="side-panel-filter">
        <Search size={14} className="filter-icon" />
        <input
          type="text"
          placeholder="Filter items..."
          value={filterText}
          onChange={(e) => setFilterText(e.target.value)}
        />
      </div>

      {/* Side Panel Content */}
      <div className="side-panel-body">
        {sidePanelTab === 'bookmarks' && (
          <div className="bookmarks-list">
            {filteredBookmarks.length === 0 ? (
              <div className="empty-panel-msg">No bookmarks saved yet.</div>
            ) : (
              filteredBookmarks.map((bm) => (
                <div key={bm.id} className="panel-item">
                  <div className="panel-item-info" onClick={() => openDocumentInActiveTab(bm.id, bm.title)}>
                    <div className="item-title">{bm.title}</div>
                    <div className="item-sub">
                      <span className="cat-badge">{bm.category}</span>
                      <span className="time">{bm.savedAt}</span>
                    </div>
                  </div>
                  <button
                    className="delete-item-btn"
                    onClick={() => toggleBookmark({ id: bm.id, title: bm.title, category: bm.category })}
                    title="Remove Bookmark"
                  >
                    <Trash2 size={14} />
                  </button>
                </div>
              ))
            )}
          </div>
        )}

        {sidePanelTab === 'history' && (
          <div className="history-list">
            <div className="history-actions">
              <span className="count-label">{history.length} items recorded</span>
              {history.length > 0 && (
                <button className="clear-history-btn" onClick={clearHistory}>
                  <Trash2 size={13} />
                  <span>Clear All</span>
                </button>
              )}
            </div>

            {filteredHistory.length === 0 ? (
              <div className="empty-panel-msg">No browsing history recorded.</div>
            ) : (
              filteredHistory.map((h) => (
                <div key={h.id} className="panel-item" onClick={() => {
                  if (h.query) performSearchInActiveTab(h.query);
                }}>
                  <div className="panel-item-info">
                    <div className="item-title">{h.title}</div>
                    <div className="item-sub">
                      <span className="url-badge">{h.url}</span>
                      <span className="time">{h.timestamp}</span>
                    </div>
                  </div>
                  <ExternalLink size={14} className="ext-link-icon" />
                </div>
              ))
            )}
          </div>
        )}

        {sidePanelTab === 'downloads' && (
          <div className="downloads-panel">
            <div className="export-card">
              <h4>Export Bookmarks</h4>
              <p>Export all saved bookmarks into a portable JSON document.</p>
              <button className="btn-primary-small" onClick={handleExportJSON}>
                <Download size={14} />
                <span>Export JSON</span>
              </button>
            </div>
          </div>
        )}
      </div>
    </aside>
  );
};
