#include "application.h"

#define GLFW_INCLUDE_NONE   // GL comes from glad, not GLFW's bundled headers
#include <glad/gl.h>
#include <GLFW/glfw3.h>

Application::Application(GLFWwindow* window)
    : m_window(window) {}

void Application::run() {
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    glViewport(0, 0, w, h);

    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        processInput();

        if (!m_paused)
            m_sim.step();

        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Particle rendering arrives in step 3b (VBO draw) / 3c (CUDA-GL interop).

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
}
