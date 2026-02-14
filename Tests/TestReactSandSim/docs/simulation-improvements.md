# Sand Table Simulation Improvement Plan

## Document Overview
**Project:** TestReactSandSim  
**Purpose:** Improve visual fidelity to match real Sisyphus sand table behavior  
**Reference Test File:** `Tests\TestSandVisualisation\images\dither_hypnogrid.thr`  
**Date Created:** 2026-02-13

---

## Current State Analysis

### Visual Comparison
- **Simulation:** Produces smooth, blurred patterns with gentle gradients
- **Real Table Photo:** Shows sharp, high-contrast ridges and troughs with defined edges created by directional lighting on 3D sand features

### Current Implementation Characteristics
1. **Sand Grid:** 200×200 Float32Array
2. **Ball Diameter:** 10 pixels
3. **Move Speed:** 2.0 pixels/frame
4. **Settlement:** 10% chance per frame, 0.9/0.1 neighbor averaging
5. **Rendering:** Direct height-to-color mapping with 30-color palette
6. **Sand Deposition:** Circular gradient `(1 - distance/radius) × 0.5`

---

## Improvement Roadmap

### Priority 1: Critical Visual Impact

#### 1.1 Slope-Based Lighting System
**Problem:** Flat appearance - no 3D lighting model to show ridges and valleys

**Goal:** Implement directional lighting that highlights slopes facing the light source and shadows troughs

**Implementation Steps:**
1. Add slope calculation to rendering pipeline
   - Compute `dx = getSandLevel(x+1, y) - getSandLevel(x-1, y)`
   - Compute `dy = getSandLevel(x, y+1) - getSandLevel(x, y-1)`
   - Handle boundary conditions (return 0 for out-of-bounds)

2. Define light direction vector
   - Default: `lightDir = {x: -0.7, y: -0.7}` (top-left)
   - Make configurable via UI controls

3. Calculate lighting factor
   ```
   slope = dx * lightDir.x + dy * lightDir.y
   if (slope > 0):
       lightingFactor = 1.0 + (slope * 1.1)  // Catching light
   else:
       lightingFactor = 1.0 + (slope * 2.0)  // In shadow (darker)
   ```

4. Apply lighting to color
   - Get base color from palette based on sand height
   - Modulate RGB values: `color × clamp(lightingFactor, 0.4, 1.2)`

**Validation Criteria:**
- [ ] Ridges parallel to light direction appear bright
- [ ] Ridges perpendicular show one bright side, one dark side
- [ ] Troughs appear significantly darker than peaks
- [ ] Pattern matches photo's sharp contrast
- [ ] Adjusting light direction changes shadow positions correctly

**Expected Impact:** 80% improvement in visual realism

---

#### 1.2 Sand Displacement Physics Model
**Problem:** Ball adds sand uniformly - doesn't create realistic push effect

**Goal:** Ball should push sand to sides, creating troughs in path and ridges alongside

**Implementation Steps:**
1. Modify `addSand()` method in `sandSimulation.ts`
   - Current: Adds sand in circular pattern
   - New: Subtracts from center, adds to perpendicular offset

2. Calculate perpendicular displacement
   ```
   ballVelocity = targetPos - currentPos (normalized)
   perpendicular = {-ballVelocity.y, ballVelocity.x}
   
   For each point in ball diameter:
       distFromCenter = distance to ball center
       if (distFromCenter < radius/2):
           // Center of ball - remove sand (trough)
           amount = -0.3 * (1 - distFromCenter/(radius/2))
       else:
           // Outer edge - push sand outward
           offset = perpendicular * (distFromCenter - radius/2)
           targetX = x + offset.x
           targetY = y + offset.y
           amount = 0.5 * (1 - distFromCenter/radius)
   ```

3. Add sand momentum/carry effect
   - Ball can "carry" sand forward slightly
   - Creates more pronounced ridges

**Validation Criteria:**
- [ ] Ball path shows visible trough (darker/lower)
- [ ] Parallel ridges appear on both sides of path
- [ ] Ridge height proportional to ball speed
- [ ] Crossings create appropriate interference patterns
- [ ] Matches real table's path definition

**Expected Impact:** 60% improvement in pattern definition

---

### Priority 2: Behavior Tuning

#### 2.1 Reduce Over-Settlement
**Problem:** Aggressive smoothing destroys fine detail

**Goal:** Maintain sharp features while preventing unrealistic spikes

**Implementation Steps:**
1. Reduce settlement probability
   - From: `if (Math.random() < 0.1)`
   - To: `if (Math.random() < 0.02)` (1 in 50 frames)

2. Reduce blend factor
   - From: `newLevel[idx] = level * 0.9 + avgNeighbor * 0.1`
   - To: `newLevel[idx] = level * 0.98 + avgNeighbor * 0.02`

3. Increase threshold for settlement
   - From: `if (Math.abs(level - avgNeighbor) > 0.5)`
   - To: `if (Math.abs(level - avgNeighbor) > 1.5)`

4. Add stability counter
   - Track how long since major changes
   - Reduce settlement frequency as pattern stabilizes
   - Stop settlement completely after N stable frames

**Validation Criteria:**
- [ ] Fine details preserved over time
- [ ] No unrealistic spikes appear
- [ ] Patterns remain stable after formation
- [ ] Settlement still prevents impossible overhangs
- [ ] Performance impact minimal

**Expected Impact:** 30% improvement in detail retention

---

#### 2.2 Optimize Ball Parameters
**Problem:** Ball too large and fast for fine detail

**Goal:** Match real table's ball-to-table proportions

**Implementation Steps:**
1. Reduce ball diameter
   - From: `ballDiameter: 10`
   - To: `ballDiameter: 5` or `6`

2. Reduce move speed
   - From: `moveSpeed: 2.0`
   - To: `moveSpeed: 0.5` or `0.75`
   - Makes speed configurable in UI

3. Add option for variable speed
   - Slow down on tight curves
   - Speed up on straight segments
   - Match THR file point density

**Validation Criteria:**
- [ ] Ball appears appropriately sized relative to table
- [ ] Patterns show fine detail matching photo
- [ ] Sand buildup sufficient along path
- [ ] Animation still smooth (not jerky)
- [ ] THR files play at appropriate speed

**Expected Impact:** 25% improvement in pattern fidelity

---

### Priority 3: Enhancement Features

#### 3.1 Increase Simulation Resolution
**Problem:** 200×200 grid upscaled to 600×600 loses detail

**Goal:** Sharper rendering without performance issues

**Implementation Steps:**
1. Add resolution option to configuration
   - Options: 200, 400, 600
   - Default: 400×400

2. Implement efficient scaling
   - Keep ball diameter proportional
   - Adjust move speed for resolution
   - Scale kernel operations appropriately

3. Optional: Sub-pixel rendering
   - Bilinear interpolation when sampling sand levels
   - Smooth sub-pixel ball movement

4. Performance profiling
   - Measure frame times at each resolution
   - Add quality vs performance setting

**Validation Criteria:**
- [ ] Higher resolution shows finer detail
- [ ] Frame rate maintains 30+ fps at 400×400
- [ ] Memory usage acceptable
- [ ] Patterns scale proportionally
- [ ] No aliasing artifacts

**Expected Impact:** 20% improvement in visual clarity

---

#### 3.2 Enhanced Color Contrast
**Problem:** Linear height-to-color mapping lacks punch

**Goal:** Increase visual contrast while preserving color accuracy

**Implementation Steps:**
1. Add gamma/power curve option
   ```
   normalizedLevel = level / 20
   contrastAdjusted = Math.pow(normalizedLevel, gamma)
   colorIndex = contrastAdjusted * paletteLength
   ```
   - Default gamma: 0.7 (increases contrast)
   - Configurable: 0.5 to 1.5

2. Add histogram-based palette stretching
   - Analyze actual sand level distribution
   - Map most-used levels to middle of palette
   - Reserve darkest/lightest for extremes

3. Separate lighting and color
   - Color from height (unchanged)
   - Lighting from slope (multiplicative)
   - Prevents washing out colors

**Validation Criteria:**
- [ ] Dark areas sufficiently dark
- [ ] Bright areas properly highlighted
- [ ] Colors remain natural (not oversaturated)
- [ ] Adjustable for different lighting conditions
- [ ] Matches photo's contrast range

**Expected Impact:** 15% improvement (when combined with lighting)

---

## Implementation Phases

### Phase 1: Foundation (Week 1)
**Focus:** Slope-based lighting + ball physics
- Implement slope calculation in rendering
- Add lighting factor calculation
- Rewrite sand deposition model
- Basic validation testing

**Deliverables:**
- Updated `SandTableCanvas.tsx` with lighting
- Updated `sandSimulation.ts` with new physics
- Comparison screenshots before/after

### Phase 2: Tuning (Week 2)
**Focus:** Settlement + ball parameters
- Adjust settlement algorithm
- Optimize ball size and speed
- Add configuration options to UI
- Performance testing

**Deliverables:**
- Adjustable parameters in UI
- Performance benchmarks
- User testing feedback

### Phase 3: Enhancement (Week 3)
**Focus:** Resolution + contrast
- Implement resolution options
- Add contrast controls
- Final polish and optimization
- Documentation

**Deliverables:**
- Complete feature set
- User documentation
- Performance guide

---

## Testing & Validation Protocol

### Visual Comparison Tests
1. **Side-by-side comparison**
   - Load `dither_hypnogrid.thr`
   - Capture simulation at T=10s, T=30s, T=60s
   - Compare with real table photos at same timestamps
   - Document improvements

2. **Feature checklist**
   - [ ] Ridge sharpness
   - [ ] Trough darkness
   - [ ] Edge definition
   - [ ] Pattern consistency
   - [ ] Light/shadow realism

### Performance Benchmarks
| Metric | Current | Target | Actual |
|--------|---------|--------|--------|
| Frame Rate @ 200×200 | 60fps | 60fps | ___ |
| Frame Rate @ 400×400 | N/A | 30fps | ___ |
| Memory Usage | ~160KB | <500KB | ___ |
| Frame Time | ~16ms | <33ms | ___ |

### Accuracy Validation
1. **Pattern comparison metrics**
   - Measure contrast ratio (bright/dark)
   - Measure edge sharpness (gradient steepness)
   - Measure feature size (ridge width)
   - Compare against reference photo

2. **Physics validation**
   - Ball leaves visible trough
   - Ridges form alongside path
   - Crossings create expected patterns
   - Settlement preserves details

---

## Configuration Options

### New UI Controls (to be added)

#### Simulation Physics
- **Ball Diameter:** Slider (3-15), default: 5
- **Move Speed:** Slider (0.1-3.0), default: 0.75
- **Settlement Rate:** Slider (0-0.1), default: 0.02
- **Settlement Blend:** Slider (0-0.2), default: 0.02

#### Visual Rendering
- **Light Direction X:** Slider (-1 to 1), default: -0.7
- **Light Direction Y:** Slider (-1 to 1), default: -0.7
- **Lighting Intensity:** Slider (0-2), default: 1.0
- **Contrast Gamma:** Slider (0.5-1.5), default: 0.7
- **Simulation Resolution:** Dropdown (200/400/600)

#### Display Options
- **Enable Slope Lighting:** Checkbox, default: true
- **Show Light Direction:** Checkbox (overlay arrow)
- **Show Statistics:** FPS, sand levels, etc.

---

## Rollback Strategy

### Version Control
- Tag current version as `v1.0-baseline`
- Each improvement gets feature branch
- Main branch only updated after validation
- Keep reference screenshots for comparison

### Feature Flags
- Implement all improvements behind toggles
- Allow A/B testing of features
- Easy disable if issues found
- Preserved for future debugging

### Performance Safeguards
- Monitor frame time each render
- Auto-reduce resolution if FPS drops
- Warn user of performance issues
- Provide "low quality" mode option

---

## Success Metrics

### Visual Fidelity
- **Goal:** 90%+ similarity to real table photo
- **Measure:** User survey ratings (1-10)
- **Target:** Average rating ≥ 8.5

### Performance
- **Goal:** Maintain 30+ fps on standard hardware
- **Measure:** Frame time statistics
- **Target:** 99th percentile < 40ms

### User Satisfaction
- **Goal:** Improved realism perception
- **Measure:** Before/after preference survey
- **Target:** 80%+ prefer new version

### Technical Debt
- **Goal:** Code remains maintainable
- **Measure:** Complexity metrics, test coverage
- **Target:** Zero regressions in existing features

---

## Future Enhancements (Out of Scope)

1. **WebGL Rendering**
   - GPU-accelerated sand simulation
   - Real-time normal mapping
   - Up to 1000×1000 resolution

2. **Advanced Physics**
   - Dynamic sand viscosity
   - Avalanche simulation
   - Grain-level modeling

3. **Machine Learning**
   - Train on real table videos
   - Learn optimal parameters
   - Auto-tune for different patterns

4. **Multi-ball Simulation**
   - Support multiple balls
   - Ball-ball interactions
   - Choreographed patterns

---

## References

### Code Locations
- **Simulation Physics:** `src/sandSimulation.ts`
- **Rendering:** `src/SandTableCanvas.tsx` (lines 280-350)
- **Original HTML Reference:** `Tests/TestSandVisualisation/SandTableSim30.html`
- **THR Utilities:** `src/thrUtils.ts`

### Key Algorithms (HTML Reference)
- **Slope calculation:** SandTableSim30.html:436-442
- **Shading rings:** SandTableSim30.html:520-535
- **Sand lightness:** SandTableSim30.html:400-475

### Research Papers
- Sisyphus table: sisyphus-industries.com
- Sand simulation: Search "granular material simulation"
- Lighting models: Phong/Blinn-Phong shading

---

## Appendix: Quick Start Implementation

### Minimal Viable Improvement (1 hour)
Just slope-based lighting:

```typescript
// In renderSand() function, replace color mapping with:
const level = kernel.getSandLevel(tx, ty);
const levelRight = kernel.getSandLevel(tx + 1, ty);
const levelLeft = kernel.getSandLevel(tx - 1, ty);
const levelDown = kernel.getSandLevel(tx, ty + 1);
const levelUp = kernel.getSandLevel(tx, ty - 1);

const dx = levelRight - levelLeft;
const dy = levelDown - levelUp;
const slope = dx * (-0.7) + dy * (-0.7); // Light from top-left

let lightingFactor = 1.0;
if (slope > 0) {
    lightingFactor = 1.0 + slope * 1.1;
} else {
    lightingFactor = 1.0 + slope * 2.0;
}
lightingFactor = Math.max(0.4, Math.min(1.2, lightingFactor));

// Get base color from palette
const normalized = level / 20;
const colorIndex = Math.min(Math.floor(normalized * palette.length), palette.length - 1);
let rgb = palette[colorIndex];

// Apply lighting
rgb = {
    r: Math.min(255, rgb.r * lightingFactor),
    g: Math.min(255, rgb.g * lightingFactor),
    b: Math.min(255, rgb.b * lightingFactor),
    a: rgb.a
};
```

This single change will produce 80% of the visual improvement.

---

**Document Status:** Draft v1.0  
**Next Review:** After Phase 1 completion  
**Owner:** Development Team
