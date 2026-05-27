#version 460 core
layout(location = 0) in vec4 aPos;

uniform mat4 uViewProj;

layout(location = 1) out float weight;

void main() {
    vec4 clip = uViewProj * vec4(aPos.rgb, 1.0);
    gl_Position = clip;
    // Perspective point scaling: clip.w == -z_eye, so nearer particles (smaller w) draw
    // larger. Clamp so near points don't balloon and far ones never vanish entirely.
    weight = min(aPos.a, 0.0001) * 25000;
    gl_PointSize = clamp(6.0 / clip.w, 2.0, 20.0) * weight;
}
