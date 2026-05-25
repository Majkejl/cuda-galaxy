#version 460 core
layout(location = 0) in vec3 aPos;

uniform mat4 uViewProj;

void main() {
    vec4 clip = uViewProj * vec4(aPos, 1.0);
    gl_Position = clip;
    // Perspective point scaling: clip.w == -z_eye, so nearer particles (smaller w) draw
    // larger. Clamp so near points don't balloon and far ones never vanish entirely.
    gl_PointSize = clamp(6.0 / clip.w, 1.0, 20.0);
}
