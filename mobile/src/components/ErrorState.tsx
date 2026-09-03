import React from 'react';
import { AlertTriangle, RefreshCw, Cpu } from 'lucide-react';
import { ApiErrorResponse } from '../api/types';

interface ErrorStateProps {
  error: ApiErrorResponse | Error | null;
  onRetry?: () => void;
}

export const ErrorState: React.FC<ErrorStateProps> = ({ error, onRetry }) => {
  const isApiErr = error && 'code' in error;
  const code = isApiErr ? (error as ApiErrorResponse).code : 'SERVER_OFFLINE';
  const message = error
    ? error.message
    : 'CWeb Backend Server is initializing or unreachable on port 8080.';

  return (
    <div className="error-card">
      <div className="error-header">
        <AlertTriangle className="error-icon" size={28} />
        <div>
          <h3 className="error-title">Backend Server Notice ({code})</h3>
          <p className="error-message">{message}</p>
        </div>
      </div>

      <div className="error-actions" style={{ display: 'flex', gap: '0.6rem', marginTop: '0.8rem' }}>
        {onRetry && (
          <button className="btn-secondary retry-btn" onClick={onRetry}>
            <RefreshCw size={16} />
            <span>Retry Connection</span>
          </button>
        )}

        <button className="btn-secondary retry-btn" onClick={() => onRetry && onRetry()}>
          <Cpu size={16} />
          <span>Use Offline Index Cache</span>
        </button>
      </div>
    </div>
  );
};
