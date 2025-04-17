#include "Light.h"
#include "Texture.h"
#include <cmath>
#include <iostream>
#include <limits>

Light::Light(Type type) :
    type(type),
    position(0.0f, 0.0f, 0.0f),
    color(1.0f, 1.0f, 1.0f),
    intensity(1.0f),
    range(10.0f),
    shadows(false),
    direction(0.0f, -1.0f, 0.0f),
    spotInnerAngle(glm::radians(30.0f)),
    spotOuterAngle(glm::radians(45.0f)),
    areaSize(1.0f, 1.0f),
    volumetricDensity(0.1f),
    volumetricScattering(0.5f),
    iesProfile(nullptr),
    flickerAmount(0.0f),
    flickerSpeed(0.0f),
    flickerTime(0.0f),
    currentFlickerValue(1.0f)
{
    // Set appropriate defaults based on light type
    switch (type)
    {
        case DIRECTIONAL:
            range = std::numeric_limits<float>::max();
            break;
        case POINT:
            range = 10.0f;
            break;
        case SPOT:
            range = 15.0f;
            break;
        case AREA:
            range = 5.0f;
            break;
        case VOLUMETRIC:
            range = 8.0f;
            break;
    }
}

Light::~Light()
{
    // Clean up resources
    if (iesProfile)
    {
        // Note: In a real implementation, we'd need a proper resource management system
        // For now, we'll assume texture resources are managed elsewhere
    }
}

void Light::setColor(const glm::vec3& color)
{
    this->color = color;
}

void Light::setIntensity(float lumens)
{
    this->intensity = std::max(0.0f, lumens);
}

void Light::setRange(float range)
{
    this->range = std::max(0.1f, range);
}

void Light::setCastShadows(bool castShadows)
{
    this->shadows = castShadows;
}

void Light::setDirection(const glm::vec3& direction)
{
    if (type == DIRECTIONAL || type == SPOT)
    {
        this->direction = glm::normalize(direction);
    }
}

void Light::setSpotAngles(float innerAngle, float outerAngle)
{
    if (type == SPOT)
    {
        // Convert from degrees to radians and ensure inner < outer
        spotInnerAngle = glm::radians(std::min(innerAngle, outerAngle));
        spotOuterAngle = glm::radians(std::max(innerAngle, outerAngle));
    }
}

void Light::setAreaSize(const glm::vec2& size)
{
    if (type == AREA)
    {
        areaSize = glm::max(size, glm::vec2(0.01f));
    }
}

void Light::setVolumetricParameters(float density, float scattering)
{
    if (type == VOLUMETRIC)
    {
        volumetricDensity = glm::clamp(density, 0.0f, 1.0f);
        volumetricScattering = glm::clamp(scattering, 0.0f, 1.0f);
    }
}

void Light::setIESProfile(const std::string& iesFilePath)
{
    this->iesFilePath = iesFilePath;
    loadIESProfile(iesFilePath);
}

void Light::setFlicker(float amount, float speed)
{
    flickerAmount = glm::clamp(amount, 0.0f, 1.0f);
    flickerSpeed = speed;
}

void Light::setPosition(const glm::vec3& position)
{
    this->position = position;
}

glm::vec3 Light::getPosition() const
{
    return position;
}

Light::Type Light::getType() const
{
    return type;
}

const glm::vec3& Light::getColor() const
{
    return color;
}

float Light::getIntensity() const
{
    return intensity;
}

float Light::getRange() const
{
    return range;
}

bool Light::getCastShadows() const
{
    return shadows;
}

const glm::vec3& Light::getDirection() const
{
    return direction;
}

void Light::getSpotAngles(float& innerAngle, float& outerAngle) const
{
    innerAngle = spotInnerAngle;
    outerAngle = spotOuterAngle;
}

const glm::vec2& Light::getAreaSize() const
{
    return areaSize;
}

void Light::getVolumetricParameters(float& density, float& scattering) const
{
    density = volumetricDensity;
    scattering = volumetricScattering;
}

Texture* Light::getIESProfile() const
{
    return iesProfile;
}

void Light::getFlickerParameters(float& amount, float& speed) const
{
    amount = flickerAmount;
    speed = flickerSpeed;
}

float Light::getCurrentFlickerModifier() const
{
    return currentFlickerValue;
}

void Light::update(float deltaTime)
{
    // Update flicker effect
    if (flickerAmount > 0.0f)
    {
        updateFlicker(deltaTime);
    }
}

void Light::packLightData(std::vector<float>& buffer, uint32_t& offset) const
{
    // Pack light data for GPU consumption
    // Light data layout (16 floats per light):
    // [0-3]: position + type
    // [4-7]: color + intensity * flicker
    // [8-11]: direction + range
    // [12-15]: additional params based on light type

    // Position + type (in w component for easy shader access)
    buffer[offset++] = position.x;
    buffer[offset++] = position.y;
    buffer[offset++] = position.z;
    buffer[offset++] = static_cast<float>(type);

    // Color + intensity (with flicker applied)
    buffer[offset++] = color.r;
    buffer[offset++] = color.g;
    buffer[offset++] = color.b;
    buffer[offset++] = intensity * currentFlickerValue;

    // Direction + range
    buffer[offset++] = direction.x;
    buffer[offset++] = direction.y;
    buffer[offset++] = direction.z;
    buffer[offset++] = range;

    // Additional type-specific parameters
    switch (type)
    {
        case DIRECTIONAL:
            // For directional lights, we don't need additional params
            buffer[offset++] = 0.0f; // padding
            buffer[offset++] = 0.0f; // padding
            buffer[offset++] = 0.0f; // padding
            buffer[offset++] = shadows ? 1.0f : 0.0f; // shadow flag
            break;

        case SPOT:
            // Spot light cone angles
            buffer[offset++] = std::cos(spotInnerAngle); // cos of inner angle for faster shader calculations
            buffer[offset++] = std::cos(spotOuterAngle); // cos of outer angle
            buffer[offset++] = (iesProfile) ? 1.0f : 0.0f; // ies profile flag
            buffer[offset++] = shadows ? 1.0f : 0.0f; // shadow flag
            break;

        case AREA:
            // Area light size
            buffer[offset++] = areaSize.x;
            buffer[offset++] = areaSize.y;
            buffer[offset++] = 0.0f; // padding
            buffer[offset++] = shadows ? 1.0f : 0.0f; // shadow flag
            break;

        case VOLUMETRIC:
            // Volumetric parameters
            buffer[offset++] = volumetricDensity;
            buffer[offset++] = volumetricScattering;
            buffer[offset++] = 0.0f; // padding
            buffer[offset++] = shadows ? 1.0f : 0.0f; // shadow flag
            break;

        case POINT:
        default:
            // Point light doesn't need additional params
            buffer[offset++] = (iesProfile) ? 1.0f : 0.0f; // ies profile flag
            buffer[offset++] = 0.0f; // padding
            buffer[offset++] = 0.0f; // padding
            buffer[offset++] = shadows ? 1.0f : 0.0f; // shadow flag
            break;
    }
}

void Light::loadIESProfile(const std::string& path)
{
    if (path.empty())
    {
        iesProfile = nullptr;
        return;
    }

    // In a real implementation, this would:
    // 1. Parse the IES file format
    // 2. Generate a texture represeting the light distribution
    // 3. Store the texture for use in shaders

    // For this implementation, we'll just print a message
    std::cout << "Loading IES profile from: " << path << " (placeholder)" << std::endl;

    // TODO: Implement actual IES profile loading
    // iesProfile = TextureManager::loadIESProfile(path);
}

void Light::updateFlicker(float deltaTime)
{
    // Update flicker time
    flickerTime += deltaTime * flickerSpeed;

    // Calculate flicker value using Perlin noise or similar
    // For this implementation, we'll use a simple sin function with some variation
    float noise1 = std::sin(flickerTime * 1.0f);
    float noise2 = std::sin(flickerTime * 2.7f) * 0.5f;
    float noise3 = std::sin(flickerTime * 4.3f) * 0.25f;

    float noiseSum = (noise1 + noise2 + noise3) / 1.75f; // Normalize to -1 to 1 range

    // Convert the -1 to 1 range to 0 to 1, then apply amount
    float flickerNoise = (noiseSum * 0.5f + 0.5f) * flickerAmount;

    // Apply to current flicker value (1.0 is base, can go down to 1.0 - flickerAmount)
    currentFlickerValue = 1.0f - flickerNoise;
}

float Light::calculatePixelArtAttenuation(float distance) const
{
    // For pixel art, we want fewer steps in lighting falloff to create a stylized look

    // Normalize distance by range
    float normalizedDist = glm::clamp(distance / range, 0.0f, 1.0f);

    // Standard quadratic attenuation as reference
    float standardAtten = 1.0f / (1.0f + normalizedDist * normalizedDist);

    // Quantize into distinct steps for pixel art aesthetic (8 steps)
    const int steps = 8;
    float quantizedAtten = std::floor(standardAtten * steps) / steps;

    return quantizedAtten;
}