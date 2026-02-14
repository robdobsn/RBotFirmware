// Pattern selector component

import React from 'react';
import { PatternType } from './types';
import { PATTERN_OPTIONS } from './patternUtils';

export interface PatternSelectorProps {
  selectedPattern: PatternType;
  onPatternChange: (pattern: PatternType) => void;
}

export const PatternSelector: React.FC<PatternSelectorProps> = ({
  selectedPattern,
  onPatternChange,
}) => {
  return (
    <div style={containerStyle}>
      <label htmlFor="pattern-select" style={labelStyle}>
        Select Pattern:
      </label>
      <select
        id="pattern-select"
        value={selectedPattern}
        onChange={(e) => onPatternChange(e.target.value as PatternType)}
        style={selectStyle}
      >
        {PATTERN_OPTIONS.map((option) => (
          <option key={option.value} value={option.value}>
            {option.label}
          </option>
        ))}
      </select>
    </div>
  );
};

const containerStyle: React.CSSProperties = {
  display: 'flex',
  alignItems: 'center',
  gap: '10px',
  padding: '10px',
};

const labelStyle: React.CSSProperties = {
  fontSize: '16px',
  fontWeight: 'bold',
  color: '#e0e0e0',
};

const selectStyle: React.CSSProperties = {
  padding: '8px 12px',
  fontSize: '14px',
  borderRadius: '4px',
  border: '1px solid #555',
  backgroundColor: '#2a2a2a',
  color: '#e0e0e0',
  cursor: 'pointer',
  minWidth: '200px',
};
