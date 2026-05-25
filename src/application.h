#pragma once

#include "simulation.h"
#include "renderer.h"
#include "camera.h"

struct GLFWwindow;   // forward-declared so the GLFW header stays out of this interface

// High-level orchestration: owns the render/sim loop, the camera, and input handling.
// Pure C++ — no CUDA headers. The GPU work is delegated to Simulation.
class Application {
public:
    explicit Application(GLFWwindow* window);

    void run();   // poll input, step the sim, render — until the window closes

    // GLFW event hooks, invoked via thin static thunks in application.cpp.
    void onScroll(double yOffset);
    void onResize(int width, int height);

private:
    void processInput();

    GLFWwindow* m_window;
    Simulation  m_sim;                   // owns all device buffers
    Renderer    m_renderer;
    Camera      m_camera;
    bool        m_paused       = false;
    bool        m_spaceWasDown = false;  // edge-detect SPACE so a held key toggles once

    // Mouse-drag orbit state.
    bool   m_dragging = false;
    double m_lastX    = 0.0;
    double m_lastY    = 0.0;
};
