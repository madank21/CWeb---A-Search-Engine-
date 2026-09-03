import React from 'react';
import { useStore } from '../store/useStore';
import { Bookmark, ExternalLink, Trash2 } from 'lucide-react';

export const BookmarksScreen: React.FC = () => {
  const { bookmarks, openDocumentInActiveTab, toggleBookmark } = useStore();

  return (
    <div className="screen-container">
      <div className="screen-header">
        <h1 className="screen-title">Saved Bookmarks</h1>
        <p className="screen-subtitle">Your locally saved engineering documentation pages.</p>
      </div>

      {bookmarks.length === 0 ? (
        <div className="empty-state">
          <Bookmark size={48} className="empty-icon" />
          <h3>No Bookmarks Yet</h3>
          <p>Bookmark documents during search to read them later offline.</p>
        </div>
      ) : (
        <div className="bookmarks-grid">
          {bookmarks.map((item) => (
            <div key={item.id} className="bookmark-card">
              <div className="bookmark-card-header">
                <span className="category-badge">{item.category}</span>
                <button
                  className="icon-btn-danger"
                  onClick={() => toggleBookmark({ id: item.id, title: item.title, category: item.category })}
                  title="Remove Bookmark"
                >
                  <Trash2 size={16} />
                </button>
              </div>

              <h3 className="bookmark-card-title" onClick={() => openDocumentInActiveTab(item.id, item.title)}>
                {item.title}
              </h3>

              <div className="bookmark-card-footer">
                <span className="saved-date">Saved on {item.savedAt}</span>
                <button className="btn-secondary view-link-btn" onClick={() => openDocumentInActiveTab(item.id, item.title)}>
                  <span>Read</span>
                  <ExternalLink size={14} />
                </button>
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );
};
