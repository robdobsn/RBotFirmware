// Utilities for parsing and handling Sisyphus .thr (track) files

import { THRPoint, THRTrack, Point } from './types';

/**
 * Parse a .thr file content into a THRTrack object
 * @param content - Raw text content of the .thr file
 * @param filename - Name of the file for identification
 * @returns Parsed THRTrack object
 */
export function parseTHRFile(content: string, filename: string): THRTrack {
  const lines = content.split('\n');
  const points: THRPoint[] = [];

  for (const line of lines) {
    const trimmed = line.trim();
    
    // Skip empty lines and comments
    if (trimmed === '' || trimmed.startsWith('#')) {
      continue;
    }

    // Parse "theta rho" format
    const parts = trimmed.split(/\s+/);
    if (parts.length >= 2) {
      const theta = parseFloat(parts[0]);
      const rho = parseFloat(parts[1]);

      if (!isNaN(theta) && !isNaN(rho)) {
        points.push({ theta, rho });
      }
    }
  }

  return {
    name: filename,
    points,
  };
}

/**
 * Convert polar coordinates (theta, rho) to Cartesian (x, y)
 * @param theta - Angle in radians
 * @param rho - Distance from center (0 to 1)
 * @param maxRadius - Maximum radius for scaling
 * @returns Cartesian point
 */
export function polarToCartesian(theta: number, rho: number, maxRadius: number): Point {
  const radius = rho * maxRadius;
  return {
    x: radius * Math.cos(theta),
    y: radius * Math.sin(theta),
  };
}

/**
 * Get a point along the THR track at a given time/index
 * @param track - THR track data
 * @param t - Time parameter (index into the track)
 * @param maxRadius - Maximum radius for coordinate scaling
 * @returns Cartesian point at the given time
 */
export function getTHRPoint(track: THRTrack, t: number, maxRadius: number): Point {
  if (track.points.length === 0) {
    return { x: 0, y: 0 };
  }

  // Wrap t to track length
  const index = Math.floor(t) % track.points.length;
  const point = track.points[index];
  
  return polarToCartesian(point.theta, point.rho, maxRadius);
}

/**
 * Get interpolated point along the THR track with smooth transitions
 * @param track - THR track data
 * @param t - Time parameter (can be fractional)
 * @param maxRadius - Maximum radius for coordinate scaling
 * @returns Interpolated Cartesian point
 */
export function getTHRPointInterpolated(track: THRTrack, t: number, maxRadius: number): Point {
  if (track.points.length === 0) {
    return { x: 0, y: 0 };
  }

  if (track.points.length === 1) {
    return polarToCartesian(track.points[0].theta, track.points[0].rho, maxRadius);
  }

  // Get current and next point indices
  const totalPoints = track.points.length;
  const index1 = Math.floor(t) % totalPoints;
  const index2 = (index1 + 1) % totalPoints;
  const fraction = t - Math.floor(t);

  const p1 = track.points[index1];
  const p2 = track.points[index2];

  // Interpolate in polar coordinates
  let thetaDiff = p2.theta - p1.theta;
  
  // Handle theta wraparound for shortest path
  if (thetaDiff > Math.PI) {
    thetaDiff -= 2 * Math.PI;
  } else if (thetaDiff < -Math.PI) {
    thetaDiff += 2 * Math.PI;
  }

  const theta = p1.theta + thetaDiff * fraction;
  const rho = p1.rho + (p2.rho - p1.rho) * fraction;

  return polarToCartesian(theta, rho, maxRadius);
}
