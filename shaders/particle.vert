#version 460 core
layout(location = 0) in vec4 aPos;

uniform mat4 uViewProj;
uniform float uPointScale;
uniform float uMassMin;   // star-mass tint range = mean - k*sigma of the star distribution
uniform float uMassMax;   // star-mass tint range = mean + k*sigma; cores sit far above -> clamp to blue

layout(location = 1) out float vColorMix;   // 0 = low-mass (yellow), 1 = high-mass (blue)
layout(location = 2) out float vSizeFade;   // brightness scale that conserves a floored point's total light

void main() {
    vec4 clip = uViewProj * vec4(aPos.rgb, 1.0);
    gl_Position = clip;
    // Perspective point scaling: clip.w == -z_eye, so nearer particles (smaller w) draw
    // larger. Clamp the mass at the top of the star distribution (uMassMax = mean + 2*sigma)
    // so the far-heavier cores don't balloon — same data-derived ceiling the color tint uses,
    // not a hardcoded magic number. Local now (drives size only); the tint is normalized below.
    float weight = min(aPos.a, uMassMax) * 25000;

    // 'want' = the point's ideal on-screen diameter in pixels: 6/clip.w is the perspective
    // shrink (clip.w == -z_eye, so farther → larger w → smaller sprite), times the per-star
    // weight. Below minPx a sprite is too small to sample without flicker, so we floor its
    // size — but a floored star kept at full brightness twinkles as it slides across pixel
    // centers. So we also dim it by the AREA ratio (want/minPx)², making its TOTAL emitted
    // light invariant to the floor and steady under motion. want >= minPx → fade == 1 (no-op).
    const float minPx = 5.0 * uPointScale;
    float want = 10.0 / clip.w * weight * uPointScale;
    gl_PointSize = clamp(want, minPx, 20.0 * uPointScale);
    float fade   = clamp(want / minPx, 0.0, 1.0);
    vSizeFade    = fade * fade;

    // Mass tint. uMassMin/uMassMax bracket mean ± 2*sigma (the passed range is 4*sigma wide), but
    // saturating colour over the full ±2σ leaves most stars bunched near white at the mean. So we
    // tighten the colour window to the inner ±1σ and use smoothstep (an S-ramp: steeper than a
    // linear clamp near the mean, flat at the ends) so typical stars genuinely span yellow→blue.
    // smoothstep already clamps outside its edges, so the heavy cores still saturate to 1 (blue).
    // This is decoupled from the SIZE clamp above, which deliberately stays at the full ±2σ.
    float cMid  = 0.5  * (uMassMin + uMassMax);   // distribution mean
    float cHalf = 0.25 * (uMassMax - uMassMin);   // ≈ 1*sigma; shrink the factor for even more contrast
    vColorMix = smoothstep(cMid - cHalf, cMid + cHalf, aPos.a);
}