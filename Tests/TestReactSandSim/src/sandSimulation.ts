// Sand simulation physics and rendering logic

import { Point, SandSimulationOptions } from './types';

export class SandKernel {
  private tableSize: number;
  private sandLevel: Float32Array;
  private sandStartLevel: number;
  private maxSandLevel: number;
  
  constructor(tableSize: number, sandStartLevel: number, maxSandLevel: number) {
    this.tableSize = tableSize;
    this.sandStartLevel = sandStartLevel;
    this.maxSandLevel = maxSandLevel;
    this.sandLevel = new Float32Array(tableSize * tableSize);
    
    // Initialize sand level
    for (let i = 0; i < this.sandLevel.length; i++) {
      this.sandLevel[i] = sandStartLevel;
    }
  }
  
  getSandLevel(x: number, y: number): number {
    const ix = Math.floor(x);
    const iy = Math.floor(y);
    
    if (ix < 0 || ix >= this.tableSize || iy < 0 || iy >= this.tableSize) {
      return 0;
    }
    
    return this.sandLevel[iy * this.tableSize + ix];
  }
  
  addSand(x: number, y: number, amount: number): void {
    const ix = Math.floor(x);
    const iy = Math.floor(y);
    
    if (ix < 0 || ix >= this.tableSize || iy < 0 || iy >= this.tableSize) {
      return;
    }
    
    const idx = iy * this.tableSize + ix;
    this.sandLevel[idx] = Math.min(this.sandLevel[idx] + amount, this.maxSandLevel);
  }
  
  // Simulate sand settling
  settle(): void {
    const newLevel = new Float32Array(this.sandLevel);
    
    for (let y = 1; y < this.tableSize - 1; y++) {
      for (let x = 1; x < this.tableSize - 1; x++) {
        const idx = y * this.tableSize + x;
        const level = this.sandLevel[idx];
        
        // Check neighbors
        const neighbors = [
          this.sandLevel[(y-1) * this.tableSize + x],     // top
          this.sandLevel[(y+1) * this.tableSize + x],     // bottom
          this.sandLevel[y * this.tableSize + (x-1)],     // left
          this.sandLevel[y * this.tableSize + (x+1)],     // right
        ];
        
        const avgNeighbor = neighbors.reduce((a, b) => a + b, 0) / 4;
        
        // Gentler settling: only smooth large differences, preserve detail
        // Increased threshold from 0.5 to 1.5, reduced blend from 0.1 to 0.02
        if (Math.abs(level - avgNeighbor) > 1.5) {
          newLevel[idx] = level * 0.98 + avgNeighbor * 0.02;
        }
      }
    }
    
    this.sandLevel = newLevel;
  }
  
  reset(): void {
    for (let i = 0; i < this.sandLevel.length; i++) {
      this.sandLevel[i] = this.sandStartLevel;
    }
  }
  
  getSandLevelArray(): Float32Array {
    return this.sandLevel;
  }
  
  getTableSize(): number {
    return this.tableSize;
  }
}

export class SandTableSim {
  private kernel: SandKernel;
  private ballDiameter: number;
  private tableSize: number;
  private patternScale: number;
  private currentPos: Point;
  private targetPos: Point;
  private prevPos: Point;
  private moveSpeed: number;
  
  constructor(options: SandSimulationOptions) {
    this.ballDiameter = options.ballDiameter;
    this.tableSize = options.tableSize;
    this.patternScale = options.patternScale ?? 250;
    this.kernel = new SandKernel(
      options.tableSize,
      options.sandStartLevel,
      options.maxSandLevel
    );
    
    this.currentPos = { x: this.tableSize / 2, y: this.tableSize / 2 };
    this.targetPos = { x: this.tableSize / 2, y: this.tableSize / 2 };
    this.prevPos = { x: this.tableSize / 2, y: this.tableSize / 2 };
    this.moveSpeed = options.moveSpeed;
  }
  
  setTargetPosition(pos: Point): void {
    // Convert from pattern coordinates (-patternScale to +patternScale) to table coordinates (0 to tableSize)
    const scale = this.tableSize / (2 * this.patternScale);
    this.targetPos = {
      x: this.tableSize / 2 + pos.x * scale,
      y: this.tableSize / 2 + pos.y * scale
    };
  }
  
  getCurrentPosition(): Point {
    return this.currentPos;
  }
  
  update(): void {
    // Store previous position
    this.prevPos.x = this.currentPos.x;
    this.prevPos.y = this.currentPos.y;
    
    // Move current position towards target
    const dx = this.targetPos.x - this.currentPos.x;
    const dy = this.targetPos.y - this.currentPos.y;
    const dist = Math.sqrt(dx * dx + dy * dy);
    
    if (dist > this.moveSpeed) {
      this.currentPos.x += (dx / dist) * this.moveSpeed;
      this.currentPos.y += (dy / dist) * this.moveSpeed;
    } else {
      this.currentPos.x = this.targetPos.x;
      this.currentPos.y = this.targetPos.y;
    }
    
    // Calculate ball velocity direction for displacement
    const velX = this.currentPos.x - this.prevPos.x;
    const velY = this.currentPos.y - this.prevPos.y;
    const velMag = Math.sqrt(velX * velX + velY * velY);
    
    // Normalized velocity and perpendicular direction
    const velNormX = velMag > 0.001 ? velX / velMag : 0;
    const velNormY = velMag > 0.001 ? velY / velMag : 0;
    const perpX = -velNormY; // Perpendicular to velocity
    const perpY = velNormX;
    
    // Sand displacement physics - ball pushes sand to sides, creates trough in center
    const radius = this.ballDiameter / 2;
    const centerRadius = radius * 0.67; // Trough ~1/3 of ball diameter (~67% of radius)
    
    for (let dy = -radius; dy <= radius; dy++) {
      for (let dx = -radius; dx <= radius; dx++) {
        const d = Math.sqrt(dx * dx + dy * dy);
        if (d <= radius) {
          const posX = this.currentPos.x + dx;
          const posY = this.currentPos.y + dy;
          
          if (d < centerRadius) {
            // Center of ball path - create trough (width ~1/3 of ball diameter)
            const troughFactor = 1 - (d / centerRadius);
            const sandAmount = -1.8 * troughFactor; // Remove sand progressively
            this.kernel.addSand(posX, posY, sandAmount);
          } else {
            // Outer ring - push sand to sides
            // Keep ridges closer to trough for more concentrated effect
            const sideFactor = (d - centerRadius) / (radius - centerRadius);
            
            // Calculate displacement direction (perpendicular to movement)
            const offsetDist = (1 - sideFactor) * 1.2;  // Reduced from 2.0
            const displaceX = posX + perpX * offsetDist;
            const displaceY = posY + perpY * offsetDist;
            
            // Add sand at displaced position (creates ridges)
            const sandAmount = 1.2 * (1 - d / radius);
            this.kernel.addSand(displaceX, displaceY, sandAmount);
            
            // Also add on opposite side for symmetry
            const displaceX2 = posX - perpX * offsetDist;
            const displaceY2 = posY - perpY * offsetDist;
            this.kernel.addSand(displaceX2, displaceY2, sandAmount * 0.8);
          }
        }
      }
    }
    
    // Settle sand less frequently to preserve detail (0.5% = every 200 frames)
    if (Math.random() < 0.005) {
      this.kernel.settle();
    }
  }
  
  reset(): void {
    this.kernel.reset();
    this.currentPos = { x: this.tableSize / 2, y: this.tableSize / 2 };
    this.targetPos = { x: this.tableSize / 2, y: this.tableSize / 2 };
    this.prevPos = { x: this.tableSize / 2, y: this.tableSize / 2 };
  }
  
  getKernel(): SandKernel {
    return this.kernel;
  }
  
  getBallDiameter(): number {
    return this.ballDiameter;
  }
  
  setBallDiameter(diameter: number): void {
    this.ballDiameter = diameter;
  }
  
  getMoveSpeed(): number {
    return this.moveSpeed;
  }
  
  setMoveSpeed(speed: number): void {
    this.moveSpeed = speed;
  }
}
