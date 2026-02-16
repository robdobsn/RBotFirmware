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
 * Fragment shader - renders sand with height-based luminance
 * 
 * Core rule: peaks are bright (near white), troughs are dark.
 * Grain texture is concentrated at transition edges (steep slopes)
 * where real sand grains are visibly disturbed, while flat peaks
 * and trough bottoms remain smooth.
 */
export const FRAGMENT_SHADER = `
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

uniform sampler2D uSandHeights;  // Sand height texture
uniform vec3 uBaseColor;         // Base sand color (e.g. cool silver-gray)
uniform float uMinHeight;        // Minimum display height (typically 0)
uniform float uMaxHeight;        // Display height range
uniform vec2 uResolution;        // Canvas resolution
uniform vec2 uTableSize;         // Sand table grid size

varying vec2 vTexCoord;

// Hash-based pseudo-random noise for grain texture
float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
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
    vec2 texelSize = vec2(1.0 / uTableSize.x, 1.0 / uTableSize.y);
    
    // Sample neighbors for slope computation
    float hLeft  = texture2D(uSandHeights, vTexCoord + vec2(-texelSize.x, 0.0)).r;
    float hRight = texture2D(uSandHeights, vTexCoord + vec2( texelSize.x, 0.0)).r;
    float hUp    = texture2D(uSandHeights, vTexCoord + vec2(0.0, -texelSize.y)).r;
    float hDown  = texture2D(uSandHeights, vTexCoord + vec2(0.0,  texelSize.y)).r;
    
    // Height gradient magnitude — high at transition edges, zero on flats
    float dHdx = hRight - hLeft;
    float dHdy = hDown - hUp;
    float slopeMag = length(vec2(dHdx, dHdy));
    
    // Normalize height to [0, 1] range
    float normalized = clamp((height - uMinHeight) / uMaxHeight, 0.0, 1.0);
    
    // Apply smoothstep for crisper trough/peak transitions
    float luminance = normalized * normalized * (3.0 - 2.0 * normalized);
    
    // Scale luminance: troughs dark (0.3), peaks near-white (1.0)
    float brightness = 0.3 + luminance * 0.7;
    
    // ---- Edge-concentrated grain texture ----
    // Texture is strongest where the slope is steep (transition edges).
    // Flat peaks and trough bottoms stay smooth.
    vec2 grainCoord = vTexCoord * uTableSize;
    float grain1 = valueNoise(grainCoord * 0.8) - 0.5;  // coarse grain
    float grain2 = valueNoise(grainCoord * 1.6) - 0.5;  // fine grain
    float grainNoise = grain1 * 0.6 + grain2 * 0.4;     // [-0.5, 0.5]
    
    // Slope drives texture intensity: no slope = no texture
    float edgeFactor = clamp(slopeMag * 4.0, 0.0, 1.0);
    float textureOffset = grainNoise * edgeFactor * 0.25; // up to ±12.5% brightness at edges
    
    brightness += textureOffset;
    brightness = clamp(brightness, 0.0, 1.0);
    
    // Final color = base color * brightness
    vec3 finalColor = uBaseColor * brightness;
    finalColor = clamp(finalColor, 0.0, 1.0);
    
    gl_FragColor = vec4(finalColor, 1.0);
}
`;
