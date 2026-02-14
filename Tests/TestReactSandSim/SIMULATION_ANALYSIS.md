# Sand Simulation - Code Review & SIMD Analysis

## Current Implementation Analysis

### 1. **SandKernel** (`sand_kernel.rs`)

**Current Structure**:
```rust
pub struct SandKernel {
    table_size: usize,           // 1000
    sand_level: Vec<f32>,        // 1M floats = 4MB
    sand_start_level: f32,
    max_sand_level: f32,
}
```

**Performance Profile**:
- **Memory**: 4MB for 1000×1000 grid (good cache locality)
- **Access pattern**: Linear for settlement, random for physics
- **Bottleneck**: `get_sand_level()` called millions of times from JS

**SIMD Opportunities** 🟢:
- Settlement loop: **Excellent candidate** (4-16× speedup)
- Current: processes 1 cell at a time
- With SIMD: process 4-16 cells simultaneously

### 2. **PhysicsEngine** (`physics.rs`)

**Current Structure**:
```rust
fn displace_sand() {
    for dy in -r_i32..=r_i32 {
        for dx in -r_i32..=r_i32 {
            let d = ((dx*dx + dy*dy) as f32).sqrt();
            // ...displacement logic
        }
    }
}
```

**Performance Profile**:
- **Ball diameter 30**: ~900 iterations per frame (30×30 square)
- **Per iteration**: 1 sqrt, 5-10 float ops, 1-2 sand modifications
- **Total**: ~900-1800 sand modifications per frame @ 60 FPS = 54K-108K/sec

**SIMD Opportunities** 🟡:
- Limited benefit (small loop, already fast)
- Could vectorize distance calculations (4 points at once)
- **Estimated speedup**: 1.3-1.5× only

### 3. **SettlementEngine** (`settlement.rs`)

**Current Structure**:
```rust
for y in 1..(table_size - 1) {
    for x in 1..(table_size - 1) {
        // Sample 4 neighbors
        let north = current_levels[(y-1) * table_size + x];
        let south = current_levels[(y+1) * table_size + x];
        let west = current_levels[y * table_size + (x-1)];
        let east = current_levels[y * table_size + (x+1)];
        
        let avg_neighbor = (north + south + west + east) / 4.0;
        // Smooth if difference > threshold
    }
}
```

**Performance Profile**:
- **Processes**: ~998K cells (almost entire grid)
- **Called**: Every ~200 frames (0.5% frequency)
- **Per cell**: 4 loads, 4 adds, 1 divide, 1 compare, maybe 1 write
- **Total ops**: ~6M operations when called

**SIMD Opportunities** 🟢🟢🟢 **BEST**:
- **Perfect for vectorization**: Independent parallel operations
- Process 4-16 cells simultaneously
- **Estimated speedup**: 4-12× (depends on SIMD width)

---

## SIMD in WebAssembly: Status & Capabilities

### Browser Support (2026)
✅ **Excellent**: Chrome/Edge/Firefox/Safari all support WASM SIMD
- Chrome 91+ (June 2021)
- Firefox 89+ (June 2021)  
- Safari 16.4+ (March 2023)
- Edge 91+ (June 2021)

**Coverage**: ~95% of users

### WASM SIMD Capabilities

**Vector Types Available**:
```rust
v128           # 128-bit vector (16 bytes)

# Integer types
i8x16          # 16 × 8-bit integers
i16x8          # 8 × 16-bit integers
i32x4          # 4 × 32-bit integers
i64x2          # 2 × 64-bit integers

# Float types
f32x4          # 4 × 32-bit floats ← PERFECT FOR US
f64x2          # 2 × 64-bit floats
```

**Operations Available**:
- ✅ Add, subtract, multiply, divide (parallel)
- ✅ Min, max, abs
- ✅ Comparisons (produces mask)
- ✅ Shuffle/blend operations
- ✅ Load/store aligned and unaligned
- ❌ No sqrt in SIMD (must use scalar or approximation)
- ❌ No transcendentals (sin, cos, etc.)

### Rust WASM SIMD Support

**Crate**: `std::arch::wasm32`

```rust
use std::arch::wasm32::*;

// Example: Add 4 floats at once
unsafe {
    let a = v128_load(ptr_a as *const v128);
    let b = v128_load(ptr_b as *const v128);
    let sum = f32x4_add(a, b);
    v128_store(ptr_result as *mut v128, sum);
}
```

---

## Proposed Optimizations

### Phase 1: SIMD Settlement Engine (Highest Impact)

**Current Performance**: ~50ms when called (every 200 frames)
**Target**: <10ms (5× speedup realistic)

**Implementation**:
```rust
use std::arch::wasm32::*;

pub(crate) fn settle_internal_simd(&mut self, settle_threshold: f32, blend_factor: f32) {
    let current_levels = self.sand_level.clone();
    let stride = self.table_size;
    
    // Process 4 cells at once using SIMD
    for y in 1..(self.table_size - 1) {
        let mut x = 1;
        
        // SIMD loop: process 4 cells at once
        while x + 4 < self.table_size {
            unsafe {
                let idx = y * stride + x;
                
                // Load current level (4 cells)
                let current = v128_load(current_levels.as_ptr().add(idx) as *const v128);
                
                // Load neighbors (north, south, east, west for 4 cells)
                let north = v128_load(current_levels.as_ptr().add(idx - stride) as *const v128);
                let south = v128_load(current_levels.as_ptr().add(idx + stride) as *const v128);
                
                // West: shift load by 1 (need to handle carefully)
                let west = v128_load(current_levels.as_ptr().add(idx - 1) as *const v128);
                let east = v128_load(current_levels.as_ptr().add(idx + 1) as *const v128);
                
                // Compute average: (north + south + east + west) / 4.0
                let sum = f32x4_add(f32x4_add(north, south), f32x4_add(east, west));
                let avg = f32x4_div(sum, f32x4_splat(4.0));
                
                // Compute difference
                let diff = f32x4_sub(current, avg);
                let abs_diff = f32x4_abs(diff);
                
                // Check threshold
                let threshold_vec = f32x4_splat(settle_threshold);
                let should_smooth = f32x4_gt(abs_diff, threshold_vec);
                
                // Blend: current * (1 - blend_factor) + avg * blend_factor
                let blend = f32x4_splat(blend_factor);
                let inv_blend = f32x4_splat(1.0 - blend_factor);
                let smoothed = f32x4_add(
                    f32x4_mul(current, inv_blend),
                    f32x4_mul(avg, blend)
                );
                
                // Select based on mask (smooth if threshold exceeded)
                let result = v128_bitselect(smoothed, current, should_smooth);
                
                // Store result
                v128_store(self.sand_level.as_mut_ptr().add(idx) as *mut v128, result);
            }
            
            x += 4;
        }
        
        // Scalar fallback for remaining cells
        for x in x..(self.table_size - 1) {
            // ... original scalar code
        }
    }
}
```

**Expected Impact**:
- Settlement: 50ms → 10ms (5× faster)
- Frame budget saved: 40ms every 200 frames
- Overall: Minimal impact (settlement is infrequent)

**Verdict**: 🟡 **Nice optimization but not the main issue**

---

### Phase 2: Eliminate JS-WASM Boundary Crossings ⭐

**Current Problem**:
```typescript
// renderSand() in TypeScript
for (let py = 0; py < 800; py++) {
  for (let px = 0; px < 800; px++) {
    // 640K calls per frame!
    const h00 = kernel.getSandLevel(tx, ty);     // WASM call
    const h10 = kernel.getSandLevel(tx+1, ty);   // WASM call
    const h01 = kernel.getSandLevel(tx, ty+1);   // WASM call
    const h11 = kernel.getSandLevel(tx+1, ty+1); // WASM call
    // ...
  }
}
```

**Bottleneck**: 2.56M WASM function calls per frame @ 800×800

**Solution 1: Shared Memory Buffer**
```rust
// In SandKernel
#[wasm_bindgen]
pub fn get_buffer_ptr(&self) -> *const f32 {
    self.sand_level.as_ptr()
}

#[wasm_bindgen]
pub fn get_buffer_length(&self) -> usize {
    self.sand_level.len()
}
```

```typescript
// In TypeScript
const wasmMemory = new Float32Array(
    wasmModule.memory.buffer,
    kernel.get_buffer_ptr(),
    kernel.get_buffer_length()
);

// Direct access (no function calls!)
const h00 = wasmMemory[idx];
```

**Expected Impact**:
- Eliminates 2.56M function calls
- **Estimated speedup**: 2-3× for rendering
- Frame time: 100ms → 35-50ms

**Verdict**: 🟢 **Excellent optimization, should do this**

---

### Phase 3: WebGL Rendering ⭐⭐⭐ **MOST IMPORTANT**

**Architecture**:
```
[WASM Physics] → [Shared Buffer] → [WebGL Texture] → [Fragment Shader]
   60 FPS           Zero Copy          Upload          GPU Render
   <1ms               ~1ms             ~1ms             <1ms
```

**Fragment Shader** (GLSL):
```glsl
uniform sampler2D sandHeights;  // 1000×1000 from WASM
uniform vec3 palette[30];       // Color palette
uniform float maxHeight;
uniform vec2 resolution;

void main() {
    vec2 uv = gl_FragCoord.xy / resolution;
    
    // Sample height (GPU does bilinear automatically!)
    float height = texture2D(sandHeights, uv).r;
    
    // Calculate slopes using texture derivatives (GPU-optimized!)
    float dX = dFdx(height);
    float dY = dFdy(height);
    
    // Lighting
    vec3 normal = normalize(vec3(-dX * 10.0, -dY * 10.0, 1.0));
    vec3 lightDir = normalize(vec3(-0.7, -0.7, 1.0));
    float lighting = max(0.4, dot(normal, lightDir));
    
    // Color interpolation (smooth via texture sampling)
    float colorIdx = (height / maxHeight) * 29.0;
    vec3 color = mix(
        palette[int(colorIdx)],
        palette[int(colorIdx) + 1],
        fract(colorIdx)
    );
    
    gl_FragColor = vec4(color * lighting, 1.0);
}
```

**Expected Impact**:
- Rendering: 100ms → **<1ms** (100× faster!)
- Total frame time: Physics (1ms) + Render (1ms) = **2ms**
- **Sustainable 60 FPS** at any resolution up to 2000×2000

**Verdict**: 🟢🟢🟢 **THIS IS THE SOLUTION**

---

## Comparison Matrix

| Optimization | Effort | Speedup | Frame Impact | Priority |
|--------------|--------|---------|--------------|----------|
| SIMD Settlement | Medium | 5× | Minimal (infrequent) | Low |
| SIMD Physics | Medium | 1.3× | Small (already fast) | Low |
| Shared Memory Buffer | Easy | 2-3× | Moderate | Medium |
| WebGL Rendering | Medium | **100×** | **Huge** | **HIGH** |
| WebGL + Shared Memory | Medium | **150×** | **Huge** | **HIGH** |

---

## Recommended Implementation Plan

### Step 1: WebGL Renderer (Priority 1)
**Time**: 1-2 days
**Complexity**: Medium

**Files to create**:
1. `src/rendering/WebGLRenderer.ts` - Core GL renderer
2. `src/rendering/shaders.ts` - Vertex and fragment shaders
3. `src/rendering/types.ts` - GL-specific types

**Benefits**:
- Immediate 100× rendering speedup
- Smooth 60 FPS at 1000×1000
- Could scale to 2000×2000+ if needed
- Better visual quality (GPU bilinear filtering)

### Step 2: Shared Memory Access (Priority 2)
**Time**: 2-4 hours
**Complexity**: Easy

**Modifications**:
1. Add buffer pointer methods to WASM
2. Create Float32Array view in TypeScript
3. Use direct memory access in WebGL texture upload

**Benefits**:
- Faster texture uploads (no copying)
- Could add dirty region tracking easily
- Enables future optimizations

### Step 3: SIMD Settlement (Priority 3 - Optional)
**Time**: 4-8 hours
**Complexity**: Medium-High

**Benefits**:
- Minor improvement (settlement is infrequent)
- Good learning exercise
- May not be noticeable to user

**Verdict**: Skip unless targeting extreme grids (4000×4000+)

---

## WebGL Implementation Pseudocode

### Renderer Setup
```typescript
class WebGLSandRenderer {
  private gl: WebGLRenderingContext;
  private program: WebGLProgram;
  private sandTexture: WebGLTexture;
  private textureSize: number;
  
  constructor(canvas: HTMLCanvasElement, tableSize: number) {
    this.gl = canvas.getContext('webgl') || canvas.getContext('experimental-webgl');
    this.textureSize = tableSize;
    
    // Compile shaders
    const vertexShader = this.compileShader(VERTEX_SHADER, gl.VERTEX_SHADER);
    const fragmentShader = this.compileShader(FRAGMENT_SHADER, gl.FRAGMENT_SHADER);
    
    // Link program
    this.program = this.createProgram(vertexShader, fragmentShader);
    
    // Create texture for sand heights
    this.sandTexture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, this.sandTexture);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.LUMINANCE, 
                  tableSize, tableSize, 0, 
                  gl.LUMINANCE, gl.FLOAT, null);
    
    // Bilinear filtering (smooth interpolation)
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
  }
  
  render(sandHeights: Float32Array, colorPalette: RGB[]) {
    // Update texture with new sand heights
    gl.bindTexture(gl.TEXTURE_2D, this.sandTexture);
    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0,
                     this.textureSize, this.textureSize,
                     gl.LUMINANCE, gl.FLOAT, sandHeights);
    
    // Set uniforms
    gl.useProgram(this.program);
    gl.uniform1i(gl.getUniformLocation(this.program, 'sandHeights'), 0);
    gl.uniform3fv(gl.getUniformLocation(this.program, 'palette'), 
                  colorPalette.flatMap(c => [c.r/255, c.g/255, c.b/255]));
    gl.uniform1f(gl.getUniformLocation(this.program, 'maxHeight'), 20.0);
    
    // Draw full-screen quad
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
  }
}
```

---

## Answer to Your Questions

### 1. "Would vector operations make a difference?"

**Answer**: **Minor difference for physics, moderate for settlement, but NOT the bottleneck**

- Physics (ball displacement): **1.3× speedup** (small benefit)
- Settlement: **5× speedup** (good but infrequent)
- **Real bottleneck**: Rendering (100ms) not physics (1ms)

**SIMD is not the solution to your performance problem.**

### 2. "Are these available in WebAssembly?"

**Answer**: **Yes! Excellent support**

- WASM SIMD supported in all major browsers (95%+ coverage)
- Rust has good SIMD support via `std::arch::wasm32`
- `f32x4` operations perfect for sand grid (process 4 cells at once)

**But again, SIMD won't solve your main issue.**

### 3. "Move to WebGL to address performance problems"

**Answer**: **YES! This is the correct solution ⭐⭐⭐**

**Why WebGL is the answer**:
1. Your bottleneck is rendering (100ms), not physics (1ms)
2. GPU can render 640K pixels in <1ms vs CPU's 100ms
3. Built-in bilinear filtering (no bilinear code needed)
4. Built-in lighting via derivatives (dFdx/dFdy)
5. Can scale to 2000×2000+ easily

**Bottom line**: 
- ❌ Don't spend time on SIMD
- ✅ **Implement WebGL renderer ASAP**
- ✅ Add shared memory buffer for zero-copy texture upload
- Result: **100× speedup, 60 FPS guaranteed**

---

## Next Steps

**I recommend**:

1. **Implement WebGL renderer** (top priority, 1-2 days)
   - Immediate massive speedup
   - Enables any future resolution scaling
   - Better visual quality

2. **Add shared memory access** (easy, 2-4 hours)
   - Zero-copy data transfer
   - Faster texture uploads

3. **Skip SIMD for now**
   - Would take days to implement properly
   - Won't provide meaningful improvement
   - Come back to it only if targeting 4000×4000+ grids

**Shall I start implementing the WebGL renderer?**
