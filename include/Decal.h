#pragma once

#include <glm.hpp>
#include <string>
#include <vector>

class Texture;
class Material;

// Forward declaration
class ClusteredRenderSystem;

class Decal
{
public:
    enum BlendMode
    {
        BLEND_NORMAL,
        BLEND_ADDITIVE,
        BLEND_MULTIPLY
    };

    Decal();
    ~Decal();

    // Transform methods
    void setPosition(const glm::vec3& position);
    void setRotation(const glm::vec3& rotation); // Euler angles in degrees
    void setScale(const glm::vec3& scale);

    // Appearance configuration
    void setDiffuseTexture(Texture* texture);
    void setNormalTexture(Texture* texture);
    void setRoughnessTexture(Texture* texture);
    void setColor(const glm::vec4& color); // RGB + alpha
    void setBlendMode(BlendMode mode);
    void setMaterial(Material* material);

    // Projection settings
    void setProjectionDistance(float distance);
    void setFadeDistance(float startDistance, float endDistance);
    void setPixelSnapping(bool enable);
    void setPixelSize(int virtualPixelSize);

    // Getters for internal use by render system
    const glm::vec3& getPosition() const;
    const glm::vec3& getRotation() const;
    const glm::vec3& getScale() const;
    glm::mat4 getTransformMatrix() const;

    Texture* getDiffuseTexture() const;
    Texture* getNormalTexture() const;
    Texture* getRoughnessTexture() const;
    const glm::vec4& getColor() const;
    BlendMode getBlendMode() const;
    Material* getMaterial() const;

    float getProjectionDistance() const;
    void getFadeDistance(float& start, float& end) const;
    bool getPixelSnapping() const;
    int getPixelSize() const;

    // Prepare decal data for shader
    void packDecalData(std::vector<float>& buffer, uint32_t& offset) const;

private:
    // Transform
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    // Appearance
    Texture* diffuseTexture;
    Texture* normalTexture;
    Texture* roughnessTexture;
    glm::vec4 color;
    BlendMode blendMode;
    Material* material;

    // Projection settings
    float projectionDistance;
    float fadeStart;
    float fadeEnd;
    bool pixelSnapping;
    int pixelSize;
};