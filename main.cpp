// Entry point only: create the GL context/window, then hand off to Application.
// All simulation and CUDA logic lives elsewhere (application.cpp / simulation.cu).
#define GLFW_INCLUDE_NONE   // we load OpenGL through glad, not GLFW's bundled headers
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>

#include "application.h"

static void glfwErrorCallback(int code, const char* desc) {
    std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
}

int main() {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        std::fprintf(stderr, "glfwInit failed\n");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "cuda-nbody", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "window creation failed\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);   // vsync

    // GLAD must load the GL function pointers AFTER a context is current and BEFORE
    // any GL call (including the VBO creation that arrives in step 3b).
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::fprintf(stderr, "gladLoadGL failed\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

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
