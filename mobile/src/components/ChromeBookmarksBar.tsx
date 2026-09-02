import React from 'react';
import { Bookmark, Folder, Search, Cpu, FileText } from 'lucide-react';
import { useStore } from '../store/useStore';

export const ChromeBookmarksBar: React.FC = () => {
  const { showBookmarksBar, bookmarks, performSearchInActiveTab, updateActiveTab, openDocumentInActiveTab } = useStore();

  if (!showBookmarksBar) return null;

  const defaultShortcuts = [
    { title: 'Compilers', query: 'compiler optimization', icon: <Search size={13} /> },
    { title: 'Data Structures', query: 'hash table trie', icon: <Search size={13} /> },
    { title: 'Memory Management', query: 'memory heap stack', icon: <Search size={13} /> },
    { title: 'System Telemetry', type: 'admin', icon: <Cpu size={13} /> }
  ];

  return (
    <div className="chrome-bookmarks-bar">
      <div className="bookmarks-items-container">
        {/* Preset Shortcuts */}
        {defaultShortcuts.map((sc, idx) => (
          <button
            key={idx}
            className="bookmark-chip"
            onClick={() => {
              if (sc.type === 'admin') {
                updateActiveTab({ activeScreen: 'admin', url: 'cweb://admin', title: 'Admin Stats' });
              } else if (sc.query) {
                performSearchInActiveTab(sc.query);
              }
            }}
          >
            {sc.icon}
            <span>{sc.title}</span>
          </button>
        ))}

        {bookmarks.length > 0 && <div className="bookmarks-divider" />}

        {/* User Bookmarks */}
        {bookmarks.slice(0, 5).map((bm) => (
          <button
            key={bm.id}
            className="bookmark-chip user-bm"
            onClick={() => openDocumentInActiveTab(bm.id, bm.title)}
            title={bm.title}
          >
            <FileText size={13} className="doc-icon" />
            <span className="bm-title">{bm.title}</span>
          </button>
        ))}
      </div>
    </div>
  );
};
