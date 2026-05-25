#pragma once
#include <glm/glm.hpp>

// Orbit ("arcball-lite") camera. It always looks at m_target and sits on a sphere around
// it, parameterized by yaw/pitch/distance — drag orbits, scroll dollies in/out. Pure math:
// no GL, GLFW, or CUDA here, so it stays trivially testable and reusable.
class Camera {
public:
    explicit Camera(float aspect = 16.0f / 9.0f);

    void orbit(float dYawRad, float dPitchRad);   // apply a mouse-drag delta (radians)
    void zoom(float scrollSteps);                 // apply scroll-wheel steps (+ = closer)
    void setAspect(float aspect);                 // call on window resize

    glm::mat4 viewProjection() const;             // proj * view, ready for the shader uniform
    glm::vec3 eye() const;                         // world-space camera position

private:
    glm::vec3 m_target{0.0f};        // the point we orbit around
    float m_distance = 3.0f;         // radius from target
    float m_yaw      = 0.0f;         // radians, longitude around +Y
    float m_pitch    = 0.35f;        // radians, latitude above the xz-plane
    float m_aspect   = 16.0f / 9.0f;
    float m_fovY     = glm::radians(45.0f);
    float m_near     = 0.05f;
    float m_far      = 100.0f;
};
