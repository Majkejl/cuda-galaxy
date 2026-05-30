#pragma once
// Renderer: owns the VAO/VBO + shader program and draws the particles. The VBO is shared
// with CUDA (registered in Simulation); here we only touch it as a GL vertex source.

#include <filesystem>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

constexpr float clear_color[4] = {0.f, 0.f, 0.f, 1.f};
constexpr float clear_depth[1] = {1.f};

class Renderer {
public:
    Renderer(const void *data, int n);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void draw(int count, const glm::mat4& viewProj);

    void resize(int width, int height);   // (re)create the FBO textures to match the framebuffer size
    void adjustBlur(int delta);           // step the per-level blur radius (clamped); keybind-driven
    void adjustLevels(int delta);         // step how many pyramid levels are summed (clamped); keybind-driven

    // Star-mass tint range (mean ± 2*sigma), supplied by Simulation from the galaxy distribution.
    void setStarMassRange(float lo, float hi) { m_starMassMin = lo; m_starMassMax = hi; }

    unsigned int vboId() const { return vbo; }
private:
    void compileShaders();

    static constexpr int kBloomLevels = 6;   // ceiling on pyramid levels we ever allocate

    GLuint vao;
    GLuint vbo;

    GLuint prog;       // particle program
    GLuint post;       // per-level separable blur (bloom.frag)
    GLuint combine;    // sum levels + composite (bloom_combine.frag)

    GLuint framebuffer;
    GLuint framebuffer_color = 0;   // 0-init: resize() deletes these before first (re)creation,
    GLuint framebuffer_tresh = 0;   // and glDeleteTextures(0) is a defined no-op

    GLuint bloom_fbo;               // reusable post FBO; re-pointed at a (texture, level) per pass
    GLuint bloom_tex = 0;           // mipmapped pyramid: holds the per-level blurred bloom
    GLuint bloom_tmp = 0;           // mipmapped ping-pong: the horizontal pass writes here

    int    m_fbWidth  = 0;          // window/output size; resize() skips redundant recreation
    int    m_fbHeight = 0;
    int    m_renderWidth  = 0;      // supersampled offscreen size (= m_ssaa * m_fbWidth); scene+bloom run here
    int    m_renderHeight = 0;
    int    m_bloomMipLevels = 0;    // levels actually allocated for the current size (<= kBloomLevels)

    int    m_ssaa = 2;              // SSAA factor: offscreen targets are this many times bigger per axis

    GLuint sh_vert;
    GLuint sh_frag;
    GLuint sh_post;
    GLuint sh_postVert;
    GLuint sh_combine;

    GLint  m_uViewProj = -1;   // cached location of the uViewProj uniform

    GLint  m_uGaussSize = -1;  // blur program (post) uniform locations
    GLint  m_uDirection = -1;
    GLint  m_uLod       = -1;

    GLint  m_cLevels    = -1;  // combine program uniform locations
    GLint  m_cStrength  = -1;

    GLint m_uPointScale = -1;
    GLint m_uMassMin    = -1;   // star-mass tint range (particle program)
    GLint m_uMassMax    = -1;
    GLint m_cSS = -1;

    float m_starMassMin = 0.f;  // mean - 2*sigma; set by setStarMassRange()
    float m_starMassMax = 1.f;  // mean + 2*sigma (also the size clamp in the vertex shader)

    int    m_gaussSize     = 12;     // per-level blur radius (small — the pyramid gives the spread)
    int    m_bloomLevels   = 6;     // pyramid levels summed at runtime (clamped to 1..allocated)
    float  m_bloomStrength = 3.0f;  // composite gain on the bloom; tune to taste
};
