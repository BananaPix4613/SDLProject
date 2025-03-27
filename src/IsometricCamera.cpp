//#include "IsometricCamera.h"
//
//IsometricCamera::IsometricCamera(float aspect)
//    : position(30.0f, 30.0f, 30.0f),
//      target(0.0f, 0.0f, 0.0f),
//      zoom(1.0f),
//      aspectRatio(aspect)
//{
//    
//}
//
//IsometricCamera::~IsometricCamera()
//{
//    
//}
//
//void IsometricCamera::setPosition(const glm::vec3 &pos)
//{
//    position = pos;
//}
//
//void IsometricCamera::setTarget(const glm::vec3 &tgt)
//{
//    target = tgt;
//}
//
//void IsometricCamera::setZoom(float zoomLevel)
//{
//    zoom = glm::clamp(zoomLevel, 0.1f, 10.0f);
//}
//
//void IsometricCamera::setAspectRatio(float aspect)
//{
//    aspectRatio = aspect;
//}
//
//glm::vec3 IsometricCamera::getPosition() const
//{
//    return position;
//}
//
//glm::vec3 IsometricCamera::getTarget() const
//{
//    return target;
//}
//
//float IsometricCamera::getZoom() const
//{
//    return zoom;
//}
//
//float IsometricCamera::getAspectRatio() const
//{
//    return aspectRatio;
//}
//
//glm::mat4 IsometricCamera::getViewMatrix() const
//{
//    return glm::lookAt(position, target, glm::vec3(0.0f, 1.0f, 0.0f));
//}
//
//glm::mat4 IsometricCamera::getProjectionMatrix() const
//{
//    float orthoSize = 20.0f / zoom;
//    return glm::ortho(
//        -orthoSize * aspectRatio, orthoSize * aspectRatio,
//        -orthoSize, orthoSize,
//        -100.0f, 1000.0f
//    );
//}
//
//void IsometricCamera::rotate(float angle)
//{
//    // Create a rotation matrix around the Y axis
//    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
//
//    // Get the direction vector from target to position
//    glm::vec3 direction = position - target;
//
//    // Apply rotation to the direction vector
//    glm::vec4 rotatedDirection = rotationMatrix * glm::vec4(direction, 0.0f);
//
//    // Calculate new position
//    position = target + glm::vec3(rotatedDirection);
//}
//
//void IsometricCamera::pan(const glm::vec3 &direction)
//{
//    target += direction;
//    position += direction;
//}
