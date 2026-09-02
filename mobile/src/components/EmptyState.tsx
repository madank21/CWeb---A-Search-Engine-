import React from 'react';
import { SearchX } from 'lucide-react';

interface EmptyStateProps {
  title?: string;
  message?: string;
  query?: string;
  didYouMean?: string | null;
  onApplySuggestion?: (term: string) => void;
}

export const EmptyState: React.FC<EmptyStateProps> = ({
  title = 'No Results Found',
  message = 'No indexed documents matched your query criteria.',
  query,
  didYouMean,
  onApplySuggestion
}) => {
  return (
    <div className="empty-state">
      <SearchX className="empty-icon" size={48} />
      <h3 className="empty-title">{title}</h3>
      <p className="empty-message">
        {query ? (
          <>
            No documents found for <span className="highlight-term">"{query}"</span>.
          </>
        ) : (
          message
        )}
      </p>

      {didYouMean && onApplySuggestion && (
        <div className="did-you-mean-box">
          <span>Did you mean: </span>
          <button
            className="did-you-mean-btn"
            onClick={() => onApplySuggestion(didYouMean)}
          >
            "{didYouMean}"
          </button>
        </div>
      )}
    </div>
  );
};
