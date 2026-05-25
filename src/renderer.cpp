// Day 1 stub — implementation added Day 4.
#include <string>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>   // glm::value_ptr — hand a mat4 to glUniformMatrix4fv
#include "renderer.h"
#include "utils.h"

Renderer::Renderer(const void *data, int n) {
    // VAO + VBO setup
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, n * 4 * sizeof(float), data, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    // shaders
    compileShaders();

    // Let the vertex shader drive point size via gl_PointSize (for perspective scaling).
    // In a core profile this is off by default, so it must be explicitly enabled.
    glEnable(GL_PROGRAM_POINT_SIZE);
};

Renderer::~Renderer() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(prog);
}

static GLuint compShader(GLenum type, const std::filesystem::path &source) {
    GLuint shader = glCreateShader(type);
    const std::string shader_string = readFile(source);
    const char *shader_source = shader_string.data();
    glShaderSource(shader, 1, &shader_source, nullptr);
    glCompileShader(shader);

    // glCompileShader never fails the call itself — it compiles into the shader object
    // and stashes the result. We must poll GL_COMPILE_STATUS, or a broken shader sails
    // through as a useless object and only surfaces (cryptically) at link time.
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);   // length includes the null terminator
        std::string log(logLen, '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        glDeleteShader(shader);                               // don't leak the object we're abandoning
        throw std::runtime_error("shader compile failed (" + source.string() + "):\n" + log);
    }
    return shader;
}

void Renderer::compileShaders() {
    sh_vert = compShader(GL_VERTEX_SHADER, RESOURCE_DIR "/particle.vert");
    sh_frag = compShader(GL_FRAGMENT_SHADER, RESOURCE_DIR "/particle.frag");
    // sh_post = compShader(GL_FRAGMENT_SHADER, RESOURCE_DIR "/bloom.frag");

    prog = glCreateProgram();
    glAttachShader(prog, sh_vert);
    glAttachShader(prog, sh_frag);
    
    glLinkProgram(prog);

    // Safe to flag the shaders for deletion now: while attached they stay alive, and
    // they're actually freed when the program is. The linked program keeps its own copy
    // of the compiled code, so the link-status query below is unaffected.
    glDeleteShader(sh_vert);
    glDeleteShader(sh_frag);

    // Link is where stage-mismatch bugs land (e.g. two vertex shaders / no fragment
    // stage) — each file can compile fine yet still fail to form a valid pipeline.
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetProgramInfoLog(prog, logLen, nullptr, log.data());
        glDeleteProgram(prog);
        throw std::runtime_error("program link failed:\n" + log);
    }

    // Cache the uniform location once. -1 would mean GLSL optimized it out (e.g. unused).
    m_uViewProj = glGetUniformLocation(prog, "uViewProj");
}

void Renderer::draw(int count, const glm::mat4& viewProj) {
    glUseProgram(prog);
    glUniformMatrix4fv(m_uViewProj, 1, GL_FALSE, glm::value_ptr(viewProj));
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, count);

    // TODO add post process pass
}