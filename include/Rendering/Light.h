// -------------------------------------------------------------------------
// Light.h
// -------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>

namespace PixelCraft::Rendering
{
    /**
     * @brief Types of light sources supported by the engine
     */
    enum class LightType
    {
        Directional,  ///< Directional light (sun, moon)
        Point,        ///< Point light (omnidirectional)
        Spot,         ///< Spot light (cone)
        Area          ///< Area light (rectangular)
    };

    /**
     * @brief Data structure for light properties optimized for GPU use
     *
     * This structure matches the shader layout exactly and is designed to be 
     * directly uploaded to a GPU buffer for efficient use in shaders.
     */
    struct LightData
    {
        glm::vec4 position;     ///< xyz = World position, w = light type (0=directional, 1=point, 2=spot)
        glm::vec4 direction;    ///< xyz = Forward direction, w = inner cone angle (for spot lights)
        glm::vec4 color;        ///< xyz = RGB color, w = intensity
        glm::vec4 attenuation;  ///< x=constant, y=linear, z=quadratic, w=outer cone angle
        glm::vec4 metadata;     ///< x=shadowIndex, y=castShadows, z=unused, w=unused
    };

    /**
     * @brief Light source with customizable properties
     *
     * Represents a single light source in the scene with properties
     * specific to each light type (directional, point, spot, area).
     */
    class Light
    {
    public:
        /**
         * @brief Constructor
         * @param type The type of light to create
         */
        Light(LightType type);

        /**
         * @brief Virtual destructor
         */
        virtual ~Light();

        // Light properties
        /**
         * @brief Set the world position of the light
         * @param position The new position
         */
        void setPosition(const glm::vec3& position);

        /**
         * @brief Set the direction the light is pointing
         * @param direction The new direction (will be normalized)
         */
        void setDirection(const glm::vec3& direction);

        /**
         * @brief Set the RGB color of the light
         * @param color The new color
         */
        void setColor(const glm::vec3& color);

        /**
         * @brief Set the brightness multiplier of the light
         * @param intensity The new intensity (must be non-negative)
         */
        void setIntensity(float intensity);

        /**
         * @brief Set the maximum range/radius of the light
         * @param range The new range (must be positive)
         */
        void setRange(float range);

        /**
         * @brief Set whether this light casts shadows
         * @param castShadows True if the light should cast shadows
         */
        void setCastShadows(bool castShadows);

        /**
         * @brief Set the attenuation parameters for point and spot lights
         * @param constant Constant attenuation factor
         * @param linear Linear attenuation factor
         * @param quadratic Quadratic attenuation factor
         */
        void setAttenuation(float constant, float linear, float quadratic);

        // Type-specific setters
        /**
         * @brief Set the inner and outer angles for spot lights
         * @param innerAngle The inner cone angle in radians
         * @param outerAngle The outer cone angle in radians
         */
        void setSpotAngle(float innerAngle, float outerAngle);

        /**
         * @brief Set the inner cone angle for spot lights
         * @param angle The inner cone angle in radians
         */
        void setInnerConeAngle(float angle);

        /**
         * @brief Set the outer cone angle for spot lights
         * @param angle The outer cone angle in radians
         */
        void setOuterConeAngle(float angle);

        /**
         * @brief Set the dimensions for area lights
         * @param size Width and height of the area light
         */
        void setAreaSize(const glm::vec2& size);

        // Getters
        /**
         * @brief Get the type of this light
         * @return The light type
         */
        LightType getType() const;

        /**
         * @brief Get the world position of the light
         * @return The light position
         */
        const glm::vec3& getPosition() const;

        /**
         * @brief Get the direction the light is pointing
         * @return The normalized light direction
         */
        const glm::vec3& getDirection() const;

        /**
         * @brief Get the RGB color of the light
         * @return The light color
         */
        const glm::vec3& getColor() const;

        /**
         * @brief Get the brightness multiplier of the light
         * @return The light intensity
         */
        float getIntensity() const;

        /**
         * @brief Get the maximum range/radius of the light
         * @return The light range
         */
        float getRange() const;

        /**
         * @brief Check if this light casts shadows
         * @return True if the light casts shadows
         */
        bool getCastShadows() const;

        /**
         * @brief Get the attenuation parameters
         * @return Vector3 with constant, linear, quadratic attenuation
         */
        glm::vec3 getAttenuation() const;

        /**
         * @brief Get the inner cone angle for spot lights
         * @return The inner cone angle in radians
         */
        float getInnerConeAngle() const;

        /**
         * @brief Get the outer cone angle for spot lights
         * @return The outer cone angle in radians
         */
        float getOuterConeAngle() const;

        /**
         * @brief Get the area light dimensions
         * @return Width and height of the area light
         */
        const glm::vec2& getAreaSize() const;

        // Utility methods
        /**
         * @brief Get the view matrix for shadow mapping
         * @return View matrix from the light's perspective
         */
        glm::mat4 getViewMatrix() const;

        /**
         * @brief Get the projection matrix for shadow mapping
         * @return Projection matrix appropriate for the light type
         */
        glm::mat4 getProjectionMatrix() const;

        // Shadow parameters
        /**
         * @brief Set the shadow bias to prevent shadow acne
         * @param bias The shadow depth bias value
         */
        void setShadowBias(float bias);

        /**
         * @brief Set the resolution of this light's shadow map
         * @param resolution The shadow map resolution (will be rounded to power of 2)
         */
        void setShadowResolution(int resolution);

        /**
         * @brief Pack light data for shader use
         * @return Packed light data structure for GPU
         */
        LightData packLightData() const;

    private:
        LightType m_type;
        glm::vec3 m_position;
        glm::vec3 m_direction;
        glm::vec3 m_color;
        float m_intensity;
        float m_range;
        bool m_castShadows;

        // Attenuation parameters
        float m_constantAttenuation;
        float m_linearAttenuation;
        float m_quadraticAttenuation;

        // Spot light properties
        float m_spotInnerAngle;
        float m_spotOuterAngle;

        // Area light properties
        glm::vec2 m_areaSize;

        // Shadow properties
        float m_shadowBias;
        int m_shadowResolution;
    };

} // namespace PixelCraft::Rendering 