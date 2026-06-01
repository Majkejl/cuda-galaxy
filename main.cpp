// Entry point only: create the GL context/window, then hand off to Application.
// All simulation and CUDA logic lives elsewhere (application.cpp / simulation.cpp).
#define GLFW_INCLUDE_NONE   // we load OpenGL through glad, not GLFW's bundled headers
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "application.h"
#include "benchmark.h"

// Hybrid-GPU (Optimus) hint: exporting these symbols from the EXE tells the NVIDIA (and
// AMD) driver to launch us on the high-performance discrete GPU. CUDA-GL interop REQUIRES
// GL and CUDA on the same GPU, so we must be on the dGPU — not the Intel iGPU the laptop
// panel is wired to. Only takes effect when exported from the executable's own module.
#if defined(_WIN32)
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement                  = 0x00000001;
    __declspec(dllexport) int           AmdPowerXpressRequestHighPerformance = 1;
}
#endif

static void glfwErrorCallback(int code, const char* desc) {
    std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
}

// Bring up GLFW + a 4.6-core context + GLAD. Shared by the interactive (visible) and the
// film (hidden) paths — the only difference between them is GLFW_VISIBLE. Returns the window,
// or nullptr (already torn down) on failure. glfwInit() is a no-op if already initialized, so
// it's safe even though only one mode runs per process. The caller sets glfwSwapInterval.
static GLFWwindow* createContext(int width, int height, bool visible) {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        std::fprintf(stderr, "glfwInit failed\n");
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(width, height, "cuda-nbody", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "window creation failed\n");
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    // GLAD must load the GL function pointers AFTER a context is current and BEFORE any GL call.
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::fprintf(stderr, "gladLoadGL failed\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return nullptr;
    }
    return window;
}

int render() {
    GLFWwindow* window = createContext(1280, 720, /*visible=*/true);
    if (!window) return EXIT_FAILURE;
    glfwSwapInterval(1);   // vsync

    {
        // Scoped so Application (and its device buffers) is destroyed while the GL
        // context is still valid — important once it owns interop resources.
        Application app(window);
        app.run();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// Offline render: a hidden, tiny window just supplies the GL context — every frame is drawn
// into the renderer's offscreen FBO at p.outW x p.outH (independent of the window/monitor),
// read back, and piped to ffmpeg. No vsync, no input, no buffer swap; decoupled from real time.
int film(const FilmParams_t& p) {
    GLFWwindow* window = createContext(16, 16, /*visible=*/false);
    if (!window) return EXIT_FAILURE;
    glfwSwapInterval(0);   // we never present, but make the intent explicit

    {
        Application app(window);   // scoped like render(): device buffers die while GL is alive
        app.recordFilm(p);         // the VideoWriter inside finalizes the mp4 on return
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

int benchmark(int iters) {
    Benchmark b;
    b.run(iters);
    return 0;
}

// --- film argv parsing helpers: each parses a WHOLE token (no trailing junk) ---
static bool parseInt(const std::string& s, int& out) {
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
    return ec == std::errc() && ptr == s.data() + s.size();
}
static bool parseFloat(const std::string& s, float& out) {
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
    return ec == std::errc() && ptr == s.data() + s.size();
}
// "WxH" (e.g. 1920x1080) → w, h. Accepts 'x' or 'X'.
static bool parseRes(const std::string& s, int& w, int& h) {
    const auto sep = s.find_first_of("xX");
    if (sep == std::string::npos) return false;
    return parseInt(s.substr(0, sep), w) && parseInt(s.substr(sep + 1), h) && w > 0 && h > 0;
}

int main(int argc, char** argv) {
    std::vector<std::string> arguments(argv, argv + argc);

    // "film [flags]" → offline render to a video file. A non-numeric keyword can't collide with
    // the benchmark parse below. Each flag maps to a FilmParams_t member and takes one value token:
    //   --res WxH   --frames N   --fps N   --speed N   --orbit F   --out PATH
    // Anything unset keeps the struct's default.
    if (argc > 1 && arguments[1] == "film") {
        FilmParams_t p;
        auto usage = [] {
            std::fprintf(stderr,
                "usage: cuda_nbody film [--res WxH] [--frames N] [--fps N] "
                "[--speed N] [--orbit F] [--out PATH]\n");
            return EXIT_FAILURE;
        };
        for (size_t i = 2; i < arguments.size(); ++i) {
            const std::string& flag = arguments[i];
            if (i + 1 >= arguments.size()) {
                std::fprintf(stderr, "missing value for %s\n", flag.c_str());
                return usage();
            }
            const std::string& val = arguments[++i];   // consume the value token

            if (flag == "--res") {
                if (!parseRes(val, p.outW, p.outH)) {
                    std::fprintf(stderr, "bad --res '%s' (expected WxH, e.g. 1920x1080)\n", val.c_str());
                    return usage();
                }
            } else if (flag == "--frames") {
                if (!parseInt(val, p.frames) || p.frames <= 0) {
                    std::fprintf(stderr, "bad --frames '%s' (expected positive int)\n", val.c_str());
                    return usage();
                }
            } else if (flag == "--fps") {
                if (!parseInt(val, p.fps) || p.fps <= 0) {
                    std::fprintf(stderr, "bad --fps '%s' (expected positive int)\n", val.c_str());
                    return usage();
                }
            } else if (flag == "--speed") {
                if (!parseInt(val, p.speed) || p.speed <= 0) {
                    std::fprintf(stderr, "bad --speed '%s' (expected positive int, sim steps/frame)\n", val.c_str());
                    return usage();
                }
            } else if (flag == "--orbit") {
                if (!parseFloat(val, p.orbit)) {
                    std::fprintf(stderr, "bad --orbit '%s' (expected float, radians/frame)\n", val.c_str());
                    return usage();
                }
            } else if (flag == "--out") {
                p.out = val;
            } else {
                std::fprintf(stderr, "unknown flag '%s'\n", flag.c_str());
                return usage();
            }
        }
        return film(p);
    }

    // argv[1] = number of benchmark iterations. Anything that isn't a positive
    // integer (or no arg at all) falls through to interactive rendering.
    if (argc > 1) {
        int iters = 0;
        const std::string& arg = arguments[1];
        auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), iters);
        if (ec == std::errc() && ptr == arg.data() + arg.size() && iters > 0)
            return benchmark(iters);
    }
    return render();
}
