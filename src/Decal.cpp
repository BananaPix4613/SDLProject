#include "Decal.h"
#include "Texture.h"
#include "Material.h"
#include <gtc/matrix_transform.hpp>

Decal::Decal() :
    position(0.0f, 0.0f, 0.0f),
    rotation(0.0f, 0.0f, 0.0f),
    scale(1.0f, 1.0f, 1.0f),
    diffuseTexture(nullptr),
    normalTexture(nullptr),
    roughnessTexture(nullptr),
    color(1.0f, 1.0f, 1.0f, 1.0f),
    blendMode(BLEND_NORMAL),
    material(nullptr),
    projectionDistance(1.0f),
    fadeStart(0.8f),
    fadeEnd(1.0f),
    pixelSnapping(true),
    pixelSize(1)
{

}

Decal::~Decal()
{
    // Textures and materials should be managed by a resource system
    // No need to delete them here
}

void Decal::setPosition(const glm::vec3& position)
{
    this->position = position;
}

void Decal::setRotation(const glm::vec3& rotation)
{
    this->rotation = rotation;
}

void Decal::setScale(const glm::vec3& scale)
{
    this->scale = scale;
}

void Decal::setDiffuseTexture(Texture* texture)
{
    diffuseTexture = texture;
}

void Decal::setNormalTexture(Texture* texture)
{
    normalTexture = texture;
}

void Decal::setRoughnessTexture(Texture* texture)
{
    roughnessTexture = texture;
}

void Decal::setColor(const glm::vec4& color)
{
    this->color = color;
}

void Decal::setBlendMode(BlendMode mode)
{
    blendMode = mode;
}

void Decal::setMaterial(Material* material)
{
    this->material = material;
}

void Decal::setProjectionDistance(float distance)
{
    projectionDistance = std::max(0.01f, distance);
}

void Decal::setFadeDistance(float startDistance, float endDistance)
{
    // Ensure start <= end and both are in [0,1] range
    fadeStart = glm::clamp(startDistance, 0.0f, endDistance);
    fadeEnd = glm::clamp(endDistance, fadeStart, 1.0f);
}

void Decal::setPixelSnapping(bool enable)
{
    pixelSnapping = enable;
}

void Decal::setPixelSize(int virtualPixelSize)
{
    pixelSize = std::max(1, virtualPixelSize);
}

const glm::vec3& Decal::getPosition() const
{
    return position;
}

const glm::vec3& Decal::getRotation() const
{
    return rotation;
}

const glm::vec3& Decal::getScale() const
{
    return scale;
}

glm::mat4 Decal::getTransformMatrix() const
{
    glm::mat4 model = glm::mat4(1.0f);

    // Apply position
    model = glm::translate(model, position);

    // Apply rotation (Euler angles in XYZ order)
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    // Apply scale
    model = glm::scale(model, scale);

    return model;
}

Texture* Decal::getDiffuseTexture() const
{
    return diffuseTexture;
}

Texture* Decal::getNormalTexture() const
{
    return normalTexture;
}

Texture* Decal::getRoughnessTexture() const
{
    return roughnessTexture;
}

const glm::vec4& Decal::getColor() const
{
    return color;
}

Decal::BlendMode Decal::getBlendMode() const
{
    return blendMode;
}

Material* Decal::getMaterial() const
{
    return material;
}

float Decal::getProjectionDistance() const
{
    return projectionDistance;
}

void Decal::getFadeDistance(float& start, float& end) const
{
    start = fadeStart;
    end = fadeEnd;
}

bool Decal::getPixelSnapping() const
{
    return pixelSnapping;
}

int Decal::getPixelSize() const
{
    return pixelSize;
}

void Decal::packDecalData(std::vector<float>& buffer, uint32_t& offset) const
{
    // Pack decal data for GPU consumption
    // Decal data layout (28 floats per decal):
    // [0-15]: Transform matrix (4x4)
    // [16-19]: Color + blend mode
    // [20-23]: Additional properties
    // [24-27]: Texture indices

    // Transform matrix (column-major)
    glm::mat4 transform = getTransformMatrix();
    glm::mat4 invTransform = glm::inverse(transform);

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            buffer[offset++] = invTransform[i][j]; // We use inverse for projecting
        }
    }

    // Color + blend mode
    buffer[offset++] = color.r;
    buffer[offset++] = color.g;
    buffer[offset++] = color.b;
    buffer[offset++] = color.a;

    // Additional properties
    buffer[offset++] = static_cast<float>(blendMode);
    buffer[offset++] = projectionDistance;
    buffer[offset++] = fadeStart;
    buffer[offset++] = fadeEnd;

    // Texture availability flags stored as IDs
    // A real implementation would use texture handles or indices
    buffer[offset++] = diffuseTexture ? 1.0f : 0.0f;
    buffer[offset++] = normalTexture ? 1.0f : 0.0f;
    buffer[offset++] = roughnessTexture ? 1.0f : 0.0f;
    buffer[offset++] = pixelSnapping ? static_cast<float>(pixelSize) : 0.0f;
}