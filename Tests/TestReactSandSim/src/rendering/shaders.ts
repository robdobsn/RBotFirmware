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
 * Fragment shader - renders sand with directional lighting
 * 
 * Uses explicit neighbor sampling to compute surface normals,
 * then applies Phong-like lighting for realistic sand appearance.
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
    float heightScale = 8.0; // Controls how dramatic the lighting is
    float dHdx = (hRight - hLeft) * heightScale;
    float dHdy = (hDown  - hUp)   * heightScale;
    vec3 normal = normalize(vec3(-dHdx, -dHdy, 1.0));
    
    // ---- Lighting Model ----
    
    // Light direction (from upper-left, angled down at ~45 degrees)
    vec3 lightDir = normalize(vec3(-0.6, -0.6, 0.8));
    
    // View direction (straight down onto table)
    vec3 viewDir = vec3(0.0, 0.0, 1.0);
    
    // Ambient: base illumination so shadows aren't pure black
    float ambient = 0.35;
    
    // Diffuse: Lambertian lighting (angle between normal and light)
    float diffuse = max(0.0, dot(normal, lightDir));
    
    // Specular: Blinn-Phong highlight for shiny ridges
    vec3 halfDir = normalize(lightDir + viewDir);
    float specAngle = max(0.0, dot(normal, halfDir));
    float specular = pow(specAngle, 32.0) * 0.3;
    
    // Combine lighting components
    float lighting = ambient + diffuse * 0.6 + specular;
    
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
