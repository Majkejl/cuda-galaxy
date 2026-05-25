#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>   // glm::lookAt, glm::perspective
#include <algorithm>                      // std::clamp
#include <cmath>                          // std::sin/cos/pow

Camera::Camera(float aspect) : m_aspect(aspect) {}

void Camera::orbit(float dYawRad, float dPitchRad) {
    m_yaw   += dYawRad;
    m_pitch += dPitchRad;
    // Clamp pitch just shy of the poles. At ±90° the eye sits directly above/below the
    // target, so the view direction becomes parallel to the (0,1,0) up vector and lookAt
    // degenerates (the image flips / collapses). Keeping it < 90° avoids that.
    const float limit = glm::radians(89.0f);
    m_pitch = std::clamp(m_pitch, -limit, limit);
}

void Camera::zoom(float scrollSteps) {
    // Multiplicative so each notch feels like the same proportional step and distance can
    // never reach zero or go negative.
    m_distance *= std::pow(0.9f, scrollSteps);
    m_distance = std::clamp(m_distance, 0.2f, 50.0f);
}

void Camera::setAspect(float aspect) { m_aspect = aspect; }

glm::vec3 Camera::eye() const {
    // Spherical -> Cartesian around the target: yaw sweeps the xz-plane, pitch lifts in y.
    return m_target + m_distance * glm::vec3(
        std::cos(m_pitch) * std::sin(m_yaw),
        std::sin(m_pitch),
        std::cos(m_pitch) * std::cos(m_yaw));
}

glm::mat4 Camera::viewProjection() const {
    const glm::mat4 view = glm::lookAt(eye(), m_target, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 proj = glm::perspective(m_fovY, m_aspect, m_near, m_far);
    return proj * view;   // column-major: applied to a vertex as proj * (view * pos)
}
