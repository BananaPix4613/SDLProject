#pragma once

#include <glm.hpp>
#include <string>
#include <memory>
#include <vector>

class Texture;
class Shader;

// Forward declaration
class ClusteredRenderSystem;

class Light
{
public:
    enum Type
    {
        DIRECTIONAL,
        POINT,
        SPOT,
        AREA,
        VOLUMETRIC
    };

    Light(Type type = POINT);
    ~Light();

    // Core attributes
    void setColor(const glm::vec3& color);
    void setIntensity(float lumens);
    void setRange(float range);
    void setCastShadows(bool castShadows);

    // Type-specific setters
    void setDirection(const glm::vec3& direction);      // Dir/spot lights
    void setSpotAngles(float innerAngle, float outerAngle); // Spot lights
    void setAreaSize(const glm::vec2& size);            // Area lights
    void setVolumetricParameters(float density, float scattering); // Volumetric

    // Advanced features
    void setIESProfile(const std::string& iesFilePath); // Custom distribution
    void setFlicker(float amount, float speed);         // Dynamic flicker

    // Transform
    void setPosition(const glm::vec3& position);
    glm::vec3 getPosition() const;

    // Getters for internal use by render system
    Type getType() const;
    const glm::vec3& getColor() const;
    float getIntensity() const;
    float getRange() const;
    bool getCastShadows() const;
    const glm::vec3& getDirection() const;
    void getSpotAngles(float& innerAngle, float& outerAngle) const;
    const glm::vec2& getAreaSize() const;
    void getVolumetricParameters(float& density, float& scattering) const;
    Texture* getIESProfile() const;
    void getFlickerParameters(float& amount, float& speed) const;
    float getCurrentFlickerModifier() const;

    // Update for animations/effects
    void update(float deltaTime);

    // Prepare light data for shader
    void packLightData(std::vector<float>& buffer, uint32_t& offset) const;

private:
    Type type;
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float range;
    bool shadows;

    // Type-specific data
    glm::vec3 direction;
    float spotInnerAngle;
    float spotOuterAngle;
    glm::vec2 areaSize;
    float volumetricDensity;
    float volumetricScattering;

    // Advanced features
    Texture* iesProfile;
    std::string iesFilePath;
    float flickerAmount;
    float flickerSpeed;
    float flickerTime;
    float currentFlickerValue;

    // Helper methods
    void loadIESProfile(const std::string& path);
    void updateFlicker(float deltaTime);
    float calculatePixelArtAttenuation(float distance) const;
};