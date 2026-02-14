# Sand Simulation Architecture - Technical Deep Dive

## Overview
This document provides a detailed technical analysis of the current sand table simulation implementation, describing the data structures, algorithms, and rendering techniques at the lowest level.

## Architecture Components

The simulation is structured in three main layers:

1. **SandKernel** - Core data storage and physics
2. **SandTableSim** - Ball movement and sand interaction
3. **Rendering Pipeline** - Visualization with lighting

---

## 1. Data Structures

### Primary Storage: Float32Array

**Location:** `SandKernel.sandLevel`

```typescript
private sandLevel: Float32Array;
```

- **Type:** JavaScript `Float32Array` (32-bit floating point)
- **Size:** `tableSize × tableSize` elements (default: 200 × 200 = 40,000 elements)
- **Memory:** 160 KB for 200×200 grid (4 bytes per float)
- **Layout:** Row-major 1D array representing 2D grid
- **Index Calculation:** `idx = y * tableSize + x`

**Characteristics:**
- Sub-pixel precision for smooth gradients
- Fixed memory layout (no dynamic allocation during runtime)
- Direct memory access (no bounds checking overhead in inner loops)
- Efficient cache locality for sequential access patterns

### State Tracking

**Ball Position State:**
```typescript
private currentPos: Point;  // Current ball position
private targetPos: Point;   // Destination from pattern
private prevPos: Point;     // Previous position (for velocity)
```

**Physics Parameters:**
```typescript
private ballDiameter: number;  // Collision radius (default: 6)
private moveSpeed: number;     // Movement rate (default: 1.5)
```

---

## 2. Sand Level Physics

### Initialization

```typescript
for (let i = 0; i < this.sandLevel.length; i++) {
  this.sandLevel[i] = sandStartLevel;  // Default: 5
}
```

- Creates uniform flat surface at height 5 (out of max 20)
- Provides 15 units of vertical range for displacement

### Sand Modification API

#### getSandLevel(x, y)
- **Input:** Floating point coordinates
- **Process:** `Math.floor()` to convert to integer indices
- **Bounds:** Returns 0 for out-of-bounds
- **Complexity:** O(1)

#### addSand(x, y, amount)
- **Input:** Position + delta amount (can be negative)
- **Process:**
  1. Convert float coordinates to integers
  2. Calculate flat array index
  3. Add amount to current level
  4. Clamp to `maxSandLevel` (20)
- **Complexity:** O(1)

**Key Characteristic:** No lower bound clamping - sand can go below `sandStartLevel`, creating deep troughs.

---

## 3. Ball Movement & Velocity Tracking

### Position Update Algorithm

**Every frame in `SandTableSim.update()`:**

```typescript
// 1. Store previous position
prevPos.x = currentPos.x;
prevPos.y = currentPos.y;

// 2. Calculate direction to target
const dx = targetPos.x - currentPos.x;
const dy = targetPos.y - currentPos.y;
const dist = Math.sqrt(dx * dx + dy * dy);

// 3. Move towards target with speed limiting
if (dist > moveSpeed) {
  currentPos.x += (dx / dist) * moveSpeed;
  currentPos.y += (dy / dist) * moveSpeed;
} else {
  currentPos.x = targetPos.x;
  currentPos.y = targetPos.y;
}
```

**Movement Characteristics:**
- **Speed Control:** Limited to `moveSpeed` units per frame
- **Smooth Pursuit:** Asymptotic approach to target
- **Direction:** Straight line interpolation
- **Frame Rate Dependency:** Movement is frame-rate dependent (not time-based)

### Velocity Calculation

```typescript
const velX = currentPos.x - prevPos.x;
const velY = currentPos.y - prevPos.y;
const velMag = Math.sqrt(velX * velX + velY * velY);

// Normalize
const velNormX = velMag > 0.001 ? velX / velMag : 0;
const velNormY = velMag > 0.001 ? velY / velMag : 0;

// Perpendicular direction (90° rotation)
const perpX = -velNormY;
const perpY = velNormX;
```

**Purpose:** 
- Determines sand displacement direction
- Creates realistic ridge formation perpendicular to motion
- Epsilon check (0.001) handles stationary ball

---

## 4. Sand Displacement Physics Model

### Two-Zone Displacement System

The ball affects sand in two concentric zones:

#### Zone 1: Center Trough (0 to 50% of radius)

```typescript
if (d < centerRadius) {  // centerRadius = ballDiameter / 4
  const troughFactor = 1 - (d / centerRadius);
  const sandAmount = -0.3 * troughFactor;
  kernel.addSand(posX, posY, sandAmount);
}
```

**Characteristics:**
- **Effect:** Removes sand (negative amount)
- **Magnitude:** Up to -0.3 at center, decreasing linearly to 0 at edge
- **Shape:** Conical removal pattern
- **Purpose:** Creates visible trough where ball passes

#### Zone 2: Outer Ridge (50% to 100% of radius)

```typescript
else {
  const sideFactor = (d - centerRadius) / (radius - centerRadius);
  const offsetDist = (1 - sideFactor) * 1.5;
  
  // Perpendicular displacement
  const displaceX = posX + perpX * offsetDist;
  const displaceY = posY + perpY * offsetDist;
  
  const sandAmount = 0.4 * (1 - d / radius);
  kernel.addSand(displaceX, displaceY, sandAmount);
  
  // Symmetric opposite side
  const displaceX2 = posX - perpX * offsetDist;
  const displaceY2 = posY - perpY * offsetDist;
  kernel.addSand(displaceX2, displaceY2, sandAmount * 0.8);
}
```

**Characteristics:**
- **Effect:** Adds sand at displaced locations
- **Direction:** Perpendicular to ball velocity
- **Symmetry:** Both sides of path (80% on opposite side)
- **Magnitude:** Up to 0.4 at inner edge, decreasing to 0 at outer edge
- **Offset:** Up to 1.5 units perpendicular displacement

### Algorithmic Structure

```
For each frame:
  For dy in [-radius, +radius]:
    For dx in [-radius, +radius]:
      d = sqrt(dx² + dy²)
      If d <= radius:
        posX = currentPos.x + dx
        posY = currentPos.y + dy
        [Apply zone-based displacement]
```

**Complexity:** O(ballDiameter²) per frame
- For diameter=6: ~113 operations per frame
- For diameter=10: ~314 operations per frame

**Mass Conservation:** NOT conserved - sand is created/destroyed
- Total sand removed per frame: varies with ball speed
- Total sand added per frame: varies with ball speed
- Net effect: Gradual accumulation of "ridges" over time

---

## 5. Settlement Algorithm

### Trigger Mechanism

```typescript
if (Math.random() < 0.02) {  // 2% probability per frame
  kernel.settle();
}
```

**Frequency:** Stochastic, approximately every 50 frames
- At 60 FPS: ~1.2 times per second
- At 30 FPS: ~0.6 times per second

### Diffusion-Based Smoothing

```typescript
settle(): void {
  const newLevel = new Float32Array(this.sandLevel);
  
  for (let y = 1; y < tableSize - 1; y++) {
    for (let x = 1; x < tableSize - 1; x++) {
      const level = sandLevel[idx];
      
      // Sample 4-connected neighbors
      const neighbors = [
        sandLevel[(y-1) * tableSize + x],  // North
        sandLevel[(y+1) * tableSize + x],  // South
        sandLevel[y * tableSize + (x-1)],  // West
        sandLevel[y * tableSize + (x+1)],  // East
      ];
      
      const avgNeighbor = sum(neighbors) / 4;
      
      // Conditional smoothing
      if (abs(level - avgNeighbor) > 1.5) {
        newLevel[idx] = level * 0.98 + avgNeighbor * 0.02;
      }
    }
  }
  
  sandLevel = newLevel;
}
```

**Algorithm Type:** Explicit heat diffusion (Jacobi iteration)

**Parameters:**
- **Threshold:** 1.5 units - only smooth significant differences
- **Blend Factor:** 0.02 (2% neighbor influence)
- **Preservation:** 0.98 (98% original value retained)

**Characteristics:**
- **Boundary Handling:** 1-pixel border untouched (static boundary)
- **Stencil:** Von Neumann (4-connected)
- **Iterations:** Single pass per settle call
- **Memory:** Requires full copy of grid (160 KB for 200×200)
- **Complexity:** O(tableSize²) when triggered

**Physical Model:** 
- Approximates granular flow over slope threshold
- Does NOT model gravity or avalanche dynamics
- Simple diffusion-based smoothing

---

## 6. Rendering Pipeline

### Pixel-by-Pixel Rasterization

```typescript
for (let py = 0; py < canvasHeight; py++) {
  for (let px = 0; px < canvasWidth; px++) {
    // Transform canvas space → table space
    const tx = ((px - canvasWidth/2) / canvasWidth) * tableSize + tableSize/2;
    const ty = ((py - canvasHeight/2) / canvasHeight) * tableSize + tableSize/2;
    
    // Sample sand level
    const level = kernel.getSandLevel(tx, ty);
    
    // Calculate lighting
    // Map to color
    // Write to imageData
  }
}
```

**Complexity:** O(canvasWidth × canvasHeight)
- For 600×600 canvas: 360,000 pixel evaluations per frame
- No spatial optimization (evaluates every pixel)

### Coordinate Transform

**Canvas Space:** (0, 0) at top-left corner
**Table Space:** (tableSize/2, tableSize/2) at center

```
table_x = (canvas_x - canvasWidth/2) / canvasWidth * tableSize + tableSize/2
```

**Inverse mapping:** Canvas pixels can sample between grid points (sub-pixel sampling)

### Circular Boundary Masking

```typescript
const dx = tx - tableSize / 2;
const dy = ty - tableSize / 2;
const dist = sqrt(dx² + dy²);

if (dist > tableSize / 2) {
  // Black pixel
}
```

**Shape:** Perfect circle inscribed in square grid
**Cost:** One sqrt() per pixel

### Slope-Based Lighting (Finite Difference)

```typescript
// Sample neighbors
levelRight = getSandLevel(tx + 1, ty);
levelLeft  = getSandLevel(tx - 1, ty);
levelDown  = getSandLevel(tx, ty + 1);
levelUp    = getSandLevel(tx, ty - 1);

// Central difference gradient
slopeDx = levelRight - levelLeft;  // df/dx
slopeDy = levelDown - levelUp;     // df/dy

// Light direction
lightDirX = -0.7;
lightDirY = -0.7;

// Dot product (directional derivative)
slope = slopeDx * lightDirX + slopeDy * lightDirY;
```

**Method:** Central finite difference
**Spacing:** h = 1 (grid unit)
**Gradient:** Approximates ∇f = (∂f/∂x, ∂f/∂y)

### Lighting Model

```typescript
if (slope > 0) {
  lightingFactor = 1.0 + slope * 1.1;   // Surface facing light
} else {
  lightingFactor = 1.0 + slope * 2.0;   // Surface facing away
}

lightingFactor = clamp(lightingFactor, 0.4, 1.2);
```

**Type:** Lambertian-inspired (but not physically accurate)
**Range:** 0.4 (darkest) to 1.2 (brightest)
**Asymmetry:** Shadows darker than highlights (2.0 vs 1.1 multiplier)

### Color Mapping

```typescript
normalized = level / 20;  // Map to [0, 1]
colorIndex = floor(normalized * paletteLength);
baseRgb = palette[colorIndex];

// Apply lighting
finalRgb = {
  r: min(255, round(baseRgb.r * lightingFactor)),
  g: min(255, round(baseRgb.g * lightingFactor)),
  b: min(255, round(baseRgb.b * lightingFactor))
};
```

**Palette:** 30 discrete colors (linear interpolation between palette entries)
**Dynamic Range:** 0-20 mapped to 0-29 palette indices

---

## 7. Performance Characteristics

### Per-Frame Computational Cost

| Operation | Complexity | Typical Cost (200×200 grid, 600×600 canvas) |
|-----------|-----------|---------------------------------------------|
| Ball movement | O(1) | ~10 ops |
| Sand displacement | O(ballDiameter²) | ~113 ops (diameter=6) |
| Settlement (when triggered) | O(tableSize²) | 40,000 ops (2% of frames) |
| Rendering | O(canvasWidth × canvasHeight) | 360,000 ops |

**Total per frame:** ~360,123 operations + occasional settlement
**Bottleneck:** Rendering (pixel rasterization)

### Memory Usage

- **Sand grid:** 160 KB (Float32Array, 200×200)
- **Settlement buffer:** 160 KB (temporary copy)
- **ImageData:** 1.44 MB (600×600 RGBA)
- **Total:** ~1.76 MB active memory

### Frame Rate Dependency

**Current Issues:**
1. Ball movement distance = `moveSpeed` per frame (not per second)
2. Sand displacement amount = fixed per frame
3. Settlement probability = 2% per frame

**Result:** Simulation runs faster on high-refresh displays, slower on low-FPS

---

## 8. Limitations & Assumptions

### Physics Limitations

1. **No mass conservation:** Sand created/destroyed freely
2. **No gravity:** Heights don't affect flow
3. **No avalanche dynamics:** Slopes can be arbitrarily steep
4. **No momentum:** Sand instantly deposited/removed
5. **Stationary ball:** No displacement when ball stops
6. **Frame-rate dependent:** Inconsistent physics across devices

### Numerical Issues

1. **Coordinate precision:** Float32 has ~7 significant digits
   - At tableSize=200: precision ~0.00001 units
   - Sufficient for current simulation

2. **Gradient artifacts:** Central difference uses spacing=1
   - Single-pixel noise in sand creates large gradients
   - Visible as "sparkling" in lighting

3. **Settlement convergence:** Single iteration insufficient
   - True equilibrium never reached
   - Depends on stochastic timing

### Rendering Limitations

1. **No mipmapping:** Downsampling artifacts at distance
2. **Point sampling:** Aliasing on fine details
3. **No shadows:** Only local slope lighting
4. **Fixed light:** Single directional light, no ambient

---

## 9. Algorithm Pseudocode

### Complete Frame Update

```
FUNCTION update():
  // 1. Ball movement
  prevPos ← currentPos
  direction ← normalize(targetPos - currentPos)
  distance ← min(||targetPos - currentPos||, moveSpeed)
  currentPos ← currentPos + direction * distance
  
  // 2. Calculate velocity
  velocity ← currentPos - prevPos
  perpendicular ← rotate90(velocity)
  
  // 3. Sand displacement
  FOR each pixel in circle(currentPos, ballDiameter/2):
    d ← distance from currentPos
    
    IF d < ballDiameter/4:
      // Center trough
      amount ← -0.3 * (1 - d/(ballDiameter/4))
      addSand(pixel, amount)
    ELSE:
      // Side ridges
      offset ← (1 - sideFactor) * 1.5
      amount ← 0.4 * (1 - d/(ballDiameter/2))
      
      addSand(pixel + perpendicular*offset, amount)
      addSand(pixel - perpendicular*offset, amount * 0.8)
  
  // 4. Stochastic settlement
  IF random() < 0.02:
    settle()
```

### Settlement Algorithm

```
FUNCTION settle():
  newGrid ← copy(sandLevel)
  
  FOR y = 1 TO tableSize-2:
    FOR x = 1 TO tableSize-2:
      current ← sandLevel[x, y]
      avgNeighbor ← mean(sandLevel[x±1, y±1])
      
      IF |current - avgNeighbor| > 1.5:
        newGrid[x, y] ← current * 0.98 + avgNeighbor * 0.02
  
  sandLevel ← newGrid
```

### Rendering Algorithm

```
FUNCTION render():
  FOR py = 0 TO canvasHeight:
    FOR px = 0 TO canvasWidth:
      // 1. Transform coordinates
      (tx, ty) ← canvasToTable(px, py)
      
      // 2. Boundary check
      IF distance(tx, ty, center) > tableSize/2:
        pixel[px, py] ← BLACK
        CONTINUE
      
      // 3. Sample height and gradients
      level ← getSandLevel(tx, ty)
      gradX ← getSandLevel(tx+1, ty) - getSandLevel(tx-1, ty)
      gradY ← getSandLevel(tx, ty+1) - getSandLevel(tx, ty-1)
      
      // 4. Calculate lighting
      slope ← dot(gradient, lightDirection)
      lightFactor ← slope > 0 ? 1+slope*1.1 : 1+slope*2.0
      lightFactor ← clamp(lightFactor, 0.4, 1.2)
      
      // 5. Color mapping
      colorIndex ← floor(level / 20 * paletteSize)
      baseColor ← palette[colorIndex]
      finalColor ← baseColor * lightFactor
      
      pixel[px, py] ← finalColor
```

---

## 10. Data Flow Diagram

```
Pattern Generator
       ↓
   targetPos
       ↓
SandTableSim.update()
   ├→ Ball Movement (velocity calculation)
   ├→ Sand Displacement (modify SandKernel.sandLevel)
   └→ Stochastic Settlement (diffusion)
       ↓
   sandLevel (Float32Array)
       ↓
Rendering Pipeline
   ├→ Coordinate Transform
   ├→ Height Sampling
   ├→ Gradient Calculation
   ├→ Lighting Computation
   └→ Color Mapping
       ↓
   Canvas ImageData
       ↓
   Display
```

---

## 11. Key Tunable Parameters

| Parameter | Current Value | Effect | Trade-off |
|-----------|--------------|--------|-----------|
| `ballDiameter` | 6 | Displacement area size | Larger = more visible but less detail |
| `moveSpeed` | 1.5 | Ball velocity | Faster = smoother paths but less displacement |
| `centerRadius` | 3 (50% of diameter) | Trough size | Larger = deeper grooves |
| `troughFactor` | -0.3 | Sand removal rate | More negative = deeper troughs |
| `ridgeAmount` | 0.4 | Sand addition rate | Higher = taller ridges |
| `offsetDist` | 1.5 | Ridge displacement | Larger = wider apart ridges |
| `settleProbability` | 0.02 | Settlement frequency | Higher = smoother but less detail |
| `settleThreshold` | 1.5 | Smoothing trigger | Lower = more smoothing |
| `settleBlend` | 0.02 | Diffusion rate | Higher = faster smoothing |
| `lightingAsymmetry` | 1.1 / 2.0 | Shadow/highlight ratio | Higher = more dramatic |

---

## Conclusion

The current simulation uses a **grid-based heightfield approach** with:
- Fast O(1) sand access
- Simple displacement physics (two-zone model)
- Stochastic diffusion-based settlement
- Per-pixel rendering with slope-based lighting

**Strengths:**
- Simple, understandable implementation
- Good visual results with lighting
- Low memory footprint
- Easily tunable parameters

**Weaknesses:**
- No physical accuracy (mass, gravity, avalanche)
- Frame-rate dependent simulation
- Expensive pixel-by-pixel rendering
- Limited to 2D heightfield (no overhangs, tunnels)
- Settlement is costly when triggered

This architecture prioritizes **visual plausibility** over **physical accuracy**, making it suitable for artistic visualization but not for scientific simulation of granular materials.
