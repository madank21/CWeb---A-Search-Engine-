import React from 'react';

interface AutocompleteDropdownProps {
  suggestions: string[];
  onSelect: (term: string) => void;
}

export const AutocompleteDropdown: React.FC<AutocompleteDropdownProps> = ({
  suggestions,
  onSelect
}) => {
  if (!suggestions || suggestions.length === 0) return null;

  return (
    <ul className="autocomplete-dropdown">
      {suggestions.map((item, idx) => (
        <li key={idx} className="autocomplete-item" onClick={() => onSelect(item)}>
          <span className="suggest-prefix">💡</span> {item}
        </li>
      ))}
    </ul>
  );
};
