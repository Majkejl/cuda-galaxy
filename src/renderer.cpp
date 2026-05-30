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

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    // shaders
    compileShaders();

    // Let the vertex shader drive point size via gl_PointSize (for perspective scaling).
    // In a core profile this is off by default, so it must be explicitly enabled.
    glEnable(GL_PROGRAM_POINT_SIZE);

    // MRT framebuffer: attachment 0 = scene color, attachment 1 = bloom threshold. The
    // draw-buffer mapping is part of the FBO's state and survives re-attaching textures,
    // so it's set once here; the textures themselves are sized later by resize().
    glCreateFramebuffers(1, &framebuffer);
    const GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glNamedFramebufferDrawBuffers(framebuffer, 2, draw_buffers);

    // One reusable framebuffer for every post pass. We re-point its color attachment at a
    // specific (texture, mip level) before each blur draw, so a single FBO covers the whole
    // pyramid. Its textures live in bloom_tex / bloom_tmp, sized by resize().
    glCreateFramebuffers(1, &bloom_fbo);

    // All FBO textures are sized to the actual framebuffer (no hardcoded dimensions): the
    // Application calls resize() once at startup and again on every window resize.
};

Renderer::~Renderer() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    
    glDeleteProgram(prog);
    glDeleteProgram(post);
    glDeleteProgram(combine);

    glDeleteTextures(1, &framebuffer_color);
    glDeleteTextures(1, &framebuffer_tresh);
    glDeleteFramebuffers(1, &framebuffer);

    glDeleteTextures(1, &bloom_tex);
    glDeleteTextures(1, &bloom_tmp);
    glDeleteFramebuffers(1, &bloom_fbo);
}

void Renderer::resize(int width, int height) {
    if (width <= 0 || height <= 0) return;                 // ignore minimize (0-sized framebuffer)
    if (width == m_fbWidth && height == m_fbHeight) return; // GLFW re-fires resize; skip no-op churn
    m_fbWidth = width;
    m_fbHeight = height;

    width *= m_ssaa;
    height *= m_ssaa;
    m_renderWidth  = width;    // everything offscreen (scene draw, blit, blur) sizes off these
    m_renderHeight = height;

    // glTextureStorage2D allocates IMMUTABLE storage — you can't resize it in place, so the
    // only way to change dimensions is to delete and recreate the texture objects. The names
    // start at 0 (header init), and glDeleteTextures ignores 0, so the first call is safe.
    glDeleteTextures(1, &framebuffer_color);
    glDeleteTextures(1, &framebuffer_tresh);
    glDeleteTextures(1, &bloom_tex);
    glDeleteTextures(1, &bloom_tmp);

    // Scene color + threshold: single-level, RGBA32F so HDR bright values survive un-clamped.
    glCreateTextures(GL_TEXTURE_2D, 1, &framebuffer_color);
    glTextureStorage2D(framebuffer_color, 1, GL_RGBA32F, width, height);
    glCreateTextures(GL_TEXTURE_2D, 1, &framebuffer_tresh);
    glTextureStorage2D(framebuffer_tresh, 1, GL_RGBA32F, width, height);

    // How many pyramid levels this size supports, capped at the ceiling. Level L is
    // floor(base / 2^L); stop before either dimension would shift to 0.
    m_bloomMipLevels = 1;
    while (m_bloomMipLevels < kBloomLevels &&
           (width >> m_bloomMipLevels) > 0 && (height >> m_bloomMipLevels) > 0)
        ++m_bloomMipLevels;
    if (m_bloomLevels > m_bloomMipLevels) m_bloomLevels = m_bloomMipLevels;  // keep runtime use valid

    // The pyramid + its ping-pong twin. LINEAR_MIPMAP_NEAREST + LINEAR mag so the combine
    // pass's textureLod upscales each coarse level smoothly; CLAMP_TO_EDGE avoids border bleed.
    for (GLuint* tex : {&bloom_tex, &bloom_tmp}) {
        glCreateTextures(GL_TEXTURE_2D, 1, tex);
        glTextureStorage2D(*tex, m_bloomMipLevels, GL_RGBA32F, width, height);
        glTextureParameteri(*tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTextureParameteri(*tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(*tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(*tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // Re-point the MRT at the fresh color/threshold textures (old attachments are now dangling).
    // bloom_fbo is re-attached per pass in draw(), so it needs nothing here.
    glNamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, framebuffer_color, 0);
    glNamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT1, framebuffer_tresh, 0);
}

void Renderer::adjustBlur(int delta) {
    m_gaussSize += delta;
    // Clamp: 0 = blur off (single tap), and cap the radius so a held key can't push the
    // per-fragment tap count (2*r+1, twice) into pathological territory.
    if (m_gaussSize < 0)  m_gaussSize = 0;
    if (m_gaussSize > 64) m_gaussSize = 64;
}

void Renderer::adjustLevels(int delta) {
    m_bloomLevels += delta;
    // At least 1 level (≈ the tight, full-res bloom); never more than what's allocated for the
    // current window size (m_bloomMipLevels, itself <= kBloomLevels). draw() also re-clamps.
    if (m_bloomLevels < 1) m_bloomLevels = 1;
    if (m_bloomLevels > m_bloomMipLevels && m_bloomMipLevels > 0) m_bloomLevels = m_bloomMipLevels;
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

// Link is where stage-mismatch bugs land (e.g. two vertex shaders / no fragment stage) —
// each file can compile fine yet still fail to form a valid pipeline. Throw with the log so
// a bad program surfaces loudly instead of silently rendering black.
static void checkLink(GLuint program, const char* what) {
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetProgramInfoLog(program, logLen, nullptr, log.data());
        throw std::runtime_error(std::string("program link failed (") + what + "):\n" + log);
    }
}

void Renderer::compileShaders() {
    sh_vert     = compShader(GL_VERTEX_SHADER,   RESOURCE_DIR "/particle.vert");
    sh_frag     = compShader(GL_FRAGMENT_SHADER, RESOURCE_DIR "/particle.frag");
    sh_post     = compShader(GL_FRAGMENT_SHADER, RESOURCE_DIR "/bloom.frag");
    sh_postVert = compShader(GL_VERTEX_SHADER,   RESOURCE_DIR "/bloom.vert");
    sh_combine  = compShader(GL_FRAGMENT_SHADER, RESOURCE_DIR "/bloom_combine.frag");

    prog = glCreateProgram();
    glAttachShader(prog, sh_vert);
    glAttachShader(prog, sh_frag);
    glLinkProgram(prog);

    // post and combine share the fullscreen-triangle vertex shader (sh_postVert). Attaching
    // one shader object to two programs is fine — it stays alive until both release it.
    post = glCreateProgram();
    glAttachShader(post, sh_post);
    glAttachShader(post, sh_postVert);
    glLinkProgram(post);

    combine = glCreateProgram();
    glAttachShader(combine, sh_combine);
    glAttachShader(combine, sh_postVert);
    glLinkProgram(combine);

    // Safe to flag the shaders for deletion now: while attached they stay alive, and they're
    // actually freed when the last program using them is. The linked programs keep their own
    // copy of the compiled code, so the link-status checks below are unaffected.
    glDeleteShader(sh_vert);
    glDeleteShader(sh_frag);
    glDeleteShader(sh_post);
    glDeleteShader(sh_postVert);
    glDeleteShader(sh_combine);

    checkLink(prog,    "particle");
    checkLink(post,    "bloom blur");
    checkLink(combine, "bloom combine");

    // Cache uniform locations once. -1 would mean GLSL optimized it out (e.g. unused). Sampler
    // bindings are fixed by layout(binding=...) in the shaders, so they need no location here.
    m_uViewProj   = glGetUniformLocation(prog, "uViewProj");      // particle program (prog) uniforms
    m_uPointScale = glGetUniformLocation(prog, "uPointScale");    // SSAA point-size scale — must come from prog
    m_uMassMin    = glGetUniformLocation(prog, "uMassMin");       // star-mass tint range (also the size clamp)
    m_uMassMax    = glGetUniformLocation(prog, "uMassMax");

    m_uGaussSize = glGetUniformLocation(post, "uGaussSize");   // per-level blur program
    m_uDirection = glGetUniformLocation(post, "uDirection");
    m_uLod       = glGetUniformLocation(post, "uLod");

    m_cLevels    = glGetUniformLocation(combine, "uLevels");   // combine/composite program
    m_cStrength  = glGetUniformLocation(combine, "uBloomStrength");
    m_cSS        = glGetUniformLocation(combine, "uSS");
}

void Renderer::draw(int count, const glm::mat4& viewProj) {

    glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearNamedFramebufferfv(framebuffer, GL_COLOR, 0, clear_color);
    glClearNamedFramebufferfv(framebuffer, GL_COLOR, 1, clear_color);
    
    glUseProgram(prog);
    glUniformMatrix4fv(m_uViewProj, 1, GL_FALSE, glm::value_ptr(viewProj));
    glUniform1f(m_uPointScale, static_cast<float>(m_ssaa));
    glUniform1f(m_uMassMin, m_starMassMin);   // constant, but cheap to re-send each frame
    glUniform1f(m_uMassMax, m_starMassMax);
    glBindVertexArray(vao);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glViewport(0, 0, m_renderWidth, m_renderHeight);   // draw the scene at the supersampled size

    glDrawArrays(GL_POINTS, 0, count);


    // ---- Post: mip-pyramid bloom (downsample → per-level blur → upsample-sum + composite) ----
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // (1) Copy the threshold into the pyramid's level 0, then box-downsample it into the
    //     coarser levels in one call. generateMipmap MUST precede the blur, or it would
    //     overwrite the levels we're about to blur.
    glNamedFramebufferReadBuffer(framebuffer, GL_COLOR_ATTACHMENT1);   // source = the threshold attachment
    glNamedFramebufferTexture(bloom_fbo, GL_COLOR_ATTACHMENT0, bloom_tex, 0);
    glBlitNamedFramebuffer(framebuffer, bloom_fbo,
                           0, 0, m_renderWidth, m_renderHeight,   // threshold + pyramid are both render-size now
                           0, 0, m_renderWidth, m_renderHeight,
                           GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glGenerateTextureMipmap(bloom_tex);

    const int levels = (m_bloomLevels < m_bloomMipLevels) ? m_bloomLevels : m_bloomMipLevels;
    
    // (2) Separable Gaussian on each level we'll use. Per level: set the viewport to the mip's
    //     size, H pass bloom_tex[L] → bloom_tmp[L], V pass bloom_tmp[L] → bloom_tex[L]. Reading
    //     and writing different textures keeps each pass free of a feedback loop.
    glUseProgram(post);
    glUniform1i(m_uGaussSize, m_gaussSize);
    glBindFramebuffer(GL_FRAMEBUFFER, bloom_fbo);
    for (int L = 0; L < levels; ++L) {
        const int lw = (m_renderWidth  >> L) > 0 ? (m_renderWidth  >> L) : 1;
        const int lh = (m_renderHeight >> L) > 0 ? (m_renderHeight >> L) : 1;
        glViewport(0, 0, lw, lh);   // gl_FragCoord must span THIS level of the render-size pyramid
        glUniform1i(m_uLod, L);

        // Horizontal: read level L of the pyramid, write level L of the ping-pong.
        glNamedFramebufferTexture(bloom_fbo, GL_COLOR_ATTACHMENT0, bloom_tmp, L);
        glUniform2f(m_uDirection, 1.0f, 0.0f);
        glBindTextureUnit(1, bloom_tex);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Vertical: read level L of the ping-pong, write back into level L of the pyramid.
        glNamedFramebufferTexture(bloom_fbo, GL_COLOR_ATTACHMENT0, bloom_tex, L);
        glUniform2f(m_uDirection, 0.0f, 1.0f);
        glBindTextureUnit(1, bloom_tmp);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    // (3) Combine to the screen: sum the blurred levels (linear-upscaled) and add the scene.
    glViewport(0, 0, m_fbWidth, m_fbHeight);   // restore full-res viewport for the final pass
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, m_fbWidth, m_fbHeight);
    glUseProgram(combine);
    glUniform1i(m_cLevels, levels);
    glUniform1i(m_cSS, m_ssaa);
    glUniform1f(m_cStrength, m_bloomStrength);
    glBindTextureUnit(0, framebuffer_color);   // sceneTex = sharp scene color
    glBindTextureUnit(1, bloom_tex);           // bloomTex = mipmapped blurred pyramid
    glDrawArrays(GL_TRIANGLES, 0, 3);
}