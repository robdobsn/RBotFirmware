/**
 * WebGL Shaders for Sand Table Rendering
 * 
 * These shaders handle GPU-accelerated rendering of the sand simulation.
 * The fragment shader performs explicit normal calculation from neighboring
 * height samples, then applies a directional lighting model with ambient,
 * diffuse, and specular components to create realistic sand appearance.
 */

/**
 * Vertex shader - creates a full-screen quad
 * No transformations needed, just pass through texture coordinates
 */
export const VERTEX_SHADER = `
attribute vec2 aPosition;
attribute vec2 aTexCoord;

varying vec2 vTexCoord;

void main() {
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
`;

/**
 * Fragment shader - renders sand with radial edge lighting
 * 
 * Uses explicit neighbor sampling to compute surface normals,
 * then applies lighting from 8 virtual LEDs evenly spaced around
 * the table rim, simulating real sand table edge-mounted LED strips.
 * Troughs appear dark/shadowed, ridges appear bright/highlighted.
 */
export const FRAGMENT_SHADER = `
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

uniform sampler2D uSandHeights;  // Sand height texture
uniform vec3 uPalette[30];       // Color palette (30 colors)
uniform float uMinHeight;        // Minimum sand height
uniform float uMaxHeight;        // Height range (max - min)
uniform vec2 uResolution;        // Canvas resolution
uniform vec2 uTableSize;         // Sand table grid size

varying vec2 vTexCoord;

// Hash-based pseudo-random noise (deterministic per pixel)
float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

// Value noise with smooth interpolation (gives grain-like texture)
float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    // Smooth interpolation
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x),
        u.y
    );
}

void main() {
    // Check if within circular table boundary
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(vTexCoord, center);
    if (dist > 0.5) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // Sample sand height at this pixel
    float height = texture2D(uSandHeights, vTexCoord).r;
    
    // Calculate texel size for neighbor sampling
    vec2 texelSize = vec2(1.0 / uTableSize.x, 1.0 / uTableSize.y);
    
    // ---- Sand Grain Roughness ----
    // Add per-grain height noise BEFORE normal computation.
    // This makes transitions rough and irregular like real sand.
    // Amplitude kept moderate so normals don't flip peak/trough lighting.
    vec2 grainCoord = vTexCoord * uTableSize;
    
    // Sample 4 neighbors with grain noise applied to each.
    // Each neighbor gets its OWN noise value, creating jagged normals.
    float noiseAmp = 0.12; // Reduced to prevent lighting inversions
    float hLeft  = texture2D(uSandHeights, vTexCoord + vec2(-texelSize.x, 0.0)).r
                 + (valueNoise((grainCoord + vec2(-1.0, 0.0)) * 0.8) - 0.5) * noiseAmp;
    float hRight = texture2D(uSandHeights, vTexCoord + vec2( texelSize.x, 0.0)).r
                 + (valueNoise((grainCoord + vec2( 1.0, 0.0)) * 0.8) - 0.5) * noiseAmp;
    float hUp    = texture2D(uSandHeights, vTexCoord + vec2(0.0, -texelSize.y)).r
                 + (valueNoise((grainCoord + vec2(0.0, -1.0)) * 0.8) - 0.5) * noiseAmp;
    float hDown  = texture2D(uSandHeights, vTexCoord + vec2(0.0,  texelSize.y)).r
                 + (valueNoise((grainCoord + vec2(0.0,  1.0)) * 0.8) - 0.5) * noiseAmp;
    
    // Compute surface normal from noisy height differences
    float heightScale = 14.0; // Higher = sharper edge contrast
    float dHdx = (hRight - hLeft) * heightScale;
    float dHdy = (hDown  - hUp)   * heightScale;
    vec3 normal = normalize(vec3(-dHdx, -dHdy, 1.0));
    
    // ---- Lighting Model (Analytical Hemisphere) ----
    float hemisphere = normal.z;
    vec2 toEdge = normalize(vTexCoord - center);
    float rimFacing = max(0.0, dot(normal.xy, toEdge) * 0.15);
    float lighting = 0.45 + hemisphere * 0.6 + rimFacing;
    
    // ---- Base Color from Height ----
    // Apply smoothstep contrast curve for defined trough/peak transitions.
    float normalized = clamp((height - uMinHeight) / uMaxHeight, 0.0, 1.0);
    float contrasted = normalized * normalized * (3.0 - 2.0 * normalized); // single smoothstep
    float palettePos = contrasted * 29.0;
    float colorFrac = fract(palettePos);
    
    int idx1 = int(floor(palettePos));
    int idx2 = int(ceil(palettePos));
    
    // WebGL 1.0: loop-based palette lookup
    vec3 color1 = uPalette[0];
    vec3 color2 = uPalette[0];
    for (int i = 0; i < 30; i++) {
        if (i == idx1) color1 = uPalette[i];
        if (i == idx2) color2 = uPalette[i];
    }
    vec3 baseColor = mix(color1, color2, colorFrac);
    
    // ---- Sand Grain Texture (constrained scatter) ----
    // Add grain noise but constrain it so peaks always stay brighter than troughs.
    // The scatter range shrinks at extremes to prevent inversions.
    vec2 sparkleCoord = vTexCoord * uResolution;
    float sparkle = valueNoise(sparkleCoord * 1.5) - 0.5; // [-0.5, 0.5]
    float fineSparkle = valueNoise(sparkleCoord * 3.0) - 0.5;
    float grainVar = (sparkle * 0.6 + fineSparkle * 0.4);
    
    // Scale noise so it can't push a pixel across the peak/trough divide:
    // At normalized=0 (trough), allow only slight brightening (+0 to +0.05)
    // At normalized=1 (peak), allow only slight darkening (-0.05 to 0)
    // At normalized=0.5 (mid), full range ±0.08
    float maxBrighten = (1.0 - normalized) * 0.12;
    float maxDarken = normalized * 0.12;
    float scatter = clamp(grainVar * 0.16, -maxDarken, maxBrighten);
    
    // Height-based brightness boost: peaks get pushed toward full white
    float heightBoost = contrasted * 0.25; // up to +0.25 at peaks
    
    // Apply lighting, scatter, and height boost
    vec3 finalColor = baseColor * (lighting + scatter + heightBoost);
    
    // Clamp to valid range
    finalColor = clamp(finalColor, 0.0, 1.0);
    
    gl_FragColor = vec4(finalColor, 1.0);
}
`;
