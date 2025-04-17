#include "Material.h"
#include "PaletteManager.h"

Material::Material(Shader* shader) :
    shader(shader), paletteManager(nullptr),
    baseColor(1.0f, 1.0f, 1.0f), metallic(0.0f), roughness(0.5f),
    emissiveColor(0.0f), emissiveIntensity(0.0f), normalScale(1.0f), occlusionStrength(1.0f),
    subsurfaceScattering(0.0f), subsurfaceColor(1.0f, 0.3f, 0.2f),
    anisotropyAmount(0.0f), anisotropyRotation(0.0f),
    clearcoatAmount(0.0f), clearcoatRoughness(0.5f),
    usePaletteConstraint(false), pixelSnapAmount(1.0f), ditherAmount(0.0f),
    albedoMap(nullptr), normalMap(nullptr), metallicRoughnessMap(nullptr),
    emissiveMap(nullptr), occlusionMap(nullptr)
{
}

Material::~Material()
{
    // The material doesn't own the shader or textures, no need to delete them
}

void Material::bind()
{
    if (!shader)
    {
        return;
    }

    shader->use();

    // Set core PBR parameters
    shader->setVec3("material.baseColor", baseColor);
    shader->setFloat("material.metallic", metallic);
    shader->setFloat("material.roughness", roughness);
    shader->setVec3("material.emissive", emissiveColor);
    shader->setFloat("material.emissiveIntensity", emissiveIntensity);
    shader->setFloat("material.normalScale", normalScale);
    shader->setFloat("material.occlusionStrength", occlusionStrength);

    // Set advanced parameters
    shader->setFloat("material.subsurfaceScattering", subsurfaceScattering);
    shader->setVec3("material.subsurfaceColor", subsurfaceColor);
    shader->setFloat("material.anisotropy", anisotropyAmount);
    shader->setFloat("material.anisotropyRotation", anisotropyRotation);
    shader->setFloat("material.clearcoat", clearcoatAmount);
    shader->setFloat("material.clearcoatRoughness", clearcoatRoughness);

    // Set pixel art parameters
    bindPixelArtParameters();

    // Bind textures
    bindTextures();

    // Bind generic parameters
    bindParameters();
}

void Material::bindTextures()
{
    // Set texture existence flags
    shader->setBool("material.hasAlbedoMap", albedoMap != nullptr);
    shader->setBool("material.hasNormalMap", normalMap != nullptr);
    shader->setBool("material.hasMetallicRoughnessMap", metallicRoughnessMap != nullptr);
    shader->setBool("material.hasEmissiveMap", emissiveMap != nullptr);
    shader->setBool("material.hasOcclusionMap", occlusionMap != nullptr);

    // Bind textures to appropriate slots
    if (albedoMap)
    {
        albedoMap->bind(0);
        shader->setInt("material.albedoMap", 0);
    }

    if (normalMap)
    {
        normalMap->bind(1);
        shader->setInt("material.normalMap", 1);
    }

    if (metallicRoughnessMap)
    {
        metallicRoughnessMap->bind(2);
        shader->setInt("material.metallicRoughnessMap", 2);
    }

    if (emissiveMap)
    {
        emissiveMap->bind(3);
        shader->setInt("material.emissiveMap", 3);
    }

    if (occlusionMap)
    {
        occlusionMap->bind(4);
        shader->setInt("material.occlusionMap", 4);
    }
}

void Material::bindParameters()
{
    // Bind float parameters
    for (const auto& param : floatParameters)
    {
        shader->setFloat(param.first, param.second);
    }

    // Bind vec3 parameters
    for (const auto& param : vec3Parameters)
    {
        shader->setVec3(param.first, param.second);
    }

    // Bind mat4 parameters
    for (const auto& param : mat4Parameters)
    {
        shader->setMat4(param.first, param.second);
    }
}

void Material::bindPixelArtParameters()
{
    shader->setBool("material.usePaletteConstraint", usePaletteConstraint);
    shader->setFloat("material.pixelSnapAmount", pixelSnapAmount);
    shader->setFloat("material.ditherAmount", ditherAmount);

    // If using palette constraint and we have a palette manager, bind its resources
    if (usePaletteConstraint && paletteManager)
    {
        paletteManager->bindPaletteResources(shader);
    }
}

// Core PBR Parameters

void Material::setBaseColor(const glm::vec3& color)
{
    baseColor = color;
}

void Material::setMetallic(float value)
{
    metallic = glm::clamp(value, 0.0f, 1.0f);
}

void Material::setRoughness(float value)
{
    roughness = glm::clamp(value, 0.0f, 1.0f);
}

void Material::setEmissive(const glm::vec3& emission, float intensity)
{
    emissiveColor = emission;
    emissiveIntensity = intensity;
}

void Material::setNormalScale(float scale)
{
    normalScale = scale;
}

void Material::setOcclusion(float occlusion)
{
    occlusionStrength = glm::clamp(occlusion, 0.0f, 1.0f);
}

// Texture Maps

void Material::setAlbedoMap(Texture* texture)
{
    albedoMap = texture;
}

void Material::setNormalMap(Texture* texture)
{
    normalMap = texture;
}

void Material::setMetallicRoughnessMap(Texture* texture)
{
    metallicRoughnessMap = texture;
}

void Material::setEmissiveMap(Texture* texture)
{
    emissiveMap = texture;
}

void Material::setOcclusionMap(Texture* texture)
{
    occlusionMap = texture;
}

// Advanced Parameters

void Material::setSubsurfaceParameters(float scattering, const glm::vec3& color)
{
    subsurfaceScattering = glm::clamp(scattering, 0.0f, 1.0f);
    subsurfaceColor = color;
}

void Material::setAnisotropy(float anisotropy, float rotation)
{
    anisotropyAmount = glm::clamp(anisotropy, 0.0f, 1.0f);
    anisotropyRotation = rotation;
}

void Material::setClearcoat(float clearcoat, float roughness)
{
    clearcoatAmount = glm::clamp(clearcoat, 0.0f, 1.0f);
    clearcoatRoughness = glm::clamp(roughness, 0.0f, 1.0f);
}

// Generic Parameter Setting

void Material::setParameter(const std::string& name, float value)
{
    floatParameters[name] = value;
}

void Material::setParameter(const std::string& name, const glm::vec3& value)
{
    vec3Parameters[name] = value;
}

void Material::setParameter(const std::string& name, const glm::mat4& value)
{
    mat4Parameters[name] = value;
}

// Pixel Art Specific Settings

void Material::setPaletteConstraint(bool constrain)
{
    usePaletteConstraint = constrain;
}

void Material::setPaletteManager(PaletteManager* manager)
{
    paletteManager = manager;
}

void Material::setPixelSnapAmount(float amount)
{
    pixelSnapAmount = glm::clamp(amount, 0.0f, 1.0f);
}

void Material::setDitherAmount(float amount)
{
    ditherAmount = glm::clamp(amount, 0.0f, 1.0f);
}

// Getters

Shader* Material::getShader() const
{
    return shader;
}

const glm::vec3& Material::getBaseColor() const
{
    return baseColor;
}

float Material::getMetallic() const
{
    return metallic;
}

float Material::getRoughness() const
{
    return roughness;
}

const glm::vec3& Material::getEmissive() const
{
    return emissiveColor;
}

float Material::getEmissiveIntensity() const
{
    return emissiveIntensity;
}

float Material::getNormalScale() const
{
    return normalScale;
}

Texture* Material::getAlbedoMap() const
{
    return albedoMap;
}

Texture* Material::getNormalMap() const
{
    return normalMap;
}

Texture* Material::getMetallicRoughnessMap() const
{
    return metallicRoughnessMap;
}

Texture* Material::getEmissiveMap() const
{
    return emissiveMap;
}

Texture* Material::getOcclusionMap() const
{
    return occlusionMap;
}

Material* Material::clone(Shader* newShader) const
{
    Material* clonedMaterial = new Material(newShader ? newShader : this->shader);

    // Copy all properties
    clonedMaterial->baseColor = this->baseColor;
    clonedMaterial->metallic = this->metallic;
    clonedMaterial->roughness = this->roughness;
    clonedMaterial->emissiveColor = this->emissiveColor;
    clonedMaterial->emissiveIntensity = this->emissiveIntensity;
    clonedMaterial->normalScale = this->normalScale;
    clonedMaterial->occlusionStrength = this->occlusionStrength;

    clonedMaterial->subsurfaceScattering = this->subsurfaceScattering;
    clonedMaterial->subsurfaceColor = this->subsurfaceColor;
    clonedMaterial->anisotropyAmount = this->anisotropyAmount;
    clonedMaterial->anisotropyRotation = this->anisotropyRotation;
    clonedMaterial->clearcoatAmount = this->clearcoatAmount;
    clonedMaterial->clearcoatRoughness = this->clearcoatRoughness;

    clonedMaterial->usePaletteConstraint = this->usePaletteConstraint;
    clonedMaterial->pixelSnapAmount = this->pixelSnapAmount;
    clonedMaterial->ditherAmount = this->ditherAmount;
    clonedMaterial->paletteManager = this->paletteManager;

    // Copy texture references
    clonedMaterial->albedoMap = this->albedoMap;
    clonedMaterial->normalMap = this->normalMap;
    clonedMaterial->metallicRoughnessMap = this->metallicRoughnessMap;
    clonedMaterial->emissiveMap = this->emissiveMap;
    clonedMaterial->occlusionMap = this->occlusionMap;

    // Copy generic parameters
    clonedMaterial->floatParameters = this->floatParameters;
    clonedMaterial->vec3Parameters = this->vec3Parameters;
    clonedMaterial->mat4Parameters = this->mat4Parameters;

    return clonedMaterial;
}