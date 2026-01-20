#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace gps {

    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;

        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUp));
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }

    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(
            cameraPosition,
            cameraPosition + cameraFrontDirection,
            cameraUpDirection
        );
    }

    void Camera::move(MOVE_DIRECTION direction, float speed) {
        if (direction == MOVE_FORWARD)
            cameraPosition += cameraFrontDirection * speed;

        if (direction == MOVE_BACKWARD)
            cameraPosition -= cameraFrontDirection * speed;

        if (direction == MOVE_RIGHT)
            cameraPosition += cameraRightDirection * speed;

        if (direction == MOVE_LEFT)
            cameraPosition -= cameraRightDirection * speed;
    }

    void Camera::rotate(float pitch, float yaw) {
        static float currentPitch = 0.0f;
        static float currentYaw = -90.0f; // look forward by default

        currentPitch += pitch;
        currentYaw += yaw;

        // clamp pitch to avoid flipping
        if (currentPitch > 89.0f)  currentPitch = 89.0f;
        if (currentPitch < -89.0f) currentPitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(currentYaw)) * cos(glm::radians(currentPitch));
        front.y = sin(glm::radians(currentPitch));
        front.z = sin(glm::radians(currentYaw)) * cos(glm::radians(currentPitch));

        cameraFrontDirection = glm::normalize(front);

        cameraRightDirection = glm::normalize(
            glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f))
        );
        cameraUpDirection = glm::normalize(
            glm::cross(cameraRightDirection, cameraFrontDirection)
        );
    }

}
