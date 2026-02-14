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
