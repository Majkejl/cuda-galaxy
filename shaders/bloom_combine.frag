#version 460 core

// Combine pass: sum the blurred bloom mip levels (each upscaled to full res) and add the
// scene back in. Coarse levels were blurred at low res, so a small kernel there covers a
// large on-screen area — stacking the levels gives the wide, bright, soft bloom.
layout(binding = 0) uniform sampler2D sceneTex;   // framebuffer_color (the sharp scene)
layout(binding = 1) uniform sampler2D bloomTex;   // the mipmapped, per-level blurred pyramid

uniform int   uLevels;         // how many pyramid levels to sum (runtime control, 1..allocated)
uniform float uBloomStrength;   // overall gain on the summed bloom

out vec4 FragColor;

void main() {
    // Normalized UV over the full-res image; textureLod reads a chosen level and the sampler's
    // LINEAR filtering upscales the coarse levels smoothly back to full resolution.
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(bloomTex, 0));

    vec3 bloom = vec3(0.0);
    for (int L = 0; L < uLevels; ++L) {
        bloom += textureLod(bloomTex, uv, float(L)).rgb;
    }

    vec3 scene = texelFetch(sceneTex, ivec2(gl_FragCoord.xy), 0).rgb;   // 1:1, no filtering
    FragColor = vec4(scene + uBloomStrength * bloom, 1.0);
}
