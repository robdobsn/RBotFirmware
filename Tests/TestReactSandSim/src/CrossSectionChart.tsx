// Cross-section visualization component for sand height profile

import React, { useRef, useEffect } from 'react';
import { Point } from './types';

export interface CrossSectionData {
  position: number;  // 0-1 (normalized position along line)
  height: number;     // Sand height at this position
  distance: number;   // Distance from start in table coordinates
}

export interface CrossSectionChartProps {
  data: CrossSectionData[];
  startPoint: Point;
  endPoint: Point;
  maxHeight: number;
  startLevel: number;
  width?: number;
  height?: number;
}

export const CrossSectionChart: React.FC<CrossSectionChartProps> = ({
  data,
  startPoint,
  endPoint,
  maxHeight,
  startLevel,
  width = 800,
  height = 200,
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || data.length === 0) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    // Clear canvas
    ctx.fillStyle = '#1a1a1a';
    ctx.fillRect(0, 0, width, height);

    // Setup padding and dimensions
    const padding = { top: 20, right: 30, bottom: 40, left: 50 };
    const chartWidth = width - padding.left - padding.right;
    const chartHeight = height - padding.top - padding.bottom;

    // Draw background grid
    ctx.strokeStyle = '#333';
    ctx.lineWidth = 1;

    // Horizontal grid lines (height levels)
    const numHLines = 5;
    for (let i = 0; i <= numHLines; i++) {
      const y = padding.top + (chartHeight * i) / numHLines;
      ctx.beginPath();
      ctx.moveTo(padding.left, y);
      ctx.lineTo(padding.left + chartWidth, y);
      ctx.stroke();

      // Y-axis labels (height values)
      const heightValue = maxHeight * (1 - i / numHLines);
      ctx.fillStyle = '#888';
      ctx.font = '11px monospace';
      ctx.textAlign = 'right';
      ctx.fillText(heightValue.toFixed(1), padding.left - 5, y + 4);
    }

    // Vertical grid lines (distance)
    const numVLines = 8;
    for (let i = 0; i <= numVLines; i++) {
      const x = padding.left + (chartWidth * i) / numVLines;
      ctx.strokeStyle = '#333';
      ctx.beginPath();
      ctx.moveTo(x, padding.top);
      ctx.lineTo(x, padding.top + chartHeight);
      ctx.stroke();

      // X-axis labels (distance)
      if (data.length > 0) {
        const totalDist = data[data.length - 1].distance;
        const distValue = (totalDist * i) / numVLines;
        ctx.fillStyle = '#888';
        ctx.font = '11px monospace';
        ctx.textAlign = 'center';
        ctx.fillText(distValue.toFixed(0), x, height - 10);
      }
    }

    // Draw start level reference line
    const startY = padding.top + chartHeight * (1 - startLevel / maxHeight);
    ctx.strokeStyle = '#4CAF50';
    ctx.lineWidth = 1;
    ctx.setLineDash([5, 5]);
    ctx.beginPath();
    ctx.moveTo(padding.left, startY);
    ctx.lineTo(padding.left + chartWidth, startY);
    ctx.stroke();
    ctx.setLineDash([]);

    // Label for start level
    ctx.fillStyle = '#4CAF50';
    ctx.font = '11px monospace';
    ctx.textAlign = 'left';
    ctx.fillText(`Start: ${startLevel}`, padding.left + 5, startY - 5);

    // Draw sand height profile
    if (data.length > 1) {
      const maxDist = data[data.length - 1].distance;

      // Fill area under curve
      ctx.fillStyle = 'rgba(210, 180, 140, 0.3)';
      ctx.beginPath();
      ctx.moveTo(padding.left, padding.top + chartHeight);
      
      data.forEach((point) => {
        const x = padding.left + (point.distance / maxDist) * chartWidth;
        const y = padding.top + chartHeight * (1 - point.height / maxHeight);
        ctx.lineTo(x, y);
      });
      
      ctx.lineTo(padding.left + chartWidth, padding.top + chartHeight);
      ctx.closePath();
      ctx.fill();

      // Draw line
      ctx.strokeStyle = '#D2B48C';
      ctx.lineWidth = 2;
      ctx.beginPath();
      
      data.forEach((point, idx) => {
        const x = padding.left + (point.distance / maxDist) * chartWidth;
        const y = padding.top + chartHeight * (1 - point.height / maxHeight);
        
        if (idx === 0) {
          ctx.moveTo(x, y);
        } else {
          ctx.lineTo(x, y);
        }
      });
      
      ctx.stroke();

      // Draw min/max markers
      const minPoint = data.reduce((min, p) => p.height < min.height ? p : min, data[0]);
      const maxPoint = data.reduce((max, p) => p.height > max.height ? p : max, data[0]);

      // Max marker (red)
      const maxX = padding.left + (maxPoint.distance / maxDist) * chartWidth;
      const maxY = padding.top + chartHeight * (1 - maxPoint.height / maxHeight);
      ctx.fillStyle = '#ff4444';
      ctx.beginPath();
      ctx.arc(maxX, maxY, 4, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = '#ff4444';
      ctx.font = 'bold 11px monospace';
      ctx.textAlign = 'center';
      ctx.fillText(`Max: ${maxPoint.height.toFixed(2)}`, maxX, maxY - 10);

      // Min marker (blue)
      const minX = padding.left + (minPoint.distance / maxDist) * chartWidth;
      const minY = padding.top + chartHeight * (1 - minPoint.height / maxHeight);
      ctx.fillStyle = '#4444ff';
      ctx.beginPath();
      ctx.arc(minX, minY, 4, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = '#4444ff';
      ctx.font = 'bold 11px monospace';
      ctx.textAlign = 'center';
      ctx.fillText(`Min: ${minPoint.height.toFixed(2)}`, minX, minY + 15);
    }

    // Draw axes
    ctx.strokeStyle = '#666';
    ctx.lineWidth = 2;
    ctx.beginPath();
    // Y-axis
    ctx.moveTo(padding.left, padding.top);
    ctx.lineTo(padding.left, padding.top + chartHeight);
    // X-axis
    ctx.lineTo(padding.left + chartWidth, padding.top + chartHeight);
    ctx.stroke();

    // Axis labels
    ctx.fillStyle = '#aaa';
    ctx.font = 'bold 13px sans-serif';
    
    // Y-axis label
    ctx.save();
    ctx.translate(15, height / 2);
    ctx.rotate(-Math.PI / 2);
    ctx.textAlign = 'center';
    ctx.fillText('Sand Height', 0, 0);
    ctx.restore();

    // X-axis label
    ctx.textAlign = 'center';
    ctx.fillText('Distance', width / 2, height - 5);

    // Title
    ctx.fillStyle = '#fff';
    ctx.font = 'bold 14px sans-serif';
    ctx.textAlign = 'center';
    const totalDist = data.length > 0 ? data[data.length - 1].distance.toFixed(1) : '0';
    ctx.fillText(`Cross Section (${data.length} samples, ${totalDist} units)`, width / 2, 15);

  }, [data, maxHeight, startLevel, width, height]);

  return (
    <div style={{ marginTop: '10px' }}>
      <canvas
        ref={canvasRef}
        width={width}
        height={height}
        style={{
          border: '2px solid #333',
          borderRadius: '8px',
          backgroundColor: '#1a1a1a',
        }}
      />
    </div>
  );
};
