// -------------------------------------------------------------------------
// SkyboxStage.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/RenderStage.h"
#include "Utility/Color.h"

#include <memory>
#include <glm/glm.hpp>

namespace PixelCraft::Rendering
{
    class Shader;
    class Texture;
    class Mesh;
    class RenderContext;
}

namespace PixelCraft::Rendering::Stages
{

    /**
     * @brief Render stage for skybox/environment rendering
     * 
     * The skybox stage renders environment maps, skyboxes, and background elements.
     * It supports cubemap textures, equirectangular maps, and procedural skyboxes.
     */
    class SkyboxStage : public RenderStage
    {
    public:
        /**
         * @brief Skybox rendering mode
         */
        enum class SkyboxMode
        {
            Cubemap,        ///< Traditional cubemap texture
            Equirectangular, ///< Equirectangular HDR map
            Procedural,     ///< Procedurally generated sky
            Gradient        ///< Simple gradient background
        };

        /**
         * @brief Constructor
         */
        SkyboxStage();

        /**
         * @brief Destructor
         */
        virtual ~SkyboxStage();

        /**
         * @brief Initialize the skybox stage
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Clean up resources
         */
        void shutdown() override;

        /**
         * @brief Execute the skybox rendering pass
         * @param context Current render context
         */
        void execute(const std::shared_ptr<RenderContext> context) override;

        /**
         * @brief Get the stage type name
         * @return Type name string
         */
        std::string getTypeName() const override
        {
            return "SkyboxStage";
        }

        /**
         * @brief Set the skybox rendering mode
         * @param mode The rendering mode to use
         */
        void setSkyboxMode(SkyboxMode mode);

        /**
         * @brief Get the current skybox mode
         * @return The current rendering mode
         */
        SkyboxMode getSkyboxMode() const;
        
        /**
         * @brief Set the skybox shader
         * @param shader Shader program for skybox rendering
         */
        void setSkyboxShader(std::shared_ptr<Shader> shader);

        /**
         * @brief Get the current skybox shader
         * @return Skybox shader program
         */
        std::shared_ptr<Shader> getSkyboxShader() const;

        /**
         * @brief Set the cubemap texture
         * @param cubemap Cubemap texture for skybox
         */
        void setCubemapTexture(std::shared_ptr<Texture> cubemap);

        /**
         * @brief Set the equirectangular texture
         * @param texture Equirectangular HDR texture
         */
        void setEquirectangularTexture(std::shared_ptr<Texture> texture);

        /**
         * @brief Set gradient colors for gradient mode
         * @param topColor Color at the top of the sky
         * @param bottomColor Color at the bottom of the sky
         */
        void setGradientColors(const Utility::Color& topColor, const Utility::Color& bottomColor);

        /**
         * @brief Set the sun direction for procedural sky
         * @param direction Normalized sun direction vector
         */
        void setSunDirection(const glm::vec3& direction);

        /**
         * @brief Set the sun color and intensity
         * @param color Sun color
         * @param intensity Sun intensity multiplier
         */
        void setSunProperties(const Utility::Color& color, float intensity);

        /**
         * @brief Set atmosphere properties for procedural sky
         * @param rayleighScattering Rayleigh scattering coefficient
         * @param mieScattering Mie scattering coefficient
         * @param atmosphereHeight Height of the atmosphere
         */
        void setAtmosphereProperties(float rayleighScattering, float mieScattering, float atmosphereHeight);

        /**
         * @brief Set the exposure for HDR skyboxes
         * @param exposure Exposure value
         */
        void setExposure(float exposure);

        /**
         * @brief Get the current exposure value
         * @return Exposure value
         */
        float getExposure() const;

        /**
         * @brief Enable or disable skybox rotation
         * @param enabled Whether rotation is enabled
         * @param speed Rotation speed in radians per second
         */
        void setRotation(bool enabled, float speed = 0.0f);

    protected:
        /**
         * @brief Render the skybox based on current mode
         * @param context Render context
         */
        void renderSkybox(const std::shared_ptr<RenderContext> context);

        /**
         * @brief Create the skybox cube mesh if needed
         */
        void createSkyboxMesh();

        /**
         * @brief Setup render states for skybox rendering
         */
        void setupSkyboxRenderState();

        /**
         * @brief Restore render states after skybox rendering
         */
        void restoreSkyboxRenderState();

    private:
        /** Current skybox rendering mode */
        SkyboxMode m_mode = SkyboxMode::Cubemap;

        /** Shader for skybox rendering */
        std::shared_ptr<Shader> m_skyboxShader;

        /** Cubemap texture */
        std::shared_ptr<Texture> m_cubemapTexture;

        /** Equirectangular texture */
        std::shared_ptr<Texture> m_equirectangularTexture;

        /** Skybox cube mesh */
        std::shared_ptr<Mesh> m_skyboxMesh;

        /** Gradient colors */
        Utility::Color m_gradientTopColor = Utility::Color(0.5f, 0.7f, 1.0f);
        Utility::Color m_gradientBottomColor = Utility::Color(1.0f, 1.0f, 1.0f);

        /** Sun properties for procedural sky */
        glm::vec3 m_sunDirection = glm::vec3(0.0f, 0.7f, 0.7f);
        Utility::Color m_sunColor = Utility::Color(1.0f, 0.95f, 0.8f);
        float m_sunIntensity = 1.0f;

        /** Atmosphere properties */
        float m_rayleighScattering = 0.0025f;
        float m_mieScattering = 0.0003f;
        float m_atmosphereHeight = 100000.0f;

        /** HDR exposure */
        float m_exposure = 1.0f;

        /** Rotation properties */
        bool m_rotationEnabled = false;
        float m_rotationSpeed = 0.0f;
        float m_currentRotation = 0.0f;
    };

} // namespace PixelCraft::Rendering::Stages