// Pattern generation utilities for sand table

import { Point, PatternType, THRTrack } from './types';
import { getTHRPointInterpolated } from './thrUtils';

export function getPatternPoint(
  pattern: PatternType,
  t: number,
  maxRadius: number = 250,
  thrTrack?: THRTrack | null
): Point {
  let x = 0, y = 0;
  
  switch(pattern) {
    case 'thr': // THR file playback
      if (thrTrack && thrTrack.points.length > 0) {
        return getTHRPointInterpolated(thrTrack, t, maxRadius);
      }
      return { x: 0, y: 0 };
      
    case 'none': // No movement - stay at center
      x = 0;
      y = 0;
      break;
      
    case 'spiral': // Archimedean Spiral
      if (t < 2 * Math.PI * 410) {
        x = Math.sin(t/20) * (maxRadius * 0.92 - t/5);
        y = Math.cos(t/20) * (maxRadius * 0.92 - t/5);
      } else {
        x = Math.sin((t-15)/20) * maxRadius;
        y = Math.cos((t-15)/20) * maxRadius;
      }
      break;
        
    case 'logSpiral': // Logarithmic Spiral
      const a = maxRadius * 0.0003; // scale initial size with maxRadius
      const b = 0.2;
      const theta = t / 30;
      const r = Math.min(a * Math.exp(b * theta), maxRadius);
      x = r * Math.cos(theta);
      y = r * Math.sin(theta);
      break;
        
    case 'fermatSpiral': // Fermat's Spiral
      const angle = t / 15;
      const radius = Math.min(maxRadius * 0.06 * Math.sqrt(angle), maxRadius);
      x = radius * Math.cos(angle);
      y = radius * Math.sin(angle);
      break;
        
    case 'rose': // Rose Curve
      const n = 5; // number of petals
      const d = 3;
      const theta_rose = t / 50;
      const r_rose = maxRadius * 0.95 * Math.cos(n / d * theta_rose);
      x = r_rose * Math.cos(theta_rose);
      y = r_rose * Math.sin(theta_rose);
      break;
        
    case 'lissajous': // Lissajous Curve
      const A = maxRadius * 0.9;
      const B = maxRadius * 0.9;
      const a_liss = 3;
      const b_liss = 4;
      const delta = Math.PI / 2;
      x = A * Math.sin(a_liss * t / 100 + delta);
      y = B * Math.sin(b_liss * t / 100);
      break;
        
    case 'epitrochoid': // Epitrochoid
      const R = maxRadius * 0.4; // fixed circle radius
      const r_epi = maxRadius * 0.16; // rolling circle radius
      const d_epi = maxRadius * 0.32; // distance from center of rolling circle
      const theta_epi = t / 20;
      x = (R + r_epi) * Math.cos(theta_epi) - d_epi * Math.cos((R + r_epi) / r_epi * theta_epi);
      y = (R + r_epi) * Math.sin(theta_epi) - d_epi * Math.sin((R + r_epi) / r_epi * theta_epi);
      break;
        
    case 'hypotrochoid': // Hypotrochoid
      const R_hypo = maxRadius * 0.6; // fixed circle radius
      const r_hypo = maxRadius * 0.2; // rolling circle radius
      const d_hypo = maxRadius * 0.32; // distance from center of rolling circle
      const theta_hypo = t / 20;
      x = (R_hypo - r_hypo) * Math.cos(theta_hypo) + d_hypo * Math.cos((R_hypo - r_hypo) / r_hypo * theta_hypo);
      y = (R_hypo - r_hypo) * Math.sin(theta_hypo) - d_hypo * Math.sin((R_hypo - r_hypo) / r_hypo * theta_hypo);
      break;
        
    case 'starfield': // Star Pattern
      const numPoints = 12;
      const progress = (t / 10) % (2 * Math.PI * numPoints);
      const pointIndex = Math.floor(progress / (2 * Math.PI));
      const pointAngle = pointIndex * 2 * Math.PI / numPoints;
      const nextAngle = ((pointIndex + 1) % numPoints) * 2 * Math.PI / numPoints;
      const blend = (progress % (2 * Math.PI)) / (2 * Math.PI);
      const r1 = maxRadius * 0.9;
      const r2 = maxRadius * 0.9;
      x = r1 * Math.cos(pointAngle) * (1 - blend) + r2 * Math.cos(nextAngle) * blend;
      y = r1 * Math.sin(pointAngle) * (1 - blend) + r2 * Math.sin(nextAngle) * blend;
      break;
        
    case 'circles': // Expanding Circles
      const circleNum = Math.floor(t / (2 * Math.PI * 50));
      const circleT = (t % (2 * Math.PI * 50)) / 50;
      const circleR = Math.min(maxRadius * 0.12 + circleNum * maxRadius * 0.12, maxRadius);
      x = circleR * Math.cos(circleT);
      y = circleR * Math.sin(circleT);
      break;
        
    case 'spiral3d': // 3D-looking Spiral
      const theta_3d = t / 20;
      const r_3d = Math.min(maxRadius * 0.04 + theta_3d * maxRadius * 0.012, maxRadius);
      const modulation = 1 + 0.3 * Math.sin(theta_3d * 5);
      x = r_3d * Math.cos(theta_3d) * modulation;
      y = r_3d * Math.sin(theta_3d) * modulation;
      break;
        
    default:
      x = 0;
      y = 0;
  }
  
  return { x, y };
}

export const PATTERN_OPTIONS: Array<{value: PatternType, label: string}> = [
  { value: 'none', label: 'None (Base State)' },
  { value: 'spiral', label: 'Archimedean Spiral' },
  { value: 'logSpiral', label: 'Logarithmic Spiral' },
  { value: 'fermatSpiral', label: "Fermat's Spiral" },
  { value: 'rose', label: 'Rose Curve' },
  { value: 'lissajous', label: 'Lissajous Curve' },
  { value: 'epitrochoid', label: 'Epitrochoid' },
  { value: 'hypotrochoid', label: 'Hypotrochoid' },
  { value: 'starfield', label: 'Star Pattern' },
  { value: 'circles', label: 'Expanding Circles' },
  { value: 'spiral3d', label: '3D Spiral' },
];
