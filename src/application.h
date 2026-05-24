#pragma once

#include "simulation.h"

struct GLFWwindow;   // forward-declared so the GLFW header stays out of this interface

// High-level orchestration: owns the render/sim loop and input handling.
// Pure C++ — no CUDA headers. The GPU work is delegated to Simulation.
class Application {
public:
    explicit Application(GLFWwindow* window);

    void run();   // poll input, step the sim, render — until the window closes

private:
    void processInput();

    GLFWwindow* m_window;
    Simulation  m_sim;                   // owns all device buffers
    bool        m_paused       = false;
    bool        m_spaceWasDown = false;  // edge-detect SPACE so a held key toggles once
};
