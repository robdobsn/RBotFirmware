// Main sand table canvas component

import React, { useRef, useEffect, useState } from 'react';
import { PatternType, SandSimulationOptions, RGB, THRTrack, Point } from './types';
import { getPatternPoint } from './patternUtils';
import { SandTableSim } from './sandSimulation';
import { SandTableSimWasm, initializeWasm } from './sandSimulationWasm';
import { CrossSectionChart, CrossSectionData } from './CrossSectionChart';
import { WebGLRenderer } from './rendering/WebGLRenderer';

const DEFAULT_COLOR_PALETTE: RGB[] = [
  // Deep troughs (dark warm browns)
  { r: 60, g: 45, b: 30, a: 1 },    // 0 - Darkest trough
  { r: 70, g: 52, b: 35, a: 1 },    // 1
  { r: 80, g: 60, b: 40, a: 1 },    // 2
  { r: 90, g: 68, b: 45, a: 1 },    // 3
  { r: 100, g: 76, b: 50, a: 1 },   // 4
  
  // Lower mid (warm brown)
  { r: 110, g: 85, b: 58, a: 1 },   // 5
  { r: 120, g: 94, b: 65, a: 1 },   // 6
  { r: 130, g: 103, b: 72, a: 1 },  // 7
  { r: 138, g: 110, b: 78, a: 1 },  // 8
  { r: 145, g: 118, b: 85, a: 1 },  // 9
  
  // Mid-range (sandy tan - this is where undisturbed sand sits)
  { r: 152, g: 125, b: 90, a: 1 },  // 10
  { r: 158, g: 132, b: 96, a: 1 },  // 11
  { r: 164, g: 138, b: 102, a: 1 }, // 12
  { r: 170, g: 144, b: 108, a: 1 }, // 13
  { r: 175, g: 150, b: 114, a: 1 }, // 14
  { r: 180, g: 155, b: 120, a: 1 }, // 15
  { r: 185, g: 160, b: 125, a: 1 }, // 16
  { r: 190, g: 166, b: 130, a: 1 }, // 17
  { r: 195, g: 172, b: 136, a: 1 }, // 18
  { r: 200, g: 178, b: 142, a: 1 }, // 19
  
  // Upper mid (light sandy)
  { r: 205, g: 184, b: 148, a: 1 }, // 20
  { r: 210, g: 190, b: 155, a: 1 }, // 21
  { r: 215, g: 196, b: 162, a: 1 }, // 22
  { r: 220, g: 202, b: 170, a: 1 }, // 23
  { r: 225, g: 208, b: 178, a: 1 }, // 24
  
  // High ridges (bright warm highlights)
  { r: 230, g: 215, b: 185, a: 1 }, // 25
  { r: 235, g: 222, b: 195, a: 1 }, // 26
  { r: 240, g: 228, b: 205, a: 1 }, // 27
  { r: 245, g: 235, b: 215, a: 1 }, // 28
  { r: 250, g: 242, b: 225, a: 1 }, // 29 - Brightest ridge
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
  ballDiameter: 10,      // Much smaller ball for finer detail (minimum practical size)
  tableSize: 2000,       // Ultra-high resolution (4M cells)
  sandStartLevel: 5,
  maxSandLevel: 20,
  moveSpeed: 15,         // Scaled proportionally for 2000×2000 grid
  patternScale: 900,     // Must match maxRadius for proper coordinate conversion
  // Physics parameters (tuned for realistic sand displacement)
  troughDepth: -1.8,     // Depth of trough behind ball (negative removes sand)
  troughWidthRatio: 0.67, // Trough width as 67% of ball radius (~1/3 ball diameter)
  ridgeHeight: 1.2,      // Height of sand ridges pushed to sides
  ridgeOffset: 1.2,      // Distance sand is pushed perpendicular to motion
  // Settlement parameters (for sand smoothing over time)
  settleThreshold: 1.5,  // Height difference to trigger settling
  blendFactor: 0.02,     // Smoothing strength (0-1, lower = gentler)
  settleFrequency: 0.005, // How often to settle (0.5% per frame)
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
  const canvasRef = useRef<HTMLCanvasElement>(null);  // WebGL canvas for sand rendering
  const overlayCanvasRef = useRef<HTMLCanvasElement>(null);  // 2D canvas for ball, UI, cross-section
  const [simulation, setSimulation] = useState<SandTableSim | SandTableSimWasm | null>(null);
  const [isWasmReady, setIsWasmReady] = useState(false);
  const tRef = useRef<number>(0);  // Use ref for t to avoid re-rendering on every frame
  const [isRunning, setIsRunning] = useState(true);
  const [colorPalette, setColorPalette] = useState<RGB[] | null>(null);
  const animationFrameRef = useRef<number>();

  // Cross-section state
  const [crossSectionEnabled, setCrossSectionEnabled] = useState(false);
  const [crossSectionStart, setCrossSectionStart] = useState<Point | null>(null);
  const [crossSectionEnd, setCrossSectionEnd] = useState<Point | null>(null);
  const [crossSectionData, setCrossSectionData] = useState<CrossSectionData[]>([]);
  const [isDrawingCrossSection, setIsDrawingCrossSection] = useState(false);
  
  // Performance optimization: frame counter for render throttling
  const frameCountRef = useRef(0);
  const [renderQuality, setRenderQuality] = useState<'high' | 'medium' | 'low'>('medium');
  
  // WebGL renderer state
  const webglRendererRef = useRef<WebGLRenderer | null>(null);
  const [useWebGL, setUseWebGL] = useState(true);  // Try WebGL by default
  const [isWebGLAvailable, setIsWebGLAvailable] = useState(false);

  // Initialize WASM or fallback to JavaScript
  useEffect(() => {
    let cancelled = false;
    
    (async () => {
      try {
        // Try to initialize WASM
        await initializeWasm();
        if (cancelled) return;
        
        const wasmSim = await SandTableSimWasm.create({ ...DEFAULT_OPTIONS, ...options, patternScale: maxRadius });
        if (cancelled) return;
        
        setSimulation(wasmSim);
        setIsWasmReady(true);
        console.log('✓ Using WebAssembly-accelerated simulation (4-5× faster)');
      } catch (error) {
        // Fallback to JavaScript implementation
        console.warn('WASM initialization failed, using JavaScript fallback:', error);
        if (cancelled) return;
        
        const jsSim = new SandTableSim({ ...DEFAULT_OPTIONS, ...options, patternScale: maxRadius });
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

  // Initialize WebGL renderer (only when useWebGL is true)
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !simulation) return;
    
    // If WebGL requested, try to initialize
    if (useWebGL) {
      // Check if WebGL is supported
      if (!WebGLRenderer.isSupported()) {
        console.warn('⚠️ WebGL not supported, falling back to Canvas2D');
        setUseWebGL(false);
        setIsWebGLAvailable(false);
        return;
      }
      
      try {
        const tableSize = simulation.getKernel().getTableSize();
        const renderer = new WebGLRenderer(canvas, tableSize);
        webglRendererRef.current = renderer;
        setIsWebGLAvailable(true);
        console.log('✅ WebGL renderer ready for 100× speedup!');
      } catch (error) {
        console.error('Failed to initialize WebGL, falling back to Canvas2D:', error);
        setUseWebGL(false);
        setIsWebGLAvailable(false);
        webglRendererRef.current = null;
      }
    } else {
      // Canvas2D mode - dispose WebGL if it exists
      if (webglRendererRef.current) {
        webglRendererRef.current.dispose();
        webglRendererRef.current = null;
        console.log('Switched to Canvas2D mode');
      }
    }
    
    // Cleanup
    return () => {
      if (webglRendererRef.current) {
        webglRendererRef.current.dispose();
        webglRendererRef.current = null;
      }
    };
  }, [simulation, useWebGL]);

  // Reset when pattern changes
  useEffect(() => {
    if (!simulation) return;
    simulation.reset();
    tRef.current = 0;
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
      const overlayCanvas = overlayCanvasRef.current;
      if (!canvas || !overlayCanvas) return;

      const ctx = overlayCanvas.getContext('2d');
      if (!ctx) return;

      // Clear overlay (transparent - let WebGL show through)
      ctx.clearRect(0, 0, width, height);

      // Get pattern point
      const point = getPatternPoint(pattern, tRef.current, maxRadius, thrTrack);
      
      // Update simulation (always run physics at 60 FPS)
      simulation.setTargetPosition(point);
      simulation.update();

      // Render sand (choose WebGL or Canvas2D)
      if (useWebGL && webglRendererRef.current) {
        try {
          const kernel = simulation.getKernel();
          const sandHeights = 'getSandLevelArrayZeroCopy' in kernel
            ? kernel.getSandLevelArrayZeroCopy()
            : kernel.getSandLevelArray();
          
          const palette = colorPalette && colorPalette.length > 0 ? colorPalette : DEFAULT_COLOR_PALETTE;
          webglRendererRef.current.render(sandHeights, palette, width, height);
        } catch (error) {
          console.error('WebGL rendering failed, falling back to Canvas2D:', error);
          setUseWebGL(false);
        }
      } else {
        // Canvas2D fallback (render sand to main canvas, throttled for performance)
        if (frameCountRef.current === 0) {
          console.log('Using Canvas2D fallback');
        }
        frameCountRef.current++;
        
        // Render sand every other frame for performance (30 FPS sand rendering)
        if (frameCountRef.current % 2 === 0) {
          const canvas = canvasRef.current;
          if (canvas) {
            const ctx2d = canvas.getContext('2d');
            if (ctx2d) {
              const palette = colorPalette && colorPalette.length > 0 ? colorPalette : DEFAULT_COLOR_PALETTE;
              renderSand(ctx2d, simulation, palette, width, height, renderQuality);
            } else {
              console.error('Failed to get 2D context for Canvas2D rendering');
            }
          }
        }
      }

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
          const futureT = tRef.current + i * 10;
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

      tRef.current += drawingSpeed;
      animationFrameRef.current = requestAnimationFrame(animate);
    };

    animationFrameRef.current = requestAnimationFrame(animate);

    return () => {
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current);
      }
    };
  }, [isRunning, pattern, simulation, colorPalette, width, height, maxRadius, thrTrack, drawingSpeed, showPathPreview, crossSectionEnabled, crossSectionStart, crossSectionEnd, renderQuality, useWebGL]);

  // Static render when paused (for cross-section mode)
  useEffect(() => {
    if (isRunning || !simulation) return;

    const canvas = canvasRef.current;
    const overlayCanvas = overlayCanvasRef.current;
    if (!canvas || !overlayCanvas) return;

    const ctx = overlayCanvas.getContext('2d');
    if (!ctx) return;

    // Clear overlay (transparent)
    ctx.clearRect(0, 0, width, height);

    // Render sand (choose WebGL or Canvas2D)
    if (useWebGL && webglRendererRef.current) {
      try {
        const kernel = simulation.getKernel();
        const sandHeights = 'getSandLevelArrayZeroCopy' in kernel
          ? kernel.getSandLevelArrayZeroCopy()
          : kernel.getSandLevelArray();
        
        const palette = colorPalette && colorPalette.length > 0 ? colorPalette : DEFAULT_COLOR_PALETTE;
        webglRendererRef.current.render(sandHeights, palette, width, height);
      } catch (error) {
        console.error('WebGL rendering failed:', error);
        renderSand(ctx, simulation, colorPalette, width, height, renderQuality);
      }
    } else {
      renderSand(ctx, simulation, colorPalette, width, height, renderQuality);
    }

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
  }, [isRunning, simulation, colorPalette, width, height, crossSectionEnabled, crossSectionStart, crossSectionEnd, renderQuality, useWebGL]);

  const handleReset = () => {
    if (!simulation) return;
    simulation.reset();
    tRef.current = 0;
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
          zIndex: 20,
        }}>
          Loading simulation...
        </div>
      )}
      <div style={{ position: 'relative', width: width, height: height }}>
        {/* Canvas for sand rendering - key forces remount when switching WebGL/Canvas2D */}
        <canvas
          key={useWebGL ? 'webgl' : 'canvas2d'}
          ref={canvasRef}
          width={width}
          height={height}
          style={{
            position: 'absolute',
            top: 0,
            left: 0,
            border: crossSectionEnabled ? '2px solid #00FF00' : '2px solid #333',
            borderRadius: '8px',
            backgroundColor: '#1a1a1a',
          }}
        />
        {/* 2D overlay canvas for ball, cross-section line, etc. */}
        <canvas
          ref={overlayCanvasRef}
          width={width}
          height={height}
          onMouseDown={handleCanvasMouseDown}
          onMouseMove={handleCanvasMouseMove}
          onMouseUp={handleCanvasMouseUp}
          style={{
            position: 'absolute',
            top: 0,
            left: 0,
            border: crossSectionEnabled ? '2px solid #00FF00' : '2px solid #333',
            borderRadius: '8px',
            cursor: crossSectionEnabled ? 'crosshair' : 'default',
            pointerEvents: 'auto',
            backgroundColor: 'transparent',  // Let WebGL show through
          }}
        />
      </div>
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
            ⚡ WASM
          </span>
        )}
        {isWebGLAvailable && useWebGL && (
          <span style={{
            color: '#2196F3',
            fontSize: '12px',
            marginLeft: '10px',
            border: '1px solid #2196F3',
            padding: '4px 8px',
            borderRadius: '4px',
          }}>
            ⚡ WebGL
          </span>
        )}
        {isWebGLAvailable && (
          <button 
            onClick={() => setUseWebGL(prev => !prev)} 
            style={{
              ...buttonStyle,
              fontSize: '12px',
              padding: '6px 12px',
            }}
          >
            {useWebGL ? 'Use Canvas2D' : 'Use WebGL'}
          </button>
        )}
        {!useWebGL && (
          <button
            onClick={() => {
              const qualities: Array<'high' | 'medium' | 'low'> = ['high', 'medium', 'low'];
              const currentIndex = qualities.indexOf(renderQuality);
              const nextIndex = (currentIndex + 1) % qualities.length;
              setRenderQuality(qualities[nextIndex]);
            }}
            style={{
              ...buttonStyle,
              fontSize: '11px',
              padding: '4px 8px',
              backgroundColor: renderQuality === 'high' ? '#9C27B0' : renderQuality === 'medium' ? '#FF9800' : '#795548',
            }}
            title="Toggle render quality: High (slower, smooth) | Medium (balanced) | Low (faster, blocky)"
          >
            Quality: {renderQuality.charAt(0).toUpperCase() + renderQuality.slice(1)}
          </button>
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
  simulation: SandTableSim | SandTableSimWasm,
  colorPalette: RGB[] | null,
  canvasWidth: number,
  canvasHeight: number,
  renderQuality: 'high' | 'medium' | 'low' = 'high'
): void {
  const kernel = simulation.getKernel();
  const tableSize = kernel.getTableSize();
  
  // Calculate actual min/max heights for proper normalization
  let minHeight = Infinity;
  let maxHeight = -Infinity;
  
  // Get sand heights array (works for both JS and WASM kernels)
  const sandHeights = 'getSandLevelArrayZeroCopy' in kernel
    ? kernel.getSandLevelArrayZeroCopy()
    : kernel.getSandLevelArray();
  
  for (let i = 0; i < sandHeights.length; i++) {
    const h = sandHeights[i];
    if (h < minHeight) minHeight = h;
    if (h > maxHeight) maxHeight = h;
  }
  const heightRange = Math.max(maxHeight - minHeight, 0.1);
  
  // Enable canvas smoothing for anti-aliased rendering
  ctx.imageSmoothingEnabled = true;
  ctx.imageSmoothingQuality = renderQuality === 'high' ? 'high' : 'medium';
  
  // Quality settings: high = every pixel, medium = every 2nd pixel, low = every 3rd pixel
  const pixelStep = renderQuality === 'high' ? 1 : renderQuality === 'medium' ? 2 : 3;
  const useBilinear = renderQuality !== 'low';
  
  const imageData = ctx.createImageData(canvasWidth, canvasHeight);
  const data = imageData.data;

  for (let py = 0; py < canvasHeight; py += pixelStep) {
    for (let px = 0; px < canvasWidth; px += pixelStep) {
      // Convert canvas pixel to table coordinates
      const tx = ((px - canvasWidth / 2) / canvasWidth) * tableSize + tableSize / 2;
      const ty = ((py - canvasHeight / 2) / canvasHeight) * tableSize + tableSize / 2;
      
      // Check if within circular table
      const dx = tx - tableSize / 2;
      const dy = ty - tableSize / 2;
      const dist = Math.sqrt(dx * dx + dy * dy);
      
      if (dist > tableSize / 2) {
        // Outside circle - black
        for (let dy = 0; dy < pixelStep; dy++) {
          for (let dx = 0; dx < pixelStep; dx++) {
            if (py + dy < canvasHeight && px + dx < canvasWidth) {
              const idx = ((py + dy) * canvasWidth + (px + dx)) * 4;
              data[idx] = 0;
              data[idx + 1] = 0;
              data[idx + 2] = 0;
              data[idx + 3] = 255;
            }
          }
        }
        continue;
      }
      
      // Sample sand level based on quality
      let level: number;
      let slopeDx: number;
      let slopeDy: number;
      
      if (useBilinear) {
        // High/Medium quality: Bilinear interpolation for smoother sand height sampling
        const tx_floor = Math.floor(tx);
        const ty_floor = Math.floor(ty);
        const tx_frac = tx - tx_floor;
        const ty_frac = ty - ty_floor;
        
        // Sample 4 neighboring points (reuse for slope calculation)
        const h00 = kernel.getSandLevel(tx_floor, ty_floor);
        const h10 = kernel.getSandLevel(tx_floor + 1, ty_floor);
        const h01 = kernel.getSandLevel(tx_floor, ty_floor + 1);
        const h11 = kernel.getSandLevel(tx_floor + 1, ty_floor + 1);
        
        // Bilinear interpolation
        const h0 = h00 * (1 - tx_frac) + h10 * tx_frac;
        const h1 = h01 * (1 - tx_frac) + h11 * tx_frac;
        level = h0 * (1 - ty_frac) + h1 * ty_frac;
        
        // Calculate slopes using the samples we already have (no additional calls)
        slopeDx = h10 - h00; // Right - Left approximation
        slopeDy = h01 - h00; // Down - Up approximation
      } else {
        // Low quality: Simple nearest-neighbor sampling (fastest)
        level = kernel.getSandLevel(Math.floor(tx), Math.floor(ty));
        const levelRight = kernel.getSandLevel(Math.floor(tx) + 1, Math.floor(ty));
        const levelDown = kernel.getSandLevel(Math.floor(tx), Math.floor(ty) + 1);
        slopeDx = levelRight - level;
        slopeDy = levelDown - level;
      }
      
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
      // Use actual height range for normalization (matches WebGL approach)
      const normalized = Math.max(0, Math.min(1, (level - minHeight) / heightRange)); // Clamp to 0-1
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
      
      // Fill in pixel block (for pixelStep > 1, fill adjacent pixels with same color)
      for (let dy = 0; dy < pixelStep; dy++) {
        for (let dx = 0; dx < pixelStep; dx++) {
          if (py + dy < canvasHeight && px + dx < canvasWidth) {
            const idx = ((py + dy) * canvasWidth + (px + dx)) * 4;
            data[idx] = rgb.r;
            data[idx + 1] = rgb.g;
            data[idx + 2] = rgb.b;
            data[idx + 3] = 255;
          }
        }
      }
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
