#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

class IsometricCamera
{
private:
    glm::vec3 position;
    glm::vec3 target;
    float zoom;
    float aspectRatio;

public:
    IsometricCamera(float aspect = 1.0f);
    ~IsometricCamera();

    void setPosition(const glm::vec3& pos);
    void setTarget(const glm::vec3& tgt);
    void setZoom(float zoomLevel);
    void setAspectRatio(float aspect);

    glm::vec3 getPosition() const;
    glm::vec3 getTarget() const;
    float getZoom() const;
    float getAspectRatio() const;
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;

    void rotate(float angle); // Rotate around target point
    void pan(const glm::vec3& direction);
};