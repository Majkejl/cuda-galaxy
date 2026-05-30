#version 460 core

// Per-level separable Gaussian blur for the bloom mip pyramid. One axis per pass; the
// renderer runs it H then V on each mip level. It only ever blurs — the scene composite
// lives in bloom_combine.frag.
layout(binding = 1) uniform sampler2D input_tresh;   // the pyramid texture we read (level = uLod)

uniform int  uGaussSize;   // blur radius in texels; full 1D kernel is (2*uGaussSize + 1) taps
uniform vec2 uDirection;   // (1,0) = horizontal pass, (0,1) = vertical pass — the separable axis
uniform int  uLod;         // which mip level we're blurring (also the render target's level)

out vec4 FragColor;

// Unnormalized 1D Gaussian weight at tap offset x for a given std-dev. We normalize in
// main() by dividing by the summed weights, so the 1/(sigma*sqrt(2*pi)) scale is irrelevant.
float gaussWeight(int x, float sigma) {
    return exp(-float(x * x) / (2.0 * sigma * sigma));
}

void main() {
    // gl_FragCoord is in THIS level's pixel space because the renderer set the viewport to the
    // mip's size, so integer fetches line up 1:1 with the level we're writing.
    ivec2 coord = ivec2(gl_FragCoord.xy);
    ivec2 axis  = ivec2(uDirection);   // march along ONE axis only — that's what makes it separable

    float sigma = max(float(uGaussSize) * 0.5, 1e-4);   // guard sigma > 0 to avoid /0 at size 0

    vec3  sum   = vec3(0.0);
    float total = 0.0;   // running weight sum: GLSL can't size an array by a runtime uniform.

    for (int i = -uGaussSize; i <= uGaussSize; i++) {
        float w = gaussWeight(i, sigma);
        // The 3rd texelFetch arg is the explicit mip level — read the same level we render into.
        sum   += w * texelFetch(input_tresh, coord + axis * i, uLod).rgb;
        total += w;
    }

    FragColor = vec4(sum / total, 1.0);   // normalized blur of this level; no composite here
}
