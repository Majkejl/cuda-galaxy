#pragma once
// Day 1 stub — Renderer class (VBO, CUDA-GL interop, shaders, bloom) added Day 4.

#include <filesystem>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

class Renderer {
public:
    Renderer(const void *data, int n);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void draw(int count);

    unsigned int vboId() const { return vbo; }
private:
    void compileShaders();

    GLuint vao;
    GLuint vbo;

    GLuint prog;

    GLuint sh_vert;
    GLuint sh_frag;
    GLuint sh_post;
};