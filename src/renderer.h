#pragma once
// Renderer: owns the VAO/VBO + shader program and draws the particles. The VBO is shared
// with CUDA (registered in Simulation); here we only touch it as a GL vertex source.

#include <filesystem>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

constexpr float clear_color[4] = {0.1f, 0.1f, 0.3f, 1.f};
constexpr float clear_depth[1] = {1.f};

class Renderer {
public:
    Renderer(const void *data, int n);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void draw(int count, const glm::mat4& viewProj);

    unsigned int vboId() const { return vbo; }
private:
    void compileShaders();

    GLuint vao;
    GLuint vbo;

    GLuint prog;
    GLuint post;

    GLuint framebuffer;
    GLuint framebuffer_color;
    GLuint framebuffer_depth;

    GLuint sh_vert;
    GLuint sh_frag;
    GLuint sh_post;
    GLuint sh_postVert;

    GLint  m_uViewProj = -1;   // cached location of the uViewProj uniform
};
