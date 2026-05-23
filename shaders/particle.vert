#version 460 core
// Day 1 stub — full vertex shader (MVP transform, point size by depth) added Day 4.
layout(location = 0) in vec3 aPos;
void main() {
    gl_Position = vec4(aPos, 1.0);
}
