import React, { useState, useEffect, useRef } from 'react';
import { useStore } from '../store/useStore';
import { cwebApi } from '../api/client';
import { Search, X, SlidersHorizontal } from 'lucide-react';
import { AutocompleteDropdown } from './AutocompleteDropdown';
import { RankingAlgorithm } from '../api/types';

interface SearchBarProps {
  onSearch: (q: string) => void;
  initialQuery?: string;
}

export const SearchBar: React.FC<SearchBarProps> = ({ onSearch, initialQuery = '' }) => {
  const [query, setQuery] = useState(initialQuery);
  const [suggestions, setSuggestions] = useState<string[]>([]);
  const [showDropdown, setShowDropdown] = useState(false);
  const { rankingAlgorithm, setRankingAlgorithm } = useStore();
  const dropdownRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    setQuery(initialQuery);
  }, [initialQuery]);

  useEffect(() => {
    if (query.trim().length >= 2) {
      cwebApi
        .suggest(query.trim())
        .then((res) => {
          setSuggestions(res.suggestions || []);
          setShowDropdown(true);
        })
        .catch(() => setSuggestions([]));
    } else {
      setSuggestions([]);
      setShowDropdown(false);
    }
  }, [query]);

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (query.trim()) {
      setShowDropdown(false);
      onSearch(query.trim());
    }
  };

  const handleSelectSuggestion = (term: string) => {
    setQuery(term);
    setShowDropdown(false);
    onSearch(term);
  };

  return (
    <div className="search-bar-container" ref={dropdownRef}>
      <form className="search-form" onSubmit={handleSubmit}>
        <div className="input-wrapper">
          <Search className="search-input-icon" size={20} />
          <input
            type="text"
            className="search-input"
            placeholder="Search docs (e.g. compiler optimization, title:kernel, 'b+ tree')..."
            value={query}
            onChange={(e) => setQuery(e.target.value)}
            onFocus={() => query.length >= 2 && setShowDropdown(true)}
          />
          {query && (
            <button
              type="button"
              className="clear-btn"
              onClick={() => {
                setQuery('');
                setSuggestions([]);
              }}
            >
              <X size={16} />
            </button>
          )}
        </div>

        <div className="algo-selector">
          <SlidersHorizontal size={16} className="algo-icon" />
          <select
            value={rankingAlgorithm}
            onChange={(e) => setRankingAlgorithm(e.target.value as RankingAlgorithm)}
            className="algo-dropdown"
          >
            <option value="bm25">BM25 (Field-Weighted)</option>
            <option value="tfidf">TF-IDF (Smoothed)</option>
          </select>
        </div>

        <button type="submit" className="btn-primary search-submit-btn">
          Search
        </button>
      </form>

      {showDropdown && suggestions.length > 0 && (
        <AutocompleteDropdown
          suggestions={suggestions}
          onSelect={handleSelectSuggestion}
        />
      )}
    </div>
  );
};
