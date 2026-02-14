// Component for loading Sisyphus .thr (track) files

import React, { useRef } from 'react';
import { THRTrack } from './types';
import { parseTHRFile } from './thrUtils';

export interface THRFileLoaderProps {
  onTrackLoaded: (track: THRTrack) => void;
}

export const THRFileLoader: React.FC<THRFileLoaderProps> = ({ onTrackLoaded }) => {
  const fileInputRef = useRef<HTMLInputElement>(null);

  const handleFileSelect = (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (e) => {
      const content = e.target?.result as string;
      if (content) {
        try {
          const track = parseTHRFile(content, file.name);
          console.log(`Loaded THR track: ${track.name} with ${track.points.length} points`);
          onTrackLoaded(track);
        } catch (error) {
          console.error('Error parsing THR file:', error);
          alert('Failed to parse THR file. Please check the file format.');
        }
      }
    };
    reader.readAsText(file);
  };

  const handleButtonClick = () => {
    fileInputRef.current?.click();
  };

  return (
    <div style={containerStyle}>
      <h3 style={titleStyle}>Load THR Track File</h3>
      <p style={descriptionStyle}>
        Load a Sisyphus .thr track file to play on the sand table
      </p>
      <input
        ref={fileInputRef}
        type="file"
        accept=".thr"
        onChange={handleFileSelect}
        style={{ display: 'none' }}
      />
      <button
        onClick={handleButtonClick}
        style={buttonStyle}
        onMouseEnter={(e) => {
          e.currentTarget.style.backgroundColor = '#5a5a5a';
        }}
        onMouseLeave={(e) => {
          e.currentTarget.style.backgroundColor = '#4a4a4a';
        }}
      >
        Choose THR File
      </button>
    </div>
  );
};

const containerStyle: React.CSSProperties = {
  padding: '20px',
  backgroundColor: '#2a2a2a',
  borderRadius: '8px',
  border: '1px solid #444',
};

const titleStyle: React.CSSProperties = {
  margin: '0 0 10px 0',
  fontSize: '18px',
  color: '#e0e0e0',
};

const descriptionStyle: React.CSSProperties = {
  margin: '0 0 15px 0',
  fontSize: '14px',
  color: '#aaa',
  lineHeight: '1.5',
};

const buttonStyle: React.CSSProperties = {
  padding: '10px 20px',
  fontSize: '14px',
  cursor: 'pointer',
  backgroundColor: '#4a4a4a',
  color: 'white',
  border: '1px solid #666',
  borderRadius: '4px',
  transition: 'background-color 0.2s',
  width: '100%',
};
