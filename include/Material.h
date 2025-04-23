#pragma once

#include <string>
#include <unordered_map>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <Shader.h>
#include <Texture.h>

// Forward declarations
class Shader;
class Texture;
class PaletteManager;

// Material system for PBR rendering adapted for pixel art aesthetics
class Material
{
public:
    Material(Shader* shader);
    ~Material();

    // Core functionality
    void bind();

    // PBR core parameters
    void setBaseColor(const glm::vec3& color);
    void setMetallic(float metallic);
    void setRoughness(float roughness);
    void setEmissive(const glm::vec3& emission, float intensity);
    void setNormalScale(float scale);
    void setOcclusion(float occlusion);

    // Texture maps
    void setAlbedoMap(Texture* texture);
    void setNormalMap(Texture* texture);
    void setMetallicRoughnessMap(Texture* texture);
    void setEmissiveMap(Texture* texture);
    void setOcclusionMap(Texture* texture);

    // Advanced parameters
    void setSubsurfaceParameters(float scattering, const glm::vec3& color);
    void setAnisotropy(float anisotropy, float rotation);
    void setClearcoat(float clearcoat, float roughness);

    // Generic parameter setting
    void setParameter(const std::string& name, float value);
    void setParameter(const std::string& name, const glm::vec3& value);
    void setParameter(const std::string& name, const glm::mat4& value);

    // Pixel art specific settings
    void setPaletteConstraint(bool constrain);
    void setPaletteManager(PaletteManager* paletteManager);
    void setPixelSnapAmount(float amount);
    void setDitherAmount(float amount);

    // Getters
    Shader* getShader() const;
    const glm::vec3& getBaseColor() const;
    float getMetallic() const;
    float getRoughness() const;
    const glm::vec3& getEmissive() const;
    float getEmissiveIntensity() const;
    float getNormalScale() const;

    // Texture access
    Texture* getAlbedoMap() const;
    Texture* getNormalMap() const;
    Texture* getMetallicRoughnessMap() const;
    Texture* getEmissiveMap() const;
    Texture* getOcclusionMap() const;

    // Clone this material with optional new shader
    Material* clone(Shader* newShader = nullptr) const;

private:
    Shader* shader;
    PaletteManager* paletteManager;

    // Core PBR parameters
    glm::vec3 baseColor;
    float metallic;
    float roughness;
    glm::vec3 emissiveColor;
    float emissiveIntensity;
    float normalScale;
    float occlusionStrength;

    // Advanced material parameters
    float subsurfaceScattering;
    glm::vec3 subsurfaceColor;
    float anisotropyAmount;
    float anisotropyRotation;
    float clearcoatAmount;
    float clearcoatRoughness;

    // Pixel art specific parameters
    bool usePaletteConstraint;
    float pixelSnapAmount;
    float ditherAmount;

    // Texture maps
    Texture* albedoMap;
    Texture* normalMap;
    Texture* metallicRoughnessMap;
    Texture* emissiveMap;
    Texture* occlusionMap;

    // Generic parameter storage for custom properties
    std::unordered_map<std::string, float> floatParameters;
    std::unordered_map<std::string, glm::vec3> vec3Parameters;
    std::unordered_map<std::string, glm::mat4> mat4Parameters;

    // Helper methods
    void bindTextures();
    void bindParameters();
    void bindPixelArtParameters();
};