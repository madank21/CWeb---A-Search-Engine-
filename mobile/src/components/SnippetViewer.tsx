import React from 'react';

interface SnippetViewerProps {
  snippetHtml: string;
}

export const SnippetViewer: React.FC<SnippetViewerProps> = ({ snippetHtml }) => {
  return (
    <div
      className="snippet-text"
      dangerouslySetInnerHTML={{ __html: snippetHtml }}
    />
  );
};
