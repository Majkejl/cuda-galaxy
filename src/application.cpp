#include "application.h"

#define GLFW_INCLUDE_NONE   // GL comes from glad, not GLFW's bundled headers
#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace {
    // GLFW takes C function pointers, so we route its callbacks back to the Application
    // instance stashed in the window user pointer.
    void scrollThunk(GLFWwindow* w, double /*xoff*/, double yoff) {
        if (auto* app = static_cast<Application*>(glfwGetWindowUserPointer(w)))
            app->onScroll(yoff);
    }
    void framebufferSizeThunk(GLFWwindow* w, int width, int height) {
        if (auto* app = static_cast<Application*>(glfwGetWindowUserPointer(w)))
            app->onResize(width, height);
    }

    constexpr float kOrbitSensitivity = 0.005f;   // radians of orbit per pixel of drag
}

Application::Application(GLFWwindow* window)
    : m_window(window), m_sim(),
    m_renderer(m_sim.initialPositions(), m_sim.particleCount()) {
    m_sim.registerVBO(m_renderer.vboId());

    // Route GLFW events to this instance and seed the viewport + camera aspect.
    glfwSetWindowUserPointer(m_window, this);
    glfwSetScrollCallback(m_window, scrollThunk);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeThunk);

    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    onResize(w, h);
}

void Application::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        processInput();

        if (!m_paused)
            m_sim.step();

        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        m_renderer.draw(m_sim.particleCount(), m_camera.viewProjection());

        glfwSwapBuffers(m_window);
    }
}

void Application::processInput() {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);

    const bool spaceDown = glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (spaceDown && !m_spaceWasDown)
        m_paused = !m_paused;
    m_spaceWasDown = spaceDown;

    // Left-drag orbits. Only apply the delta when a drag is already in progress (it was
    // held last frame too), so the initial click doesn't snap the view by a stale delta.
    double mx, my;
    glfwGetCursorPos(m_window, &mx, &my);
    const bool lmb = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (lmb && m_dragging) {
        const float dx = static_cast<float>(mx - m_lastX);
        const float dy = static_cast<float>(my - m_lastY);
        // Negate so the scene follows the cursor: drag right → world spins right.
        m_camera.orbit(-dx * kOrbitSensitivity, -dy * kOrbitSensitivity);
    }
    m_dragging = lmb;
    m_lastX = mx;
    m_lastY = my;
}

void Application::onScroll(double yOffset) {
    m_camera.zoom(static_cast<float>(yOffset));
}

void Application::onResize(int width, int height) {
    glViewport(0, 0, width, height);
    if (height > 0)
        m_camera.setAspect(static_cast<float>(width) / static_cast<float>(height));
}
