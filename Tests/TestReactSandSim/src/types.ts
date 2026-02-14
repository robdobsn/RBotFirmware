// Types for the sand table simulation

export interface Point {
  x: number;
  y: number;
}

export interface RGB {
  r: number;
  g: number;
  b: number;
  a: number;
}

export interface HSL {
  h: number;
  s: number;
  l: number;
}

export type PatternType = 
  | 'none'
  | 'spiral'
  | 'logSpiral'
  | 'fermatSpiral'
  | 'rose'
  | 'lissajous'
  | 'epitrochoid'
  | 'hypotrochoid'
  | 'starfield'
  | 'circles'
  | 'spiral3d'
  | 'thr';

export interface PatternInfo {
  value: PatternType;
  label: string;
  description?: string;
}

export interface SandSimulationOptions {
  ballDiameter: number;
  tableSize: number;
  sandStartLevel: number;
  maxSandLevel: number;
  moveSpeed: number;
  // Coordinate scaling (for pattern-to-table conversion)
  patternScale?: number;     // Pattern coordinate range (default: 250, e.g., patterns use -250 to +250)
  // Physics parameters (optional - will use defaults if not provided)
  troughDepth?: number;       // Depth of trough behind ball (default: -1.8)
  troughWidthRatio?: number;  // Trough width as ratio of ball radius (default: 0.67)
  ridgeHeight?: number;       // Height of sand ridges on sides (default: 1.2)
  ridgeOffset?: number;       // Distance sand is pushed sideways (default: 1.2)
  // Settlement parameters (optional)
  settleThreshold?: number;   // Height difference to trigger settling (default: 1.5)
  blendFactor?: number;       // Smoothing strength (default: 0.02)
  settleFrequency?: number;   // How often to settle (0-1, default: 0.005)
}

export interface ColorSample {
  rgb: RGB;
  hsl: HSL;
}

export interface THRPoint {
  theta: number;  // Angle in radians
  rho: number;    // Distance from center (0 to 1)
}

export interface THRTrack {
  name: string;
  points: THRPoint[];
}
