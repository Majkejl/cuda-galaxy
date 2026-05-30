#version 460 core
layout(location = 0) in vec4 aPos;

uniform mat4 uViewProj;

layout(location = 1) out float weight;
layout(location = 2) out float vSizeFade;   // brightness scale that conserves a floored point's total light

void main() {
    vec4 clip = uViewProj * vec4(aPos.rgb, 1.0);
    gl_Position = clip;
    // Perspective point scaling: clip.w == -z_eye, so nearer particles (smaller w) draw
    // larger. Clamp so near points don't balloon and far ones never vanish entirely.
    weight = min(aPos.a, 0.0001) * 25000;

    // 'want' = the point's ideal on-screen diameter in pixels: 6/clip.w is the perspective
    // shrink (clip.w == -z_eye, so farther → larger w → smaller sprite), times the per-star
    // weight. Below minPx a sprite is too small to sample without flicker, so we floor its
    // size — but a floored star kept at full brightness twinkles as it slides across pixel
    // centers. So we also dim it by the AREA ratio (want/minPx)², making its TOTAL emitted
    // light invariant to the floor and steady under motion. want >= minPx → fade == 1 (no-op).
    const float minPx = 2.0;
    float want = 6.0 / clip.w * weight;
    gl_PointSize = clamp(want, minPx, 20.0);
    float fade   = clamp(want / minPx, 0.0, 1.0);
    vSizeFade    = fade * fade;
}