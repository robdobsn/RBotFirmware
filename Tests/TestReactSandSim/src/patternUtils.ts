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
      
    case 'horizontalLine': // Single horizontal pass for testing (10% margins)
      // Line from -80% to +80% of maxRadius (leaves 10% margins on each side)
      const lineStart = -maxRadius * 0.8;
      const lineEnd = maxRadius * 0.8;
      const lineLength = lineEnd - lineStart;
      
      // Map t to position along line (stop at end, don't loop)
      if (t <= lineLength) {
        x = lineStart + t;
        y = 0; // Centered horizontally
      } else {
        // Stay at end position
        x = lineEnd;
        y = 0;
      }
      break;
      
    case 'spiral': // Archimedean Spiral (outward then inward, repeating)
      {
        // Total angle for one outward pass
        const spiralSpacing = maxRadius * 0.06; // gap between loops
        const maxTheta = maxRadius * 0.92 / spiralSpacing; // angle at which radius reaches max
        const fullCycleTheta = maxTheta * 2; // out + back
        const theta_sp = (t / 20) % fullCycleTheta;
        let spiralR: number;
        if (theta_sp < maxTheta) {
          // Spiraling outward
          spiralR = theta_sp * spiralSpacing;
        } else {
          // Spiraling inward
          spiralR = (fullCycleTheta - theta_sp) * spiralSpacing;
        }
        x = spiralR * Math.sin(t / 20);
        y = spiralR * Math.cos(t / 20);
      }
      break;
        
    case 'logSpiral': // Logarithmic Spiral
      {
        const a_log = maxRadius * 0.01;
        const b_log = 0.08;
        const theta_log = t / 30;
        // Cycle: grow then shrink using triangle wave on exponent
        const cycleLen = Math.log(maxRadius / a_log) / b_log; // theta at which r = maxRadius
        const thetaMod = theta_log % (cycleLen * 2);
        const effectiveTheta = thetaMod < cycleLen ? thetaMod : cycleLen * 2 - thetaMod;
        const r_log = a_log * Math.exp(b_log * effectiveTheta);
        x = r_log * Math.cos(theta_log);
        y = r_log * Math.sin(theta_log);
      }
      break;
        
    case 'fermatSpiral': // Fermat's Spiral (outward then inward)
      {
        const angle_f = t / 15;
        const maxAngle_f = (maxRadius / (maxRadius * 0.06)) ** 2; // angle at which radius = maxRadius
        const cycleMod = angle_f % (maxAngle_f * 2);
        const effectiveAngle = cycleMod < maxAngle_f ? cycleMod : maxAngle_f * 2 - cycleMod;
        const radius_f = maxRadius * 0.06 * Math.sqrt(effectiveAngle);
        x = radius_f * Math.cos(angle_f);
        y = radius_f * Math.sin(angle_f);
      }
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
        
    case 'circles': // Expanding Circles (multiple radii, cycling)
      {
        const maxCircles = 8;
        const circlePhase = t / (2 * Math.PI * 50);
        const circleIdx = Math.floor(circlePhase) % (maxCircles * 2);
        const circleAngle = (t % (2 * Math.PI * 50)) / 50;
        // Triangle wave: expand out then contract back
        const effectiveIdx = circleIdx < maxCircles ? circleIdx : maxCircles * 2 - circleIdx;
        const circleRadius = maxRadius * 0.1 + effectiveIdx * maxRadius * 0.1;
        x = circleRadius * Math.cos(circleAngle);
        y = circleRadius * Math.sin(circleAngle);
      }
      break;
        
    case 'spiral3d': // 3D-looking Spiral (outward then inward with wobble)
      {
        const theta3d = t / 20;
        const maxR3d = maxRadius * 0.9;
        const growRate = maxRadius * 0.012;
        const maxTheta3d = maxR3d / growRate; // theta at which radius = max
        const cycleMod3d = theta3d % (maxTheta3d * 2);
        const effectiveTheta3d = cycleMod3d < maxTheta3d ? cycleMod3d : maxTheta3d * 2 - cycleMod3d;
        const r3d = maxRadius * 0.04 + effectiveTheta3d * growRate;
        const modulation3d = 1 + 0.3 * Math.sin(theta3d * 5);
        x = r3d * Math.cos(theta3d) * modulation3d;
        y = r3d * Math.sin(theta3d) * modulation3d;
      }
      break;
        
    default:
      x = 0;
      y = 0;
  }
  
  return { x, y };
}

export const PATTERN_OPTIONS: Array<{value: PatternType, label: string}> = [
  { value: 'none', label: 'None (Base State)' },
  { value: 'horizontalLine', label: 'Horizontal Line' },
  { value: 'thr', label: 'THR Track File' },
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
