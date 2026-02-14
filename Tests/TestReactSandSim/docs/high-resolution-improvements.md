# High-Resolution Sand Simulation - Technical Analysis

## Executive Summary

**Current State:** 200×200 grid (40K elements, 160 KB)  
**Target Goal:** ≥100 parallel traces with fine intra-trace resolution  
**Proposed Resolution:** 800×800 to 2000×2000 grid  
**Key Challenge:** Maintaining real-time performance at 4×-100× data volume

---

## 1. Resolution Analysis

### Current System Limitations

**Grid Resolution:** 200×200 = 40,000 cells
- **Samples per diameter:** ~3.3 (for ball diameter = 6 pixels)
- **Parallel traces:** 200 traces available (but ball spans 6, so effective ~33 traces)
- **Nyquist sampling:** Insufficient for sub-ball-diameter features

**Visual Quality Issues:**
1. **Blocky displacement:** 6×6 pixel ball affects only ~113 cells
2. **Coarse gradients:** Single-pixel height changes create visible steps
3. **Limited detail:** Fine patterns (spirals, dithering) lack definition
4. **Aliasing:** Undersampled features create visual artifacts

### Target Resolution Requirements

**Minimum for 100+ Parallel Traces:**

If we want 100 distinct parallel traces AND the ball has ~6-pixel diameter:
- **Minimum grid:** 600×600 (100 traces × 6 pixels/trace)
- **Memory:** 1.44 MB (9× current)

**Recommended for High Quality:**

For smooth, detailed simulation with 100+ traces:
- **Resolution:** 1000×1000
- **Ball diameter:** 10-12 pixels (scaled proportionally)
- **Samples per trace:** 10 pixels
- **Memory:** 4 MB (25× current)
- **Parallel traces:** 100+ with excellent intra-trace detail

**Maximum Practical Resolution:**

- **Resolution:** 2000×2000
- **Memory:** 16 MB (100× current)
- **Ball diameter:** 20-24 pixels
- **Visual quality:** Near-photorealistic
- **Challenge:** Processing 4,000,000 cells per frame

---

## 2. Storage Architecture Improvements

### Option A: Simple Upscaling (Baseline)

**Approach:** Replace `tableSize = 200` with `tableSize = 1000`

```typescript
// Current: 200×200
private sandLevel: Float32Array;  // 160 KB

// Upgrade: 1000×1000
private sandLevel: Float32Array;  // 4 MB
```

**Pros:**
- Minimal code changes
- 5× resolution improvement
- Still fits in L3 cache (typical ~8 MB)

**Cons:**
- 25× memory usage
- 625× slower settlement algorithm (O(n²))
- Still uses CPU-bound processing

**Implementation Complexity:** Low (change one constant)

### Option B: Sparse Grid with Active Region Tracking

**Approach:** Only store/process cells near recent ball positions

```typescript
interface SparseCell {
  x: number;
  y: number;
  level: number;
}

class SparseGrid {
  private activeCells: Map<string, number>;  // "x,y" -> level
  private activeRegion: BoundingBox;         // Updated each frame
  
  getCellKey(x: number, y: number): string {
    return `${Math.floor(x)},${Math.floor(y)}`;
  }
  
  setSandLevel(x: number, y: number, level: number) {
    if (level !== DEFAULT_LEVEL) {
      this.activeCells.set(this.getCellKey(x, y), level);
    } else {
      this.activeCells.delete(this.getCellKey(x, y));
    }
  }
}
```

**Characteristics:**
- **Memory:** Proportional to affected area only
- **Typical usage:** 10K-50K active cells (40-200 KB) for continuous drawing
- **Peak usage:** Up to full grid if entire table affected

**Pros:**
- Scales to arbitrary resolution (2000×2000+)
- Memory efficient for localized patterns
- Settlement only processes active cells
- Easy to extend to infinite grids

**Cons:**
- Hash map overhead (~32 bytes per cell vs 4 bytes)
- Slower random access (hash lookup vs direct indexing)
- More complex implementation

**Implementation Complexity:** Medium (rewrite storage layer)

### Option C: Hierarchical Multi-Resolution Grid

**Approach:** Store multiple resolution levels

```typescript
class HierarchicalGrid {
  private levels: Float32Array[];  // [2000×2000, 1000×1000, 500×500, 250×250]
  private levelSizes: number[];    // [2000, 1000, 500, 250]
  
  // Full detail near ball
  private highResCenter: Point;
  private highResRadius: number = 50;
  
  getSandLevel(x: number, y: number, requiredDetail: number): number {
    // Distance from ball
    const distFromBall = distance({x, y}, this.highResCenter);
    
    if (distFromBall < this.highResRadius) {
      // Use highest resolution (2000×2000)
      return this.levels[0][y * 2000 + x];
    } else if (distFromBall < 100) {
      // Use medium resolution (1000×1000)
      const mx = Math.floor(x / 2);
      const my = Math.floor(y / 2);
      return this.levels[1][my * 1000 + mx];
    } else {
      // Use low resolution (500×500)
      const lx = Math.floor(x / 4);
      const ly = Math.floor(y / 4);
      return this.levels[2][ly * 500 + lx];
    }
  }
}
```

**Memory Total:**
- Level 0 (2000×2000): 16 MB
- Level 1 (1000×1000): 4 MB
- Level 2 (500×500): 1 MB
- Level 3 (250×250): 256 KB
- **Total:** 21.25 MB

**Pros:**
- High detail where needed (near ball)
- Coarse detail for distant areas (saves processing)
- Smooth LOD transitions possible
- Excellent for large tables

**Cons:**
- Complex synchronization between levels
- Potential for visual artifacts at boundaries
- Higher total memory usage

**Implementation Complexity:** High (major architectural change)

### Option D: GPU-Accelerated Storage (WebGL/WebGPU)

**Approach:** Store sand grid in GPU texture

```typescript
// WebGL2 approach
class GPUSandGrid {
  private gl: WebGL2RenderingContext;
  private sandTexture: WebGLTexture;    // 2000×2000 R32F texture
  private framebuffer: WebGLFramebuffer;
  
  constructor(size: number) {
    // Create floating-point texture
    this.sandTexture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, this.sandTexture);
    gl.texImage2D(
      gl.TEXTURE_2D, 0, gl.R32F,
      size, size, 0,
      gl.RED, gl.FLOAT, null
    );
  }
  
  updateSand(ballPos: Point, ballRadius: number) {
    // Use fragment shader to displace sand
    gl.useProgram(this.displacementShader);
    gl.uniform2f(this.ballPosUniform, ballPos.x, ballPos.y);
    gl.uniform1f(this.ballRadiusUniform, ballRadius);
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
  }
}
```

**Fragment Shader (Sand Displacement):**
```glsl
#version 300 es
precision highp float;

uniform sampler2D u_sandLevel;
uniform vec2 u_ballPos;
uniform float u_ballRadius;
uniform vec2 u_ballVelocity;

in vec2 v_texCoord;
out float fragColor;

void main() {
  vec2 cellPos = v_texCoord * vec2(textureSize(u_sandLevel, 0));
  float dist = distance(cellPos, u_ballPos);
  float currentLevel = texture(u_sandLevel, v_texCoord).r;
  
  float newLevel = currentLevel;
  
  if (dist < u_ballRadius) {
    float centerRadius = u_ballRadius * 0.5;
    
    if (dist < centerRadius) {
      // Trough
      float factor = 1.0 - (dist / centerRadius);
      newLevel += -0.3 * factor;
    } else {
      // Ridges
      vec2 perpDir = vec2(-u_ballVelocity.y, u_ballVelocity.x);
      float sideFactor = (dist - centerRadius) / (u_ballRadius - centerRadius);
      // ... ridge displacement logic
      newLevel += 0.4 * (1.0 - dist / u_ballRadius);
    }
  }
  
  fragColor = newLevel;
}
```

**Pros:**
- Native 2D texture operations (highly optimized)
- Parallel processing of all cells
- Can handle 2000×2000+ at 60 FPS
- Settlement as blur shader (extremely fast)

**Cons:**
- Requires WebGL2 or WebGPU
- More complex debugging
- CPU-GPU data transfer overhead (if needed)
- Browser compatibility considerations

**Implementation Complexity:** High (complete rewrite)

---

## 3. Processing Algorithm Improvements

### Current Processing Bottlenecks

**Per-Frame Costs (200×200 grid):**

| Operation | Complexity | 200×200 | 1000×1000 | 2000×2000 |
|-----------|-----------|---------|-----------|-----------|
| Ball displacement | O(r²) | 113 ops | 314 ops | 1,257 ops |
| Settlement | O(n²) | 40K ops | 1M ops | 4M ops |
| Rendering | O(canvas²) | 360K ops | 360K ops | 360K ops |

**Key Insight:** Settlement scales quadratically with resolution!

### Improvement 1: Localized Settlement

**Current:** Process entire grid on settlement

```typescript
settle(): void {
  for (let y = 1; y < tableSize - 1; y++) {      // Full grid
    for (let x = 1; x < tableSize - 1; x++) {    // Full grid
      // Smoothing logic
    }
  }
}
```

**Improved:** Only settle near recent ball positions

```typescript
class SandKernel {
  private dirtyRegions: BoundingBox[] = [];  // Track modified regions
  
  addSand(x: number, y: number, amount: number): void {
    // ... existing code ...
    this.markDirty(x, y, radius = 10);
  }
  
  settleLocalized(): void {
    for (const region of this.dirtyRegions) {
      for (let y = region.minY; y < region.maxY; y++) {
        for (let x = region.minX; x < region.maxX; x++) {
          // Smoothing logic
        }
      }
    }
    this.dirtyRegions = [];  // Clear after processing
  }
}
```

**Performance Improvement:**
- 200×200 grid: 40K → ~500 ops (80× faster)
- 1000×1000 grid: 1M → ~1K ops (1000× faster)
- 2000×2000 grid: 4M → ~2K ops (2000× faster)

**Result:** Settlement no longer scales with grid size!

### Improvement 2: Multi-Threaded Processing (Web Workers)

**Approach:** Distribute sand processing across CPU cores

```typescript
class MultiThreadedSandKernel {
  private workers: Worker[];
  private sharedBuffer: SharedArrayBuffer;
  
  constructor(tableSize: number, numThreads: number = 4) {
    // Allocate shared memory
    this.sharedBuffer = new SharedArrayBuffer(
      tableSize * tableSize * 4  // Float32 = 4 bytes
    );
    
    // Spawn workers
    for (let i = 0; i < numThreads; i++) {
      this.workers[i] = new Worker('sandWorker.js');
      this.workers[i].postMessage({
        type: 'init',
        buffer: this.sharedBuffer,
        tableSize: tableSize,
        rowStart: Math.floor(i * tableSize / numThreads),
        rowEnd: Math.floor((i + 1) * tableSize / numThreads)
      });
    }
  }
  
  settle(): Promise<void> {
    // Dispatch settlement to all workers
    const promises = this.workers.map(w => 
      new Promise(resolve => {
        w.onmessage = () => resolve();
        w.postMessage({ type: 'settle' });
      })
    );
    return Promise.all(promises);
  }
}
```

**sandWorker.js:**
```javascript
let sandLevel, tableSize, rowStart, rowEnd;

onmessage = (e) => {
  if (e.data.type === 'init') {
    sandLevel = new Float32Array(e.data.buffer);
    tableSize = e.data.tableSize;
    rowStart = e.data.rowStart;
    rowEnd = e.data.rowEnd;
  }
  
  if (e.data.type === 'settle') {
    // Process assigned rows only
    for (let y = rowStart; y < rowEnd; y++) {
      for (let x = 1; x < tableSize - 1; x++) {
        // Settlement logic
      }
    }
    postMessage({ type: 'done' });
  }
};
```

**Performance:**
- 4 cores: ~3.5× faster settlement
- 8 cores: ~6× faster settlement
- Scales linearly with core count

**Cons:**
- Worker overhead (~1-2ms spawn time)
- Shared memory race conditions (need synchronization)
- Not beneficial for small grids (<500×500)

### Improvement 3: Incremental Settlement (Time-Slicing)

**Approach:** Spread settlement over multiple frames

```typescript
class IncrementalSettlement {
  private settlementQueue: {x: number, y: number}[] = [];
  private settlementIndex: number = 0;
  private cellsPerFrame: number = 5000;  // Budget
  
  queueSettlement(region: BoundingBox): void {
    for (let y = region.minY; y < region.maxY; y++) {
      for (let x = region.minX; x < region.maxX; x++) {
        this.settlementQueue.push({x, y});
      }
    }
  }
  
  processIncrementalSettlement(): void {
    const endIndex = Math.min(
      this.settlementIndex + this.cellsPerFrame,
      this.settlementQueue.length
    );
    
    for (let i = this.settlementIndex; i < endIndex; i++) {
      const {x, y} = this.settlementQueue[i];
      // Smooth this cell
      this.smoothCell(x, y);
    }
    
    this.settlementIndex = endIndex;
    
    if (this.settlementIndex >= this.settlementQueue.length) {
      // Done with current batch
      this.settlementQueue = [];
      this.settlementIndex = 0;
    }
  }
}
```

**Benefits:**
- Consistent frame times (no stuttering)
- Can process enormous grids incrementally
- User doesn't notice multi-frame settlement

**Trade-off:**
- Settlement lags behind ball by several frames
- May allow temporary unrealistic states

### Improvement 4: Optimized Displacement Algorithm

**Current:** Circular iteration with distance checks

```typescript
// Current: O(π * r²) with distance calculation per cell
for (let dy = -radius; dy <= radius; dy++) {
  for (let dx = -radius; dx <= radius; dx++) {
    const d = Math.sqrt(dx * dx + dy * dy);
    if (d <= radius) {
      // Displace sand
    }
  }
}
```

**Optimized:** Bresenham-style circle fill

```typescript
// Optimized: Iterate only cells inside circle
displaceCircle(centerX: number, centerY: number, radius: number): void {
  const r2 = radius * radius;
  
  for (let y = -radius; y <= radius; y++) {
    // Calculate x extent for this y
    const xExtent = Math.sqrt(r2 - y * y);
    const xStart = Math.floor(centerX - xExtent);
    const xEnd = Math.ceil(centerX + xExtent);
    
    for (let x = xStart; x <= xEnd; x++) {
      const dx = x - centerX;
      const dy = y - (centerY);
      const d = Math.sqrt(dx * dx + dy * dy);
      
      // Displace sand at (x, centerY + y)
      this.displaceSandAtDistance(x, centerY + y, d, radius);
    }
  }
}
```

**Performance:** ~30% faster (fewer distance calculations)

**Alternative:** Pre-computed displacement stamps

```typescript
class DisplacementStampCache {
  private stamps: Map<number, DisplacementStamp>;
  
  getStamp(diameter: number, velocity: Vector2): DisplacementStamp {
    const key = `${diameter}_${velocity.x.toFixed(2)}_${velocity.y.toFixed(2)}`;
    
    if (!this.stamps.has(key)) {
      this.stamps.set(key, this.computeStamp(diameter, velocity));
    }
    
    return this.stamps.get(key);
  }
  
  private computeStamp(diameter: number, velocity: Vector2): DisplacementStamp {
    // Pre-calculate displacement for all cells in circle
    const radius = diameter / 2;
    const cells: {dx: number, dy: number, amount: number}[] = [];
    
    for (let y = -radius; y <= radius; y++) {
      for (let x = -radius; x <= radius; x++) {
        const d = Math.sqrt(x * x + y * y);
        if (d <= radius) {
          const amount = this.calculateDisplacement(x, y, d, radius, velocity);
          cells.push({dx: x, dy: y, amount});
        }
      }
    }
    
    return { cells, diameter };
  }
}

// Usage:
const stamp = stampCache.getStamp(ballDiameter, velocity);
for (const {dx, dy, amount} of stamp.cells) {
  kernel.addSand(ballPos.x + dx, ballPos.y + dy, amount);
}
```

**Benefits:**
- 10× faster for repeated velocities
- Especially good for parametric patterns (repeated angles)

---

## 4. Rendering Optimizations

### Current Bottleneck

**Per-Frame Cost:** O(canvasWidth × canvasHeight)
- 600×600 canvas = 360,000 pixel evaluations
- Each pixel: 1 sand sample + 4 neighbor samples + lighting calculation

**Rendering is resolution-independent** (doesn't care about grid size)

### Optimization 1: Render to Lower Resolution, Upscale

**Approach:**
```typescript
// Render to 300×300 off-screen canvas
const renderCanvas = document.createElement('canvas');
renderCanvas.width = 300;
renderCanvas.height = 300;

// Render sand
renderSand(renderCtx, simulation, colorPalette, 300, 300);

// Upscale to 600×600 with smoothing
displayCtx.imageSmoothingEnabled = true;
displayCtx.imageSmoothingQuality = 'high';
displayCtx.drawImage(renderCanvas, 0, 0, 300, 300, 0, 0, 600, 600);
```

**Performance:** 4× faster rendering (300² vs 600² pixels)
**Quality:** Minimal loss (native browser upscaling is excellent)

### Optimization 2: Dirty Rectangle Rendering

**Approach:** Only re-render regions that changed

```typescript
class DirtyRectRenderer {
  private dirtyRegions: Rectangle[] = [];
  private previousFrame: ImageData;
  
  markDirty(region: Rectangle): void {
    this.dirtyRegions.push(region);
  }
  
  render(): void {
    if (this.dirtyRegions.length === 0) {
      // No changes, re-use previous frame
      ctx.putImageData(this.previousFrame, 0, 0);
      return;
    }
    
    // Render only dirty regions
    for (const region of this.dirtyRegions) {
      this.renderRegion(region);
    }
    
    this.previousFrame = ctx.getImageData(0, 0, width, height);
    this.dirtyRegions = [];
  }
}
```

**Performance:** 10-50× faster (only renders ~10% of screen)

### Optimization 3: WebGL Rendering

**Approach:** GPU-accelerated shader rendering

```glsl
// Vertex shader
attribute vec2 a_position;
varying vec2 v_texCoord;

void main() {
  gl_Position = vec4(a_position, 0.0, 1.0);
  v_texCoord = a_position * 0.5 + 0.5;
}

// Fragment shader
precision highp float;
uniform sampler2D u_sandLevel;
uniform sampler2D u_colorPalette;
varying vec2 v_texCoord;

void main() {
  // Sample sand level from texture
  float level = texture2D(u_sandLevel, v_texCoord).r;
  
  // Calculate gradients
  float levelRight = texture2D(u_sandLevel, v_texCoord + vec2(1.0/2000.0, 0.0)).r;
  float levelLeft = texture2D(u_sandLevel, v_texCoord - vec2(1.0/2000.0, 0.0)).r;
  float levelDown = texture2D(u_sandLevel, v_texCoord + vec2(0.0, 1.0/2000.0)).r;
  float levelUp = texture2D(u_sandLevel, v_texCoord - vec2(0.0, 1.0/2000.0)).r;
  
  vec2 gradient = vec2(levelRight - levelLeft, levelDown - levelUp);
  
  // Lighting
  vec2 lightDir = normalize(vec2(-0.7, -0.7));
  float slope = dot(gradient, lightDir);
  float lighting = slope > 0.0 ? 1.0 + slope * 1.1 : 1.0 + slope * 2.0;
  lighting = clamp(lighting, 0.4, 1.2);
  
  // Color lookup
  float colorIndex = level / 20.0;
  vec3 color = texture2D(u_colorPalette, vec2(colorIndex, 0.5)).rgb;
  
  gl_FragColor = vec4(color * lighting, 1.0);
}
```

**Performance:** 100× faster rendering (GPU parallel processing)
**Quality:** Identical to CPU rendering

---

## 5. Recommended Implementation Path

### Phase 1: Resolution Upgrade (Quick Win)

**Target:** 600×600 or 800×800 grid
**Changes:**
1. Change `tableSize` constant to 600 or 800
2. Scale `ballDiameter` proportionally (6 → 18 or 24)
3. Test performance on target hardware

**Expected Results:**
- 9-16× memory usage
- 3× parallel trace density
- ~10-20 FPS on modern hardware

**Implementation Time:** 1 hour

### Phase 2: Localized Settlement (Performance)

**Target:** Maintain 60 FPS with 800×800 grid
**Changes:**
1. Implement dirty region tracking
2. Replace full-grid settlement with localized settlement
3. Tune region size for balance

**Expected Results:**
- 60 FPS stable at 800×800
- 30-60 FPS at 1000×1000

**Implementation Time:** 4-8 hours

### Phase 3: High-Resolution Option (Stretch Goal)

**Target:** 1500×1500 or 2000×2000 grid
**Approach:** Choose ONE of:
- **Option A:** GPU Acceleration (WebGL rendering + compute)
- **Option B:** Sparse grid with localized settlement
- **Option C:** Multi-threaded settlement (Web Workers)

**Expected Results:**
- 2000×2000 grid at 30-60 FPS
- Photo-realistic sand simulation
- 100+ parallel traces with excellent detail

**Implementation Time:** 1-3 weeks (varies by approach)

---

## 6. Resolution Comparison Table

| Resolution | Memory | Traces | Detail/Trace | Ball Diameter | Settlement Cost | Recommended For |
|------------|--------|--------|--------------|---------------|----------------|----------------|
| 200×200 | 160 KB | 33 | Poor | 6 px | 40K ops | Testing only |
| 400×400 | 640 KB | 66 | Fair | 12 px | 160K ops | Low-end devices |
| 600×600 | 1.4 MB | 100 | Good | 18 px | 360K ops | **Balanced choice** |
| 800×800 | 2.5 MB | 133 | Very good | 24 px | 640K ops | High-quality |
| 1000×1000 | 4 MB | 166 | Excellent | 30 px | 1M ops | With localized settlement |
| 1500×1500 | 9 MB | 250 | Superb | 45 px | 2.25M ops | GPU or sparse grid |
| 2000×2000 | 16 MB | 333 | Photo-realistic | 60 px | 4M ops | GPU acceleration |

---

## 7. Performance Benchmarks (Projected)

### Settlement Performance (with optimizations)

| Grid Size | Full Settlement | Localized Settlement | GPU Settlement |
|-----------|----------------|---------------------|----------------|
| 200×200 | 0.4 ms | 0.05 ms | 0.01 ms |
| 600×600 | 3.6 ms | 0.1 ms | 0.02 ms |
| 1000×1000 | 10 ms | 0.15 ms | 0.03 ms |
| 2000×2000 | 40 ms | 0.2 ms | 0.05 ms |

### Frame Budget @ 60 FPS = 16.67 ms

**600×600 with localized settlement:**
- Ball displacement: 0.05 ms
- Settlement: 0.1 ms
- Rendering: 6 ms
- **Total:** 6.15 ms (37% of budget) ✓

**1000×1000 with localized settlement:**
- Ball displacement: 0.1 ms
- Settlement: 0.15 ms
- Rendering: 6 ms
- **Total:** 6.25 ms (37% of budget) ✓

**2000×2000 with GPU:**
- Ball displacement: 0.1 ms
- Settlement: 0.05 ms
- Rendering: 1 ms (GPU)
- **Total:** 1.15 ms (7% of budget) ✓

---

## 8. Recommended Next Steps

1. **Immediate (1 hour):** Upgrade to 600×600 grid, test performance
2. **Short-term (1 day):** Implement localized settlement
3. **Medium-term (1 week):** Add resolution selector UI (400/600/800/1000)
4. **Long-term (3 weeks):** Evaluate GPU acceleration for 2000×2000

**Priority:** Start with 600×600 + localized settlement. This achieves 100+ parallel traces with manageable complexity and excellent performance.
