import React from 'react';
import { SearchResultItem } from '../api/types';
import { SnippetViewer } from './SnippetViewer';
import { useStore } from '../store/useStore';
import { Bookmark, BookmarkCheck, ExternalLink, Hash } from 'lucide-react';

interface ResultCardProps {
  result: SearchResultItem;
}

export const ResultCard: React.FC<ResultCardProps> = ({ result }) => {
  const { openDocumentInActiveTab, toggleBookmark, isBookmarked } = useStore();
  const bookmarked = isBookmarked(result.id);

  return (
    <div className="result-card">
      <div className="result-header">
        <div className="result-meta">
          <span className="category-badge">{result.category}</span>
          <span className="score-badge">
            <Hash size={12} /> Score: {result.score.toFixed(4)}
          </span>
        </div>

        <button
          className={`bookmark-btn ${bookmarked ? 'bookmarked' : ''}`}
          onClick={() => toggleBookmark(result)}
          title={bookmarked ? 'Remove Bookmark' : 'Bookmark Page'}
        >
          {bookmarked ? <BookmarkCheck size={18} /> : <Bookmark size={18} />}
        </button>
      </div>

      <h3 className="result-title" onClick={() => openDocumentInActiveTab(result.id, result.title)}>
        {result.title}
        <ExternalLink size={14} className="link-icon" />
      </h3>

      <SnippetViewer snippetHtml={result.snippet} />

      <div className="result-footer">
        <span className="doc-url">{result.url}</span>
        <button className="view-btn" onClick={() => openDocumentInActiveTab(result.id, result.title)}>
          Read Full Document
        </button>
      </div>
    </div>
  );
};
