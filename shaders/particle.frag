#version 460 core

layout(location = 1) in float vColorMix;   // normalized star-mass tint: 0 = yellow, 1 = blue
layout(location = 2) in float vSizeFade;   // from the vertex shader: energy-conserving size fade

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 TreshColor;

void main() {
    vec2 uv = gl_PointCoord.xy;
    float dist = distance(uv, vec2(0.5));

    // Screen-space width of the rim, computed BEFORE any discard so the 2x2 derivative quad
    // is intact (a discarded neighbour makes fwidth undefined). fwidth(dist) ≈ how much 'dist'
    // changes per pixel, so the fade below stays ~1px wide regardless of sprite size.
    float aa = fwidth(dist);
    if (dist > 0.5) discard;

    // Soft analytic edge: coverage falls 1→0 over the last ~pixel at the rim instead of a hard
    // binary cutoff — that cutoff was what aliased on the ~pixel-sized sprites.
    float coverage = 1.0 - smoothstep(0.5 - aa, 0.5, dist);

    // 1. Core Star Intensity
    // Map distance to a sharp, bright core (e.g., dist < 0.1)
    float core = 1.0 - smoothstep(0.0, 0.15, dist);

    // 2. Diffuse Glow
    // Map distance to a smooth halo (e.g., dist < 0.5)
    float glow = 1.0 - smoothstep(0.15, 0.5, dist);

    // Combine core and glow, then apply both AA terms: coverage softens the rim, vSizeFade
    // keeps a size-floored star's total light constant. Folding them into intensity makes the
    // scene color AND the bloom threshold (which is derived from intensity) consistent.
    float intensity = (core * 1.5 + glow * 0.5) * coverage * vSizeFade;
    
    // Warm (low-mass) → cool (high-mass) tint; vColorMix is already normalized to [0,1].
    // Warm end is a saturated amber-gold (like a cool K-type star) rather than pale cream, so the
    // low-mass majority reads as a distinct gold against the blue instead of washing toward white.
    vec3 starColor = mix(vec3(0.4, 0.2, 0.9), vec3(0.7, 0.95, 1.0), vColorMix) * intensity;
    
    FragColor = vec4(starColor, intensity);
    float factor = smoothstep(0.75, 0.85, intensity);
    TreshColor = mix(vec4(vec3(0.), 1.), vec4(starColor, 1.), factor);
}