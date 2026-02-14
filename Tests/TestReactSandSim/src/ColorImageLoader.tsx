// Color image loader component for sampling colors

import React, { useRef, useState } from 'react';

export interface ColorImageLoaderProps {
  onImageLoaded: (image: HTMLImageElement) => void;
  currentImage?: string;
}

export const ColorImageLoader: React.FC<ColorImageLoaderProps> = ({
  onImageLoaded,
  currentImage,
}) => {
  const [previewUrl, setPreviewUrl] = useState<string | null>(currentImage || null);
  const fileInputRef = useRef<HTMLInputElement>(null);

  const handleFileChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (e) => {
      const url = e.target?.result as string;
      setPreviewUrl(url);
      
      const img = new Image();
      img.onload = () => {
        onImageLoaded(img);
      };
      img.src = url;
    };
    reader.readAsDataURL(file);
  };

  const handleLoadFromUrl = () => {
    const url = prompt('Enter image URL:');
    if (!url) return;

    const img = new Image();
    img.crossOrigin = 'anonymous';
    img.onload = () => {
      setPreviewUrl(url);
      onImageLoaded(img);
    };
    img.onerror = () => {
      alert('Failed to load image from URL');
    };
    img.src = url;
  };

  const handleClearImage = () => {
    setPreviewUrl(null);
    if (fileInputRef.current) {
      fileInputRef.current.value = '';
    }
  };

  return (
    <div style={containerStyle}>
      <div style={headerStyle}>
        <h3 style={titleStyle}>Color Source Image</h3>
        <p style={descriptionStyle}>
          Upload an image to sample colors from for the sand visualization
        </p>
      </div>
      
      <div style={controlsStyle}>
        <input
          ref={fileInputRef}
          type="file"
          accept="image/*"
          onChange={handleFileChange}
          style={fileInputStyle}
          id="image-upload"
        />
        <label htmlFor="image-upload" style={buttonStyle}>
          Choose File
        </label>
        
        <button onClick={handleLoadFromUrl} style={buttonStyle}>
          Load from URL
        </button>
        
        {previewUrl && (
          <button onClick={handleClearImage} style={{ ...buttonStyle, backgroundColor: '#c44' }}>
            Clear Image
          </button>
        )}
      </div>

      {previewUrl && (
        <div style={previewContainerStyle}>
          <img
            src={previewUrl}
            alt="Color source preview"
            style={previewImageStyle}
          />
        </div>
      )}
    </div>
  );
};

const containerStyle: React.CSSProperties = {
  padding: '20px',
  backgroundColor: '#2a2a2a',
  borderRadius: '8px',
  border: '1px solid #444',
};

const headerStyle: React.CSSProperties = {
  marginBottom: '15px',
};

const titleStyle: React.CSSProperties = {
  margin: '0 0 5px 0',
  fontSize: '18px',
  color: '#e0e0e0',
};

const descriptionStyle: React.CSSProperties = {
  margin: 0,
  fontSize: '14px',
  color: '#aaa',
};

const controlsStyle: React.CSSProperties = {
  display: 'flex',
  gap: '10px',
  alignItems: 'center',
  marginBottom: '15px',
};

const fileInputStyle: React.CSSProperties = {
  display: 'none',
};

const buttonStyle: React.CSSProperties = {
  padding: '8px 16px',
  fontSize: '14px',
  cursor: 'pointer',
  backgroundColor: '#4a4a4a',
  color: 'white',
  border: 'none',
  borderRadius: '4px',
  transition: 'background-color 0.2s',
};

const previewContainerStyle: React.CSSProperties = {
  marginTop: '15px',
  border: '2px solid #555',
  borderRadius: '4px',
  overflow: 'hidden',
  backgroundColor: '#1a1a1a',
};

const previewImageStyle: React.CSSProperties = {
  width: '100%',
  maxHeight: '200px',
  objectFit: 'contain',
};
