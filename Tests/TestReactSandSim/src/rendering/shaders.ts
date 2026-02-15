/**
 * WebGL Shaders for Sand Table Rendering
 * 
 * These shaders handle GPU-accelerated rendering of the sand simulation.
 * The fragment shader performs bilinear sampling, slope-based lighting,
 * and smooth color interpolation entirely on the GPU.
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
 * Fragment shader - renders sand with lighting and color
 * 
 * Features:
 * - Bilinear height sampling (automatic via GL_LINEAR)
 * - Slope-based lighting using texture derivatives
 * - Smooth color palette interpolation
 * - Circular table masking
 */
export const FRAGMENT_SHADER = `
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

#extension GL_OES_standard_derivatives : enable

uniform sampler2D uSandHeights;  // Sand height texture (1000×1000)
uniform vec3 uPalette[30];        // Color palette (30 colors)
uniform float uMinHeight;         // Minimum sand height for normalization
uniform float uMaxHeight;         // Height range (max - min)
uniform vec2 uResolution;         // Canvas resolution
uniform vec2 uTableSize;          // Sand table grid size

varying vec2 vTexCoord;

void main() {
    // Sample sand height from texture (GPU does bilinear automatically)
    float height = texture2D(uSandHeights, vTexCoord).r;
    
    // Check if within circular table boundary
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(vTexCoord, center);
    if (dist > 0.5) {
        // Outside circle - render black
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // DIAGNOSTIC: Visualize raw height value to verify texture is working
    // Uncomment next line to see if height varies across texture (should see gradients, not uniform color)
    // gl_FragColor = vec4(height/10.0, height/10.0, height/10.0, 1.0); return;
    
    // Map height to palette index (0 to 29)
    // Normalize: (height - min) / (max - min) to get 0-1 range
    float normalized = clamp((height - uMinHeight) / uMaxHeight, 0.0, 1.0);
    
    // DIAGNOSTIC: Visualize normalized value (should be 0.0 to 1.0, showing as black to white)
    // Uncomment next line to verify normalization is working
    // gl_FragColor = vec4(normalized, normalized, normalized, 1.0); return;
    
    float palettePos = normalized * 29.0;
    float colorFrac = fract(palettePos);
    
    // Clamp indices to valid range
    int idx1 = int(floor(palettePos));
    int idx2 = int(ceil(palettePos));
    
    // WebGL 1.0 requires constant array indices, so use loop-based lookup
    // Initialize to first color as fallback (not black!)
    vec3 color1 = uPalette[0];
    vec3 color2 = uPalette[0];
    
    // Loop through palette and pick colors matching our indices
    for (int i = 0; i < 30; i++) {
        if (i == idx1) {
            color1 = uPalette[i];
        }
        if (i == idx2) {
            color2 = uPalette[i];
        }
    }
    
    // Interpolate between two palette colors for smooth gradients
    vec3 baseColor = mix(color1, color2, colorFrac);
    
    // Calculate slopes using texture derivatives (GPU-optimized) for lighting
    float dHdx = dFdx(height) * uTableSize.x;
    float dHdy = dFdy(height) * uTableSize.y;
    
    // Compute surface normal from slopes
    vec3 normal = normalize(vec3(-dHdx, -dHdy, 1.0));
    
    // Light direction (from top-left)
    vec3 lightDir = normalize(vec3(-0.7, -0.7, 1.0));
    
    // Calculate lighting (diffuse + ambient)
    float diffuse = max(0.0, dot(normal, lightDir));
    float lighting = mix(0.4, 1.2, diffuse);  // Range: 0.4 (shadow) to 1.2 (highlight)
    
    // Apply lighting to color
    vec3 finalColor = baseColor * lighting;
    
    gl_FragColor = vec4(finalColor, 1.0);
}
`;
