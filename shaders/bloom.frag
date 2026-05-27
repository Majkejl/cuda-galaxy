#version 460 core

layout(binding = 0) uniform sampler2D input_image;

out vec4 FragColor;
void main() {
    FragColor = vec4(1.0);
}
