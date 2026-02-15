// Main application demonstrating the sand table simulation

import React, { useState } from 'react';
import { PatternType, THRTrack } from './types';
import { SandTableCanvas } from './SandTableCanvas';
import { PatternSelector } from './PatternSelector';
import { ColorImageLoader } from './ColorImageLoader';
import { THRFileLoader } from './THRFileLoader';

export const App: React.FC = () => {
  const [selectedPattern, setSelectedPattern] = useState<PatternType>('spiral');
  const [colorImage, setColorImage] = useState<HTMLImageElement | null>(null);
  const [showPathPreview, setShowPathPreview] = useState<boolean>(false);
  const [thrTrack, setThrTrack] = useState<THRTrack | null>(null);
  const [ballDiameter, setBallDiameter] = useState<number>(10);  // Smaller ball for finer tracks
  const [moveSpeed, setMoveSpeed] = useState<number>(15);  // Scaled for 2000×2000 grid
  const [drawingSpeed, setDrawingSpeed] = useState<number>(2);

  const handlePatternChange = (pattern: PatternType) => {
    setSelectedPattern(pattern);
  };

  const handleImageLoaded = (image: HTMLImageElement) => {
    setColorImage(image);
  };

  const handleTrackLoaded = (track: THRTrack) => {
    setThrTrack(track);
    setSelectedPattern('thr'); // Automatically switch to THR pattern
  };

  return (
    <div style={appStyle}>
      <header style={headerStyle}>
        <h1 style={titleStyle}>Sand Table Simulation</h1>
        <p style={subtitleStyle}>
          Interactive parametric pattern visualization on a virtual sand table
        </p>
      </header>

      <div style={contentStyle}>
        <div style={leftColumnStyle}>
          <SandTableCanvas
            pattern={selectedPattern}
            colorImage={colorImage}
            thrTrack={thrTrack}
            width={1000}
            height={1000}
            maxRadius={900}
            showPathPreview={showPathPreview}
            drawingSpeed={drawingSpeed}
            options={{ ballDiameter, moveSpeed }}
          />
        </div>

        <div style={rightColumnStyle}>
          <PatternSelector
            selectedPattern={selectedPattern}
            onPatternChange={handlePatternChange}
          />

          <div style={dividerStyle} />

          <div style={optionsBoxStyle}>
            <h3 style={optionsTitleStyle}>Visualization Options</h3>
            <label style={checkboxLabelStyle}>
              <input
                type="checkbox"
                checked={showPathPreview}
                onChange={(e) => setShowPathPreview(e.target.checked)}
                style={checkboxStyle}
              />
              <span>Show Path Preview</span>
            </label>
          </div>

          <div style={dividerStyle} />
          <div style={optionsBoxStyle}>
            <h3 style={optionsTitleStyle}>Physics Parameters</h3>
            
            <div style={sliderContainerStyle}>
              <label style={sliderLabelStyle}>
                <span>Ball Diameter: {ballDiameter}</span>
                <input
                  type="range"
                  min="10"
                  max="80"
                  step="2"
                  value={ballDiameter}
                  onChange={(e) => setBallDiameter(Number(e.target.value))}
                  style={sliderStyle}
                />
              </label>
            </div>
            
            <div style={sliderContainerStyle}>
              <label style={sliderLabelStyle}>
                <span>Move Speed: {moveSpeed.toFixed(2)}</span>
                <input
                  type="range"
                  min="1.0"
                  max="100.0"
                  step="1.0"
                  value={moveSpeed}
                  onChange={(e) => setMoveSpeed(Number(e.target.value))}
                  style={sliderStyle}
                />
              </label>
            </div>            
            <div style={sliderContainerStyle}>
              <label style={sliderLabelStyle}>
                <span>Drawing Speed: {drawingSpeed.toFixed(2)}</span>
                <input
                  type="range"
                  min="0.1"
                  max="5.0"
                  step="0.1"
                  value={drawingSpeed}
                  onChange={(e) => setDrawingSpeed(Number(e.target.value))}
                  style={sliderStyle}
                />
              </label>
            </div>            
            <div style={resetButtonContainerStyle}>
              <button
                onClick={() => {
                  setBallDiameter(10);
                  setMoveSpeed(15);
                  setDrawingSpeed(0.5);
                }}
                style={resetButtonStyle}
                onMouseEnter={(e) => {
                  e.currentTarget.style.backgroundColor = '#555';
                }}
                onMouseLeave={(e) => {
                  e.currentTarget.style.backgroundColor = '#444';
                }}
              >
                Reset to Defaults
              </button>
            </div>
          </div>

          <div style={dividerStyle} />
          <ColorImageLoader
            onImageLoaded={handleImageLoaded}
          />

          <div style={dividerStyle} />

          <THRFileLoader
            onTrackLoaded={handleTrackLoaded}
          />
          {thrTrack && (
            <div style={thrInfoStyle}>
              <strong>Loaded:</strong> {thrTrack.name} ({thrTrack.points.length.toLocaleString()} points)
            </div>
          )}

          <div style={infoBoxStyle}>
            <h3 style={infoTitleStyle}>About</h3>
            <p style={infoTextStyle}>
              This simulation recreates the mesmerizing patterns of a Sisyphus table,
              where a ball rolls through sand creating beautiful parametric designs.
            </p>
            <p style={infoTextStyle}>
              <strong>Pattern Types:</strong>
            </p>
            <ul style={listStyle}>
              <li style={listItemStyle}>Archimedean & Logarithmic Spirals</li>
              <li style={listItemStyle}>Rose Curves & Lissajous Figures</li>
              <li style={listItemStyle}>Epitrochoids & Hypotrochoids</li>
              <li style={listItemStyle}>Star Patterns & Expanding Circles</li>
              <li style={listItemStyle}>Sisyphus .thr Track Files</li>
            </ul>
            <p style={infoTextStyle}>
              Upload an image to sample colors from, or load a .thr track file
              to play authentic Sisyphus table patterns!
            </p>
          </div>
        </div>
      </div>

      <footer style={footerStyle}>
        <p style={footerTextStyle}>
          Sand Table Simulation | React + TypeScript + Canvas
        </p>
      </footer>
    </div>
  );
};

const appStyle: React.CSSProperties = {
  minHeight: '100vh',
  backgroundColor: '#1a1a1a',
  color: '#e0e0e0',
  fontFamily: '-apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif',
  display: 'flex',
  flexDirection: 'column',
};

const headerStyle: React.CSSProperties = {
  textAlign: 'center',
  padding: '30px 20px',
  backgroundColor: '#2a2a2a',
  borderBottom: '2px solid #444',
};

const titleStyle: React.CSSProperties = {
  margin: '0 0 10px 0',
  fontSize: '36px',
  fontWeight: 'bold',
  background: 'linear-gradient(135deg, #667eea 0%, #764ba2 100%)',
  WebkitBackgroundClip: 'text',
  WebkitTextFillColor: 'transparent',
  backgroundClip: 'text',
};

const subtitleStyle: React.CSSProperties = {
  margin: 0,
  fontSize: '16px',
  color: '#aaa',
};

const contentStyle: React.CSSProperties = {
  flex: 1,
  display: 'flex',
  gap: '30px',
  padding: '30px',
  maxWidth: '1400px',
  margin: '0 auto',
  width: '100%',
  boxSizing: 'border-box',
};

const leftColumnStyle: React.CSSProperties = {
  flex: '0 0 auto',
};

const rightColumnStyle: React.CSSProperties = {
  flex: '1',
  display: 'flex',
  flexDirection: 'column',
  gap: '20px',
};

const dividerStyle: React.CSSProperties = {
  height: '1px',
  backgroundColor: '#444',
  margin: '10px 0',
};

const optionsBoxStyle: React.CSSProperties = {
  padding: '20px',
  backgroundColor: '#2a2a2a',
  borderRadius: '8px',
  border: '1px solid #444',
};

const optionsTitleStyle: React.CSSProperties = {
  margin: '0 0 15px 0',
  fontSize: '18px',
  color: '#e0e0e0',
};

const checkboxLabelStyle: React.CSSProperties = {
  display: 'flex',
  alignItems: 'center',
  gap: '10px',
  fontSize: '14px',
  color: '#ccc',
  cursor: 'pointer',
  userSelect: 'none',
};

const checkboxStyle: React.CSSProperties = {
  width: '18px',
  height: '18px',
  cursor: 'pointer',
  accentColor: '#667eea',
};

const thrInfoStyle: React.CSSProperties = {
  marginTop: '10px',
  padding: '10px',
  backgroundColor: '#333',
  borderRadius: '4px',
  fontSize: '13px',
  color: '#aaa',
};

const infoBoxStyle: React.CSSProperties = {
  padding: '20px',
  backgroundColor: '#2a2a2a',
  borderRadius: '8px',
  border: '1px solid #444',
};

const infoTitleStyle: React.CSSProperties = {
  margin: '0 0 15px 0',
  fontSize: '18px',
  color: '#e0e0e0',
};

const infoTextStyle: React.CSSProperties = {
  margin: '0 0 12px 0',
  fontSize: '14px',
  lineHeight: '1.6',
  color: '#ccc',
};

const listStyle: React.CSSProperties = {
  margin: '8px 0 12px 20px',
  padding: 0,
};

const listItemStyle: React.CSSProperties = {
  fontSize: '14px',
  lineHeight: '1.8',
  color: '#ccc',
};

const footerStyle: React.CSSProperties = {
  textAlign: 'center',
  padding: '20px',
  backgroundColor: '#2a2a2a',
  borderTop: '2px solid #444',
};

const footerTextStyle: React.CSSProperties = {
  margin: 0,
  fontSize: '14px',
  color: '#888',
};

const sliderContainerStyle: React.CSSProperties = {
  marginBottom: '20px',
};

const sliderLabelStyle: React.CSSProperties = {
  display: 'flex',
  flexDirection: 'column',
  gap: '8px',
  fontSize: '14px',
  color: '#ccc',
};

const sliderStyle: React.CSSProperties = {
  width: '100%',
  height: '6px',
  borderRadius: '3px',
  background: '#444',
  outline: 'none',
  accentColor: '#667eea',
  cursor: 'pointer',
};

const resetButtonContainerStyle: React.CSSProperties = {
  marginTop: '15px',
};

const resetButtonStyle: React.CSSProperties = {
  padding: '8px 16px',
  fontSize: '13px',
  cursor: 'pointer',
  backgroundColor: '#444',
  color: '#ccc',
  border: '1px solid #555',
  borderRadius: '4px',
  transition: 'background-color 0.2s',
  width: '100%',
};
