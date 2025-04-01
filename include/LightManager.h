#pragma once

#include <glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include "RenderSystem.h"
#include "Shader.h"

// Forward declarations
class RenderContext;
class Shader;
class RenderTarget;
class Material;

// Light types
enum class LightType
{
    DIRECTIONAL,
    POINT,
    SPOT
};

// Base class for all lights
class Light
{
public:
    Light(const std::string& name, LightType type)
        : name(name), type(type), enabled(true), castShadows(false),
          color(1.0f, 1.0f, 1.0f), intensity(1.0f)
    {
    
    }

    virtual ~Light() = default;

    // Getters and setters
    const std::string& getName() const { return name; }
    LightType getType() const { return type; }

    bool isEnabled() const { return enabled; }
    void setEnabled(bool enabled) { this->enabled = enabled; }

    bool doesCastShadows() const { return castShadows; }
    void setCastShadows(bool castShadows) { this->castShadows = castShadows; }

    glm::vec3 getColor() const { return color; }
    void setColor(const glm::vec3& color) { this->color = color; }

    float getIntensity() const { return intensity; }
    void setIntensity(float intensity) { this->intensity = intensity; }

    // Virtual methods for light-specific operations
    virtual glm::vec3 getDirection() const = 0;
    virtual glm::mat4 calculateLightSpaceMatrix() const = 0;

    // Set shader parameters specific to this light type
    virtual void setShaderParameters(Shader* shader, int lightIndex) const = 0;

protected:
    std::string name;
    LightType type;
    bool enabled;
    bool castShadows;
    glm::vec3 color;
    float intensity;
};

// Directional light (sun-like)
class DirectionalLight : public Light
{
public:
    DirectionalLight(const std::string& name)
        : Light(name, LightType::DIRECTIONAL),
          direction(0.0f, -1.0f, 0.0f),
          shadowOrthoSize(50.0f),
          shadowNearPlane(1.0f),
          shadowFarPlane(100.0f)
    {

    }

    glm::vec3 getDirection() const override { return direction; }
    void setDirection(const glm::vec3& dir) { direction = glm::normalize(dir); }

    // Shadow mapping parameters
    float getShadowOrthoSize() const { return shadowOrthoSize; }
    void setShadowOrthoSize(float size) { shadowOrthoSize = size; }

    float getShadowNearPlane() const { return shadowNearPlane; }
    void setShadowNearPlane(float near) { shadowNearPlane = near; }

    float getShadowFarPlane() const { return shadowFarPlane; }
    void setShadowFarPlane(float far) { shadowFarPlane = far; }

    glm::mat4 calculateLightSpaceMatrix() const override
    {
        // For directional light, the position is calculated from the direction
        glm::vec3 lightPos = -direction * 50.0f; // Position far away
        glm::vec3 lightTarget(0.0f); // Look at center

        // View matrix
        glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));

        // Projection matrix (orthographic for directional light)
        glm::mat4 lightProjection = glm::ortho(
            -shadowOrthoSize, shadowOrthoSize,
            -shadowOrthoSize, shadowOrthoSize,
            shadowNearPlane, shadowFarPlane
        );

        return lightProjection * lightView;
    }

    void setShaderParameters(Shader* shader, int lightIndex) const override
    {
        if (!shader) return;

        std::string prefix = "lights[" + std::to_string(lightIndex) + "].";

        shader->setInt(prefix + "type", static_cast<int>(type));
        shader->setBool(prefix + "enabled", enabled);
        shader->setVec3(prefix + "direction", direction);
        shader->setVec3(prefix + "color", color);
        shader->setFloat(prefix + "intensity", intensity);
        shader->setBool(prefix + "castShadows", castShadows);
    }

private:
    glm::vec3 direction;
    float shadowOrthoSize;
    float shadowNearPlane;
    float shadowFarPlane;
};

// Point light (omnidirectional)
class PointLight : public Light
{
public:
    PointLight(const std::string& name)
        : Light(name, LightType::POINT),
          position(0.0f, 0.0f, 0.0f),
          range(10.0f),
          attenuation(1.0f, 0.09f, 0.032f)
    {
    
    }

    glm::vec3 getPosition() const { return position; }
    void setPosition(const glm::vec3& pos) { position = pos; }

    glm::vec3 getDirection() const override
    {
        // Point lights don't have a single direction
        return glm::vec3(0.0f, -1.0f, 0.0f);
    }

    float getRange() const { return range; }
    void setRange(float r) { range = r; }

    glm::vec3 getAttenuation() const { return attenuation; }
    void setAttenuation(float constant, float linear, float quadratic)
    {
        attenuation = glm::vec3(constant, linear, quadratic);
    }

    glm::mat4 calculateLightSpaceMatrix() const override
    {
        // For point lights, we would need 6 directions for cubemap shadow mapping
        // This is a placeholder for future implementation
        return glm::mat4(1.0f);
    }

    void setShaderParameters(Shader* shader, int lightIndex) const override
    {
        if (!shader) return;

        std::string prefix = "lights[" + std::to_string(lightIndex) + "].";

        shader->setInt(prefix + "type", static_cast<int>(type));
        shader->setBool(prefix + "enabled", enabled);
        shader->setVec3(prefix + "position", position);
        shader->setVec3(prefix + "color", color);
        shader->setFloat(prefix + "intensity", intensity);
        shader->setFloat(prefix + "range", range);
        shader->setVec3(prefix + "attenuation", attenuation);
        shader->setBool(prefix + "castShadows", castShadows);
    }

private:
    glm::vec3 position;
    float range;
    glm::vec3 attenuation; // constant, linear, quadratic
};

// Spot light (cone-shaped)
class SpotLight : public Light
{
public:
    SpotLight(const std::string& name)
        : Light(name, LightType::SPOT),
          position(0.0f, 0.0f, 0.0f),
          direction(0.0f, -1.0f, 0.0f),
          cutoffAngle(17.5f),
          outerCutoffAngle(17.5),
          range(10.0f),
          attenuation(1.0f, 0.09f, 0.032f)
    {
    
    }

    glm::vec3 getPosition() const { return position; }
    void setPosition(const glm::vec3& pos) { position = pos; }

    glm::vec3 getDirection() const override { return direction; }
    void setDirection(const glm::vec3& dir) { direction = glm::normalize(dir); }

    float getCutoffAngle() const { return cutoffAngle; }
    void setCutoffAngle(float angle) { cutoffAngle = angle; }

    float getOuterCutoffAngle() const { return outerCutoffAngle; }
    void setOuterCutoffAngle(float angle) { outerCutoffAngle = angle; }

    float getRange() const { return range; }
    void setRange(float r) { range = r; }

    glm::vec3 getAttenuation() const { return attenuation; }
    void setAttenuation(float constant, float linear, float quadratic)
    {
        attenuation = glm::vec3(constant, linear, quadratic);
    }

    glm::mat4 calculateLightSpaceMatrix() const override
    {
        // View matrix from light's perspective
        glm::mat4 lightView = glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));

        // Projection matrix (perspective for spot light)
        float aspectRatio = 1.0f; // Square shadow map
        float fov = outerCutoffAngle * 2.0f; // FOV is twice the outer cutoff angle
        glm::mat4 lightProjection = glm::perspective(
            glm::radians(fov), aspectRatio, 0.1f, range
        );

        return lightProjection * lightView;
    }

    void setShaderParameters(Shader* shader, int lightIndex) const override
    {
        if (!shader) return;

        std::string prefix = "lights[" + std::to_string(lightIndex) + "].";

        shader->setInt(prefix + "type", static_cast<int>(type));
        shader->setBool(prefix + "enabled", enabled);
        shader->setVec3(prefix + "position", position);
        shader->setVec3(prefix + "direction", direction);
        shader->setVec3(prefix + "color", color);
        shader->setFloat(prefix + "intensity", intensity);
        shader->setFloat(prefix + "cutoff", glm::cos(glm::radians(cutoffAngle)));
        shader->setFloat(prefix + "outerCutoff", glm::cos(glm::radians(outerCutoffAngle)));
        shader->setFloat(prefix + "range", range);
        shader->setVec3(prefix + "attenuation", attenuation);
        shader->setBool(prefix + "castShadows", castShadows);
    }

private:
    glm::vec3 position;
    glm::vec3 direction;
    float cutoffAngle; // Inner cutoff angle in degrees
    float outerCutoffAngle; // Outer cutoff angle in degrees
    float range;
    glm::vec3 attenuation; // constant, linear, quadratic
};

// Global lighting ambient settings
struct AmbientSettings
{
    glm::vec3 color = glm::vec3(0.1f, 0.1f, 0.1f);
    float intensity = 0.1f;
};

// Class to manage all lights in the scene
class LightManager
{
    static LightManager& getInstance()
    {
        static LightManager instance;
        return instance;
    }

    // Initialize the light manager
    void initialize();

    // Add a new light of specified type
    std::shared_ptr<DirectionalLight> createDirectionalLight(const std::string& name);
    std::shared_ptr<PointLight> createPointLight(const std::string& name);
    std::shared_ptr<SpotLight> createSpotLight(const std::string& name);

    // Get a light by name
    std::shared_ptr<Light> getLight(const std::string& name);

    // Remove a light by name
    void removeLight(const std::string& name);

    // Get all lights of a specific type
    std::vector<std::shared_ptr<Light>> getLightsByType(LightType type) const;

    // Get all lights
    const std::vector<std::shared_ptr<Light>>& getAllLights() const;

    // Get/set ambient setting
    AmbientSettings& getAmbientSettings() { return ambientSettings; }
    void setAmbientSettings(const AmbientSettings& settings) { ambientSettings = settings; }

    // Apply all enabled lights to a shader
    void applyLightsToShader(Shader* shader) const;

    // Render shadow maps for all shadow-casting lights
    void renderShadowMaps(RenderContext& context);

    // Get shadow map textures
    unsigned int getShadowMapTexture(const std::string& lightName) const;

    // Get the light space matrix for a particular light
    glm::mat4 getLightSpaceMatrix(const std::string& lightName) const;

    // Clean up resources
    void shutdown();

private:
    LightManager() {}
    ~LightManager() { shutdown(); }

    LightManager(const LightManager&) = delete;
    LightManager& operator=(const LightManager&) = delete;

    std::vector<std::shared_ptr<Light>> lights;
    std::unordered_map<std::string, std::shared_ptr<RenderTarget>> shadowMaps;
    std::unordered_map<std::string, glm::mat4> lightSpaceMatrices;
    AmbientSettings ambientSettings;

    // Shadow map resolution (configurable)
    int shadowMapResolution = 2048;
};