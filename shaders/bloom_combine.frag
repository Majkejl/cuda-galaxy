#version 460 core

// Combine pass: sum the blurred bloom mip levels (each upscaled to full res) and add the
// scene back in. Coarse levels were blurred at low res, so a small kernel there covers a
// large on-screen area — stacking the levels gives the wide, bright, soft bloom.
layout(binding = 0) uniform sampler2D sceneTex;   // framebuffer_color (the sharp scene)
layout(binding = 1) uniform sampler2D bloomTex;   // the mipmapped, per-level blurred pyramid

uniform int   uLevels;         // how many pyramid levels to sum (runtime control, 1..allocated)
uniform float uBloomStrength;   // overall gain on the summed bloom
uniform int uSS;

out vec4 FragColor;

void main() {
    // Normalized UV over the full-res image; textureLod reads a chosen level and the sampler's
    // LINEAR filtering upscales the coarse levels smoothly back to full resolution.
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(bloomTex, 0));

    vec3 bloom = vec3(0.0);
    for (int L = 0; L < uLevels; ++L) {
        bloom += textureLod(bloomTex, uv * uSS, float(L)).rgb;
    }

    // SSAA resolve: box-average the uSS×uSS block of supersamples that maps to this output
    // pixel. Multiply the INTEGER pixel coord (not the pixel-centered gl_FragCoord) so the
    // block origin is exactly uSS*px — otherwise the +0.5 center shifts us half a block and
    // reads out of bounds at the top/right edge.
    ivec2 base = ivec2(gl_FragCoord.xy) * uSS;
    vec3 scene = vec3(0.0);
    for (int y = 0; y < uSS; ++y) {
        for (int x = 0; x < uSS; ++x) {
            scene += texelFetch(sceneTex, base + ivec2(x, y), 0).rgb;
        }
    }
    scene /= float(uSS * uSS);   // mean, not sum — a box downsample is the average of the block

    FragColor = vec4(scene + uBloomStrength * bloom, 1.0);
}
