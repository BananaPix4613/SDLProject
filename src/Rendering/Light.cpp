// -------------------------------------------------------------------------
// Light.cpp
// -------------------------------------------------------------------------
#include "Rendering/Light.h"
#include "Core/Logger.h"

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"

namespace PixelCraft::Rendering {

Light::Light(LightType type)
    : m_type(type), m_position(0.0f, 0.0f, 0.0f), m_direction(0.0f, -1.0f, 0.0f), m_color(1.0f, 1.0f, 1.0f),
      m_intensity(1.0f), m_range(10.0f), m_castShadows(false), m_constantAttenuation(1.0f), m_linearAttenuation(0.09f),
      m_quadraticAttenuation(0.032f), m_spotInnerAngle(glm::radians(30.0f)), m_spotOuterAngle(glm::radians(45.0f)),
      m_areaSize(1.0f, 1.0f), m_shadowBias(0.005f), m_shadowResolution(1024) {
    // Normalize the direction vector
    if (glm::length(m_direction) > 0.0f) {
        m_direction = glm::normalize(m_direction);
    }
}

Light::~Light() {
    // No special cleanup needed
}

void Light::setPosition(const glm::vec3 &position) { m_position = position; }

void Light::setDirection(const glm::vec3 &direction) { m_direction = glm::normalize(direction); }

void Light::setColor(const glm::vec3 &color) { m_color = color; }

void Light::setIntensity(float intensity) { m_intensity = glm::max(0.0f, intensity); }

void Light::setRange(float range) { m_range = glm::max(0.1f, range); }

void Light::setCastShadows(bool castShadows) { m_castShadows = castShadows; }

void Light::setAttenuation(float constant, float linear, float quadratic) {
    m_constantAttenuation = glm::max(0.0f, constant);
    m_linearAttenuation = glm::max(0.0f, linear);
    m_quadraticAttenuation = glm::max(0.0f, quadratic);
}

void Light::setSpotAngle(float innerAngle, float outerAngle) {
    m_spotInnerAngle = glm::clamp(innerAngle, 0.0f, glm::radians(175.0f));
    m_spotOuterAngle = glm::clamp(outerAngle, m_spotInnerAngle, glm::radians(175.0f));
}

void Light::setInnerConeAngle(float angle) {
    m_spotInnerAngle = glm::clamp(angle, 0.0f, glm::radians(175.0f));
    // Ensure outer angle is at least as large as inner angle
    if (m_spotOuterAngle < m_spotInnerAngle) {
        m_spotOuterAngle = m_spotInnerAngle;
    }
}

void Light::setOuterConeAngle(float angle) {
    m_spotOuterAngle = glm::clamp(angle, m_spotInnerAngle, glm::radians(175.0f));
}

void Light::setAreaSize(const glm::vec2 &size) { m_areaSize = glm::max(size, glm::vec2(0.01f)); }

LightType Light::getType() const { return m_type; }

const glm::vec3 &Light::getPosition() const { return m_position; }

const glm::vec3 &Light::getDirection() const { return m_direction; }

const glm::vec3 &Light::getColor() const { return m_color; }

float Light::getIntensity() const { return m_intensity; }

float Light::getRange() const { return m_range; }

bool Light::getCastShadows() const { return m_castShadows; }

glm::vec3 Light::getAttenuation() const {
    return glm::vec3(m_constantAttenuation, m_linearAttenuation, m_quadraticAttenuation);
}

float Light::getInnerConeAngle() const { return m_spotInnerAngle; }

float Light::getOuterConeAngle() const { return m_spotOuterAngle; }

const glm::vec2 &Light::getAreaSize() const { return m_areaSize; }

void Light::setShadowBias(float bias) { m_shadowBias = glm::clamp(bias, 0.0f, 0.1f); }

void Light::setShadowResolution(int resolution) {
    // Ensure resolution is a power of two
    int powerOfTwo = 1;
    while (powerOfTwo < resolution && powerOfTwo < 8192) {
        powerOfTwo *= 2;
    }
    m_shadowResolution = powerOfTwo;
}

glm::mat4 Light::getViewMatrix() const {
    switch (m_type) {
    case LightType::Directional: {
        // For directional lights, create a view matrix looking in the light direction
        const glm::vec3 up =
            glm::abs(m_direction.y) > 0.999f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
        return glm::lookAt(glm::vec3(0.0f), m_direction, up);
    }
    case LightType::Point:
    case LightType::Spot: {
        // For point and spot lights, create a view matrix from the light position
        const glm::vec3 target = m_position + m_direction;
        const glm::vec3 up =
            glm::abs(m_direction.y) > 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
        return glm::lookAt(m_position, target, up);
    }
    case LightType::Area: {
        // For area lights, create a view matrix from the light position
        const glm::vec3 target = m_position + m_direction;
        const glm::vec3 up =
            glm::abs(m_direction.y) > 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
        return glm::lookAt(m_position, target, up);
    }
    default:
        return glm::mat4(1.0f);
    }
}

glm::mat4 Light::getProjectionMatrix() const {
    switch (m_type) {
    case LightType::Directional:
        // Orthographic projection for directional lights
        return glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 100.0f);

    case LightType::Point:
        // For point lights, we would typically use cubemap projections
        // This is a placeholder - actual implementation would handle all six faces
        return glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, m_range);

    case LightType::Spot: {
        // For spot lights, use a perspective projection
        float aspectRatio = 1.0f;
        float fovY = 2.0f * m_spotOuterAngle;
        return glm::perspective(fovY, aspectRatio, 0.1f, m_range);
    }

    case LightType::Area:
        // For area lights, use an orthographic projection
        return glm::ortho(-m_areaSize.x * 0.5f, m_areaSize.x * 0.5f, -m_areaSize.y * 0.5f, m_areaSize.y * 0.5f, 0.1f,
                          50.0f);

    default:
        return glm::mat4(1.0f);
    }
}

LightData Light::packLightData() const {
    LightData data;

    // Pack position and light type
    data.position = glm::vec4(m_position, static_cast<float>(m_type));

    // Pack direction and inner cone angle (for spot lights)
    data.direction = glm::vec4(m_direction, m_spotInnerAngle);

    // Pack color and intensity
    data.color = glm::vec4(m_color, m_intensity);

    // Pack attenuation and outer cone angle (for spot lights)
    data.attenuation = glm::vec4(m_constantAttenuation, m_linearAttenuation, m_quadraticAttenuation, m_spotOuterAngle);

    // Pack metadata (shadowIndex will be set by LightManager)
    data.metadata = glm::vec4(0.0f,                        // shadowIndex (placeholder - set by LightManager)
                              m_castShadows ? 1.0f : 0.0f, // castShadows
                              0.0f,                        // unused
                              0.0f                         // unused
    );

    return data;
}

} // namespace PixelCraft::Rendering
