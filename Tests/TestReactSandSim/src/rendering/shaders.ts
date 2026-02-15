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
    
    // Sample 4 neighbors for normal computation (explicit, not dFdx/dFdy)
    float hLeft  = texture2D(uSandHeights, vTexCoord + vec2(-texelSize.x, 0.0)).r;
    float hRight = texture2D(uSandHeights, vTexCoord + vec2( texelSize.x, 0.0)).r;
    float hUp    = texture2D(uSandHeights, vTexCoord + vec2(0.0, -texelSize.y)).r;
    float hDown  = texture2D(uSandHeights, vTexCoord + vec2(0.0,  texelSize.y)).r;
    
    // Compute surface normal from height differences
    // Scale the height differences to make lighting more dramatic
    float heightScale = 8.0;
    float dHdx = (hRight - hLeft) * heightScale;
    float dHdy = (hDown  - hUp)   * heightScale;
    vec3 normal = normalize(vec3(-dHdx, -dHdy, 1.0));
    
    // ---- Lighting Model (Analytical Hemisphere) ----
    // A uniform ring of LEDs around the rim is equivalent to hemispherical
    // ambient lighting. Flat surfaces (normal pointing up) receive full light.
    // Tilted surfaces receive less — troughs and ridge slopes appear darker.
    // No per-light loop = no interference/spottiness artifacts.
    
    // Hemisphere diffuse: how much the surface faces upward
    // normal.z = 1.0 for flat, < 1.0 for tilted slopes
    float hemisphere = normal.z;
    
    // Radial rim contribution: surfaces tilted toward the table edge
    // receive slightly more light (they face the LED ring).
    // Direction from center toward this pixel (where LEDs are)
    vec2 toEdge = normalize(vTexCoord - center);
    float rimFacing = max(0.0, dot(normal.xy, toEdge) * 0.15);
    
    // Combine: strong ambient + hemisphere diffuse + subtle rim bias
    float lighting = 0.4 + hemisphere * 0.55 + rimFacing;
    
    // ---- Sand Grain Texture ----
    // Add subtle brightness variation to simulate individual sand grains.
    // Applied as a small multiplier on the final lighting, not on normals.
    vec2 grainCoord = vTexCoord * uResolution;
    float grain = valueNoise(grainCoord * 0.5) * 0.6 + valueNoise(grainCoord * 0.25) * 0.4;
    lighting *= 0.97 + grain * 0.06; // ±3% brightness variation
    
    // ---- Base Color from Height ----
    
    // Normalize height for palette lookup
    float normalized = clamp((height - uMinHeight) / uMaxHeight, 0.0, 1.0);
    float palettePos = normalized * 29.0;
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
    
    // Apply lighting to base color
    vec3 finalColor = baseColor * lighting;
    
    // Clamp to valid range
    finalColor = clamp(finalColor, 0.0, 1.0);
    
    gl_FragColor = vec4(finalColor, 1.0);
}
`;
