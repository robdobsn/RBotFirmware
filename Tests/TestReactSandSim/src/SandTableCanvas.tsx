// Main sand table canvas component

import React, { useRef, useEffect, useState } from 'react';
import { PatternType, SandSimulationOptions, RGB, THRTrack, Point } from './types';
import { getPatternPoint } from './patternUtils';
import { SandTableSim } from './sandSimulation';
import { SandTableSimWasm, initializeWasm } from './sandSimulationWasm';
import { CrossSectionChart, CrossSectionData } from './CrossSectionChart';

const DEFAULT_COLOR_PALETTE: RGB[] = [
  { r: 55, g: 52, b: 43, a: 1 }, // 0
  { r: 90, g: 84, b: 79, a: 1 }, // 1
  { r: 95, g: 93, b: 84, a: 1 }, // 2
  { r: 102, g: 96, b: 91, a: 1 }, // 3
  { r: 103, g: 100, b: 93, a: 1 }, // 4
  { r: 108, g: 104, b: 101, a: 1 }, // 5
  { r: 108, g: 104, b: 101, a: 1 }, // 6
  { r: 108, g: 104, b: 101, a: 1 }, // 7
  { r: 112, g: 106, b: 101, a: 1 }, // 8
  { r: 117, g: 112, b: 103, a: 1 }, // 9
  { r: 124, g: 119, b: 114, a: 1 }, // 10
  { r: 125, g: 122, b: 114, a: 1 }, // 11
  { r: 133, g: 127, b: 122, a: 1 }, // 12
  { r: 139, g: 136, b: 128, a: 1 }, // 13
  { r: 140, g: 135, b: 131, a: 1 }, // 14
  { r: 150, g: 147, b: 136, a: 1 }, // 15
  { r: 161, g: 160, b: 151, a: 1 }, // 16
  { r: 164, g: 160, b: 156, a: 1 }, // 17
  { r: 173, g: 170, b: 157, a: 1 }, // 18
  { r: 175, g: 170, b: 165, a: 1 }, // 19
  { r: 179, g: 176, b: 168, a: 1 }, // 20
  { r: 182, g: 179, b: 169, a: 1 }, // 21
  { r: 191, g: 188, b: 180, a: 1 }, // 22
  { r: 192, g: 188, b: 183, a: 1 }, // 23
  { r: 203, g: 200, b: 192, a: 1 }, // 24
  { r: 209, g: 206, b: 198, a: 1 }, // 25
  { r: 211, g: 208, b: 197, a: 1 }, // 26
  { r: 220, g: 217, b: 207, a: 1 }, // 27
  { r: 229, g: 225, b: 220, a: 1 }, // 28
  { r: 245, g: 242, b: 230, a: 1 }, // 29
];

export interface SandTableCanvasProps {
  pattern: PatternType;
  options?: Partial<SandSimulationOptions>;
  colorImage?: HTMLImageElement | null;
  thrTrack?: THRTrack | null;
  width?: number;
  height?: number;
  maxRadius?: number;
  showPathPreview?: boolean;
  drawingSpeed?: number;
  onReset?: () => void;
}

const DEFAULT_OPTIONS: SandSimulationOptions = {
  ballDiameter: 30,      // Scaled proportionally (6 * 5)
  tableSize: 1000,      // High resolution for smooth detail (166 parallel traces)
  sandStartLevel: 5,
  maxSandLevel: 20,
  moveSpeed: 7.5,       // Scaled proportionally (1.5 * 5)
};

export const SandTableCanvas: React.FC<SandTableCanvasProps> = ({
  pattern,
  options = {},
  colorImage = null,
  thrTrack = null,
  width = 800,
  height = 800,
  maxRadius = 250,
  showPathPreview = false,
  drawingSpeed = 2,
  onReset,
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [simulation, setSimulation] = useState<SandTableSim | SandTableSimWasm | null>(null);
  const [isWasmReady, setIsWasmReady] = useState(false);
  const [t, setT] = useState(0);
  const [isRunning, setIsRunning] = useState(true);
  const [colorPalette, setColorPalette] = useState<RGB[] | null>(null);
  const animationFrameRef = useRef<number>();

  // Cross-section state
  const [crossSectionEnabled, setCrossSectionEnabled] = useState(false);
  const [crossSectionStart, setCrossSectionStart] = useState<Point | null>(null);
  const [crossSectionEnd, setCrossSectionEnd] = useState<Point | null>(null);
  const [crossSectionData, setCrossSectionData] = useState<CrossSectionData[]>([]);
  const [isDrawingCrossSection, setIsDrawingCrossSection] = useState(false);

  // Initialize WASM or fallback to JavaScript
  useEffect(() => {
    let cancelled = false;
    
    (async () => {
      try {
        // Try to initialize WASM
        await initializeWasm();
        if (cancelled) return;
        
        const wasmSim = await SandTableSimWasm.create({ ...DEFAULT_OPTIONS, ...options });
        if (cancelled) return;
        
        setSimulation(wasmSim);
        setIsWasmReady(true);
        console.log('✓ Using WebAssembly-accelerated simulation (4-5× faster)');
      } catch (error) {
        // Fallback to JavaScript implementation
        console.warn('WASM initialization failed, using JavaScript fallback:', error);
        if (cancelled) return;
        
        const jsSim = new SandTableSim({ ...DEFAULT_OPTIONS, ...options });
        setSimulation(jsSim);
        setIsWasmReady(false);
      }
    })();
    
    return () => {
      cancelled = true;
    };
  }, []); // Only run once on mount

  // Extract color palette when image changes
  useEffect(() => {
    if (colorImage && colorImage.complete && colorImage.naturalWidth > 0) {
      const palette = extractColorPalette(colorImage, 30);
      setColorPalette(palette);
      // Log colors to console for copying
      console.log('=== Color Palette Extracted ===');
      console.log('const DEFAULT_COLOR_PALETTE: RGB[] = [');
      palette.forEach((color, idx) => {
        console.log(`  { r: ${color.r}, g: ${color.g}, b: ${color.b}, a: 1 }, // ${idx}`);
      });
      console.log('];');
      console.log('===============================');
    } else {
      setColorPalette(null);
    }
  }, [colorImage]);

  // Reset when pattern changes
  useEffect(() => {
    if (!simulation) return;
    simulation.reset();
    setT(0);
    onReset?.();
  }, [pattern, simulation, onReset]);

  // Update simulation parameters when options change
  useEffect(() => {
    if (!simulation) return;
    if (options.ballDiameter !== undefined) {
      simulation.setBallDiameter(options.ballDiameter);
    }
    if (options.moveSpeed !== undefined) {
      simulation.setMoveSpeed(options.moveSpeed);
    }
  }, [options.ballDiameter, options.moveSpeed, simulation]);

  // Animation loop
  useEffect(() => {
    if (!isRunning || pattern === 'none' || !simulation) return;

    const animate = () => {
      const canvas = canvasRef.current;
      if (!canvas) return;

      const ctx = canvas.getContext('2d');
      if (!ctx) return;

      // Clear canvas
      ctx.fillStyle = '#1a1a1a';
      ctx.fillRect(0, 0, width, height);

      // Get pattern point
      const point = getPatternPoint(pattern, t, maxRadius, thrTrack);
      
      // Update simulation
      simulation.setTargetPosition(point);
      simulation.update();

      // Render sand
      renderSand(ctx, simulation, colorPalette, width, height);

      // Draw ball position
      const ballPos = simulation.getCurrentPosition();
      const canvasX = width / 2 + (ballPos.x - simulation.getKernel().getTableSize() / 2) * (width / simulation.getKernel().getTableSize());
      const canvasY = height / 2 + (ballPos.y - simulation.getKernel().getTableSize() / 2) * (height / simulation.getKernel().getTableSize());
      
      ctx.fillStyle = 'rgba(255, 100, 100, 0.8)';
      ctx.beginPath();
      ctx.arc(canvasX, canvasY, simulation.getBallDiameter() * (width / simulation.getKernel().getTableSize()), 0, Math.PI * 2);
      ctx.fill();

      // Draw pattern path preview (if enabled)
      if (showPathPreview) {
        ctx.strokeStyle = 'rgba(100, 100, 255, 0.3)';
        ctx.lineWidth = 2;
        ctx.beginPath();
        for (let i = 0; i < 100; i++) {
          const futureT = t + i * 10;
          const futurePoint = getPatternPoint(pattern, futureT, maxRadius, thrTrack);
          const futureX = width / 2 + futurePoint.x * (width / 500);
          const futureY = height / 2 + futurePoint.y * (height / 500);
          if (i === 0) {
            ctx.moveTo(futureX, futureY);
          } else {
            ctx.lineTo(futureX, futureY);
          }
        }
        ctx.stroke();
      }

      // Draw cross-section line (if active)
      if (crossSectionEnabled && crossSectionStart && crossSectionEnd) {
        ctx.strokeStyle = '#00FF00';
        ctx.lineWidth = 2;
        ctx.setLineDash([]);
        ctx.beginPath();
        ctx.moveTo(crossSectionStart.x, crossSectionStart.y);
        ctx.lineTo(crossSectionEnd.x, crossSectionEnd.y);
        ctx.stroke();

        // Draw endpoint markers
        ctx.fillStyle = '#00FF00';
        ctx.beginPath();
        ctx.arc(crossSectionStart.x, crossSectionStart.y, 5, 0, Math.PI * 2);
        ctx.fill();
        ctx.beginPath();
        ctx.arc(crossSectionEnd.x, crossSectionEnd.y, 5, 0, Math.PI * 2);
        ctx.fill();
      }

      setT(prev => prev + drawingSpeed);
      animationFrameRef.current = requestAnimationFrame(animate);
    };

    animationFrameRef.current = requestAnimationFrame(animate);

    return () => {
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current);
      }
    };
  }, [isRunning, pattern, t, simulation, colorPalette, width, height, maxRadius, thrTrack, drawingSpeed, showPathPreview, crossSectionEnabled, crossSectionStart, crossSectionEnd]);

  // Static render when paused (for cross-section mode)
  useEffect(() => {
    if (isRunning || !simulation) return;

    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    // Clear canvas
    ctx.fillStyle = '#1a1a1a';
    ctx.fillRect(0, 0, width, height);

    // Render sand
    renderSand(ctx, simulation, colorPalette, width, height);

    // Draw ball position
    const ballPos = simulation.getCurrentPosition();
    const canvasX = width / 2 + (ballPos.x - simulation.getKernel().getTableSize() / 2) * (width / simulation.getKernel().getTableSize());
    const canvasY = height / 2 + (ballPos.y - simulation.getKernel().getTableSize() / 2) * (height / simulation.getKernel().getTableSize());
    
    ctx.fillStyle = 'rgba(255, 100, 100, 0.8)';
    ctx.beginPath();
    ctx.arc(canvasX, canvasY, simulation.getBallDiameter() * (width / simulation.getKernel().getTableSize()), 0, Math.PI * 2);
    ctx.fill();

    // Draw cross-section line (if active)
    if (crossSectionEnabled && crossSectionStart && crossSectionEnd) {
      ctx.strokeStyle = '#00FF00';
      ctx.lineWidth = 3;
      ctx.setLineDash([]);
      ctx.beginPath();
      ctx.moveTo(crossSectionStart.x, crossSectionStart.y);
      ctx.lineTo(crossSectionEnd.x, crossSectionEnd.y);
      ctx.stroke();

      // Draw endpoint markers
      ctx.fillStyle = '#00FF00';
      ctx.beginPath();
      ctx.arc(crossSectionStart.x, crossSectionStart.y, 6, 0, Math.PI * 2);
      ctx.fill();
      ctx.beginPath();
      ctx.arc(crossSectionEnd.x, crossSectionEnd.y, 6, 0, Math.PI * 2);
      ctx.fill();
    }
  }, [isRunning, simulation, colorPalette, width, height, crossSectionEnabled, crossSectionStart, crossSectionEnd]);

  const handleReset = () => {
    if (!simulation) return;
    simulation.reset();
    setT(0);
    onReset?.();
  };

  const toggleRunning = () => {
    setIsRunning(prev => !prev);
  };

  const toggleCrossSection = () => {
    setCrossSectionEnabled(prev => !prev);
    if (!crossSectionEnabled) {
      // Entering cross-section mode - pause simulation
      setIsRunning(false);
    }
    // Clear existing cross-section
    setCrossSectionStart(null);
    setCrossSectionEnd(null);
    setCrossSectionData([]);
  };

  // Sample sand heights along the cross-section line
  const sampleCrossSectionData = (start: Point, end: Point) => {
    if (!simulation) return;

    const kernel = simulation.getKernel();
    const tableSize = kernel.getTableSize();

    // Convert canvas coordinates to table coordinates
    const startTable = {
      x: (start.x / width) * tableSize,
      y: (start.y / height) * tableSize,
    };
    const endTable = {
      x: (end.x / width) * tableSize,
      y: (end.y / height) * tableSize,
    };

    const dx = endTable.x - startTable.x;
    const dy = endTable.y - startTable.y;
    const lineLength = Math.sqrt(dx * dx + dy * dy);

    // Sample at sub-pixel resolution for smooth profile
    const numSamples = Math.max(100, Math.floor(lineLength * 2));
    const samples: CrossSectionData[] = [];

    for (let i = 0; i <= numSamples; i++) {
      const t = i / numSamples;
      const x = startTable.x + dx * t;
      const y = startTable.y + dy * t;

      const height = kernel.getSandLevel(x, y);
      const distance = lineLength * t;

      samples.push({ position: t, height, distance });
    }

    setCrossSectionData(samples);
  };

  // Mouse handlers for cross-section drawing
  const handleCanvasMouseDown = (e: React.MouseEvent<HTMLCanvasElement>) => {
    if (!crossSectionEnabled) return;

    const canvas = canvasRef.current;
    if (!canvas) return;

    const rect = canvas.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    setCrossSectionStart({ x, y });
    setCrossSectionEnd({ x, y });
    setIsDrawingCrossSection(true);
  };

  const handleCanvasMouseMove = (e: React.MouseEvent<HTMLCanvasElement>) => {
    if (!crossSectionEnabled || !isDrawingCrossSection || !crossSectionStart) return;

    const canvas = canvasRef.current;
    if (!canvas) return;

    const rect = canvas.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    setCrossSectionEnd({ x, y });
  };

  const handleCanvasMouseUp = (e: React.MouseEvent<HTMLCanvasElement>) => {
    if (!crossSectionEnabled || !isDrawingCrossSection) return;

    const canvas = canvasRef.current;
    if (!canvas) return;

    const rect = canvas.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    const end = { x, y };
    setCrossSectionEnd(end);
    setIsDrawingCrossSection(false);

    // Sample the cross-section data
    if (crossSectionStart) {
      sampleCrossSectionData(crossSectionStart, end);
    }
  };

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: '10px', alignItems: 'center' }}>
      {!simulation && (
        <div style={{
          position: 'absolute',
          top: '50%',
          left: '50%',
          transform: 'translate(-50%, -50%)',
          color: '#fff',
          fontSize: '16px',
          backgroundColor: 'rgba(0, 0, 0, 0.8)',
          padding: '20px',
          borderRadius: '8px',
        }}>
          Loading simulation...
        </div>
      )}
      <canvas
        ref={canvasRef}
        width={width}
        height={height}
        onMouseDown={handleCanvasMouseDown}
        onMouseMove={handleCanvasMouseMove}
        onMouseUp={handleCanvasMouseUp}
        style={{
          border: crossSectionEnabled ? '2px solid #00FF00' : '2px solid #333',
          borderRadius: '8px',
          backgroundColor: '#1a1a1a',
          cursor: crossSectionEnabled ? 'crosshair' : 'default',
        }}
      />
      <div style={{ display: 'flex', gap: '10px', alignItems: 'center' }}>
        <button onClick={toggleRunning} style={buttonStyle} disabled={crossSectionEnabled}>
          {isRunning ? 'Pause' : 'Resume'}
        </button>
        <button onClick={handleReset} style={buttonStyle}>
          Reset
        </button>
        <button 
          onClick={toggleCrossSection} 
          style={{
            ...buttonStyle,
            backgroundColor: crossSectionEnabled ? '#00AA00' : '#4a4a4a',
            fontWeight: crossSectionEnabled ? 'bold' : 'normal',
          }}
        >
          {crossSectionEnabled ? '✓ Cross Section' : 'Cross Section'}
        </button>
        {isWasmReady && (
          <span style={{
            color: '#4CAF50',
            fontSize: '12px',
            marginLeft: '10px',
            border: '1px solid #4CAF50',
            padding: '4px 8px',
            borderRadius: '4px',
          }}>
            ⚡ WASM (4-5× faster)
          </span>
        )}
      </div>

      {/* Cross-section instructions */}
      {crossSectionEnabled && (
        <div style={{
          color: '#00FF00',
          fontSize: '13px',
          padding: '8px 12px',
          backgroundColor: 'rgba(0, 255, 0, 0.1)',
          border: '1px solid #00FF00',
          borderRadius: '4px',
          maxWidth: '600px',
          textAlign: 'center',
        }}>
          <strong>Cross-Section Mode:</strong> Click and drag across the sand table to draw a measurement line
        </div>
      )}

      {/* Cross-section chart */}
      {crossSectionEnabled && crossSectionData.length > 0 && crossSectionStart && crossSectionEnd && simulation && (
        <CrossSectionChart
          data={crossSectionData}
          startPoint={crossSectionStart}
          endPoint={crossSectionEnd}
          maxHeight={DEFAULT_OPTIONS.maxSandLevel}
          startLevel={DEFAULT_OPTIONS.sandStartLevel}
          width={width}
          height={200}
        />
      )}
    </div>
  );
};

/**
 * Extract a color palette from an image by sampling across it.
 * Samples horizontally at 50% height and diagonally for variety.
 * @param image - The source image
 * @param numSamples - Number of color samples to extract (default 30)
 * @returns Array of RGB colors representing a gradient from dark to light
 */
function extractColorPalette(image: HTMLImageElement, numSamples: number = 30): RGB[] {
  const canvas = document.createElement('canvas');
  canvas.width = image.width;
  canvas.height = image.height;
  const ctx = canvas.getContext('2d');
  
  if (!ctx) return [];
  
  ctx.drawImage(image, 0, 0);
  const imageData = ctx.getImageData(0, 0, image.width, image.height);
  const data = imageData.data;
  
  const palette: RGB[] = [];
  
  // Sample across the image in multiple ways to get color variety
  // Strategy: sample diagonally, horizontally at midpoint, and vertically at midpoint
  const samplePoints: Array<{x: number, y: number}> = [];
  
  // Diagonal samples (top-left to bottom-right)
  for (let i = 0; i < numSamples / 3; i++) {
    const t = i / (numSamples / 3);
    samplePoints.push({
      x: Math.floor(t * image.width),
      y: Math.floor(t * image.height)
    });
  }
  
  // Horizontal samples at middle height
  for (let i = 0; i < numSamples / 3; i++) {
    const t = i / (numSamples / 3);
    samplePoints.push({
      x: Math.floor(t * image.width),
      y: Math.floor(image.height / 2)
    });
  }
  
  // Vertical samples at middle width
  for (let i = 0; i < numSamples / 3; i++) {
    const t = i / (numSamples / 3);
    samplePoints.push({
      x: Math.floor(image.width / 2),
      y: Math.floor(t * image.height)
    });
  }
  
  // Extract colors at sample points
  for (const point of samplePoints) {
    const x = Math.min(point.x, image.width - 1);
    const y = Math.min(point.y, image.height - 1);
    const idx = (y * image.width + x) * 4;
    
    palette.push({
      r: data[idx],
      g: data[idx + 1],
      b: data[idx + 2],
      a: 1
    });
  }
  
  // Sort by luminance (brightness) to create a gradient from dark to light
  palette.sort((a, b) => {
    const lumA = 0.299 * a.r + 0.587 * a.g + 0.114 * a.b;
    const lumB = 0.299 * b.r + 0.587 * b.g + 0.114 * b.b;
    return lumA - lumB;
  });
  
  return palette;
}

function renderSand(
  ctx: CanvasRenderingContext2D,
  simulation: SandTableSim,
  colorPalette: RGB[] | null,
  canvasWidth: number,
  canvasHeight: number
): void {
  const kernel = simulation.getKernel();
  const tableSize = kernel.getTableSize();
  
  // Enable canvas smoothing for anti-aliased rendering
  ctx.imageSmoothingEnabled = true;
  ctx.imageSmoothingQuality = 'high';
  
  const imageData = ctx.createImageData(canvasWidth, canvasHeight);
  const data = imageData.data;

  for (let py = 0; py < canvasHeight; py++) {
    for (let px = 0; px < canvasWidth; px++) {
      // Convert canvas pixel to table coordinates
      const tx = ((px - canvasWidth / 2) / canvasWidth) * tableSize + tableSize / 2;
      const ty = ((py - canvasHeight / 2) / canvasHeight) * tableSize + tableSize / 2;
      
      // Check if within circular table
      const dx = tx - tableSize / 2;
      const dy = ty - tableSize / 2;
      const dist = Math.sqrt(dx * dx + dy * dy);
      
      if (dist > tableSize / 2) {
        // Outside circle - black
        const idx = (py * canvasWidth + px) * 4;
        data[idx] = 0;
        data[idx + 1] = 0;
        data[idx + 2] = 0;
        data[idx + 3] = 255;
        continue;
      }
      
      // Bilinear interpolation for smoother sand height sampling
      const tx_floor = Math.floor(tx);
      const ty_floor = Math.floor(ty);
      const tx_frac = tx - tx_floor;
      const ty_frac = ty - ty_floor;
      
      // Sample 4 neighboring points
      const h00 = kernel.getSandLevel(tx_floor, ty_floor);
      const h10 = kernel.getSandLevel(tx_floor + 1, ty_floor);
      const h01 = kernel.getSandLevel(tx_floor, ty_floor + 1);
      const h11 = kernel.getSandLevel(tx_floor + 1, ty_floor + 1);
      
      // Bilinear interpolation
      const h0 = h00 * (1 - tx_frac) + h10 * tx_frac;
      const h1 = h01 * (1 - tx_frac) + h11 * tx_frac;
      const level = h0 * (1 - ty_frac) + h1 * ty_frac;
      
      // Calculate slopes for lighting (finite difference method)
      const levelRight = kernel.getSandLevel(tx + 1, ty);
      const levelLeft = kernel.getSandLevel(tx - 1, ty);
      const levelDown = kernel.getSandLevel(tx, ty + 1);
      const levelUp = kernel.getSandLevel(tx, ty - 1);
      
      const slopeDx = levelRight - levelLeft;
      const slopeDy = levelDown - levelUp;
      
      // Light direction (from top-left: -0.7, -0.7)
      const lightDirX = -0.7;
      const lightDirY = -0.7;
      
      // Calculate slope in light direction (dot product)
      const slope = slopeDx * lightDirX + slopeDy * lightDirY;
      
      // Calculate lighting factor based on slope
      let lightingFactor = 1.0;
      if (slope > 0) {
        // Facing light - brighter
        lightingFactor = 1.0 + slope * 1.1;
      } else {
        // Facing away - darker (more dramatic)
        lightingFactor = 1.0 + slope * 2.0;
      }
      
      // Clamp lighting factor to reasonable range
      lightingFactor = Math.max(0.4, Math.min(1.2, lightingFactor));
      
      // Use loaded color palette if available, otherwise use default palette
      const palette = colorPalette && colorPalette.length > 0 ? colorPalette : DEFAULT_COLOR_PALETTE;
      
      // Map sand level to color palette with smooth interpolation
      // Level 0 (lowest) = darkest color, level 20 (highest) = lightest color
      const normalized = Math.max(0, Math.min(1, level / 20)); // Clamp to 0-1
      const palettePos = normalized * (palette.length - 1); // Position in palette (float)
      const colorIndex1 = Math.floor(palettePos);
      const colorIndex2 = Math.min(colorIndex1 + 1, palette.length - 1);
      const colorFrac = palettePos - colorIndex1; // Fraction between colors
      
      // Interpolate between two palette colors for smooth gradients
      const color1 = palette[colorIndex1];
      const color2 = palette[colorIndex2];
      const baseRgb: RGB = {
        r: color1.r * (1 - colorFrac) + color2.r * colorFrac,
        g: color1.g * (1 - colorFrac) + color2.g * colorFrac,
        b: color1.b * (1 - colorFrac) + color2.b * colorFrac,
        a: 255
      };
      
      // Apply lighting to color
      const rgb: RGB = {
        r: Math.min(255, Math.round(baseRgb.r * lightingFactor)),
        g: Math.min(255, Math.round(baseRgb.g * lightingFactor)),
        b: Math.min(255, Math.round(baseRgb.b * lightingFactor)),
        a: baseRgb.a
      };
      
      const idx = (py * canvasWidth + px) * 4;
      data[idx] = rgb.r;
      data[idx + 1] = rgb.g;
      data[idx + 2] = rgb.b;
      data[idx + 3] = 255;
    }
  }
  
  ctx.putImageData(imageData, 0, 0);
}

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
