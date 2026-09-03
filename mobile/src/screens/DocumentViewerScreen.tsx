import React, { useState } from 'react';
import { useQuery } from '@tanstack/react-query';
import { cwebApi } from '../api/client';
import { useStore } from '../store/useStore';
import { ArrowLeft, Bookmark, BookmarkCheck, FileText, Hash, Link as LinkIcon, BookOpen, Printer, Type } from 'lucide-react';
import { ErrorState } from '../components/ErrorState';

export const DocumentViewerScreen: React.FC = () => {
  const { tabs, activeTabId, isReaderMode, toggleReaderMode, openDocumentInActiveTab, isBookmarked, toggleBookmark, updateActiveTab } = useStore();
  const activeTab = tabs.find((t) => t.id === activeTabId) || tabs[0];
  const docId = activeTab?.documentId || 1;

  const [fontSize, setFontSize] = useState<'normal' | 'large' | 'xlarge'>('normal');

  const { data: doc, isLoading, isError, error, refetch } = useQuery({
    queryKey: ['document', docId],
    queryFn: () => cwebApi.getPage(docId),
    enabled: !!docId
  });

  const bookmarked = doc ? isBookmarked(doc.id) : false;

  const handlePrint = () => {
    window.print();
  };

  return (
    <div className={`chrome-document-screen ${isReaderMode ? 'reader-mode-active' : ''} font-${fontSize}`}>
      {/* Top Controls Bar */}
      <div className="doc-toolbar">
        <button
          className="chrome-btn-ghost"
          onClick={() => updateActiveTab({ activeScreen: 'search', url: `cweb://search?q=${encodeURIComponent(activeTab.query || '')}` })}
        >
          <ArrowLeft size={16} />
          <span>Back to Search</span>
        </button>

        <div className="doc-toolbar-actions">
          {/* Font Resizer */}
          <div className="font-resizer">
            <Type size={14} />
            <button
              className={`font-size-btn ${fontSize === 'normal' ? 'active' : ''}`}
              onClick={() => setFontSize('normal')}
            >
              A
            </button>
            <button
              className={`font-size-btn ${fontSize === 'large' ? 'active' : ''}`}
              onClick={() => setFontSize('large')}
            >
              A+
            </button>
          </div>

          <button
            className={`chrome-btn-ghost ${isReaderMode ? 'active' : ''}`}
            onClick={toggleReaderMode}
            title="Toggle Distraction-Free Reader Mode"
          >
            <BookOpen size={16} />
            <span>{isReaderMode ? 'Reader View' : 'Standard View'}</span>
          </button>

          <button className="chrome-btn-ghost" onClick={handlePrint} title="Print Article">
            <Printer size={16} />
          </button>

          {doc && (
            <button
              className={`bookmark-btn ${bookmarked ? 'bookmarked' : ''}`}
              onClick={() => toggleBookmark(doc)}
            >
              {bookmarked ? <BookmarkCheck size={16} /> : <Bookmark size={16} />}
              <span>{bookmarked ? 'Saved' : 'Bookmark'}</span>
            </button>
          )}
        </div>
      </div>

      {isLoading && (
        <div className="loading-state">
          <div className="spinner"></div>
          <p>Fetching C Web Page #{docId}...</p>
        </div>
      )}

      {isError && <ErrorState error={error} onRetry={refetch} />}

      {doc && (
        <article className="document-article">
          <div className="article-header">
            <span className="category-badge">{doc.category}</span>
            <h1 className="article-title">{doc.title}</h1>
            {doc.description && <p className="article-desc">{doc.description}</p>}

            <div className="article-meta-row">
              <span><FileText size={14} /> ID: #{doc.id}</span>
              <span><Hash size={14} /> Word Count: {doc.word_count} words</span>
            </div>
          </div>

          <div className="article-body">
            <p>{doc.body_text}</p>
          </div>

          <div className="related-links-section">
            <h3><LinkIcon size={16} /> Related Engineering Pages</h3>
            <p>Explore cross-referenced documents in the index:</p>
            <div className="related-pills">
              {[1, 2, 3, 4, 5]
                .filter((id) => id !== doc.id)
                .slice(0, 3)
                .map((relId) => (
                  <button
                    key={relId}
                    className="related-pill"
                    onClick={() => openDocumentInActiveTab(relId, `Document #${relId}`)}
                  >
                    View Document #{relId}
                  </button>
                ))}
            </div>
          </div>
        </article>
      )}
    </div>
  );
};
