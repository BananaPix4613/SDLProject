// -------------------------------------------------------------------------
// ShadowStage.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/RenderStage.h"

#include <memory>
#include <vector>

namespace PixelCraft::Rendering
{
    class Shader;
    class RenderTarget;
    class Light;
    class LightManager;
}

namespace PixelCraft::ECS
{
    class Registry;
}

namespace PixelCraft::Rendering::Stages
{

    /**
     * @brief Render stage for shadow map generation
     * 
     * The shadow stage generates shadow maps for all shadow-casting lights
     * in the scene. Supports directional, point, and spot light shadows
     * with cascaded shadow maps for directional lights.
     */
    class ShadowStage : public RenderStage
    {
    public:
        /**
         * @brief Constructor
         */
        ShadowStage();

        /**
         * @brief Destructor
         */
        virtual ~ShadowStage();

        /**
         * @brief Initialize the shadow stage
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Clean up resources
         */
        void shutdown() override;

        /**
         * @brief Execute shadow map generation
         * @param context Current render context
         */
        void execute(const std::shared_ptr<RenderContext> context) override;

        /**
         * @brief Get the stage type name
         * @return Type name string
         */
        std::string getTypeName() const override
        {
            return "ShadowStage";
        }

        /**
         * @brief Set the shadow shader to use for rendering
         * @param shader Shadow mapping shader program
         */
        void setShadowShader(std::shared_ptr<Shader> shader);

        /**
         * @brief Get the current shadow shader
         * @return Shadow shader program
         */
        std::shared_ptr<Shader> getShadowShader() const;

        /**
         * @brief Set the light manager for shadow casting lights
         * @param lightManager Light manager instance
         */
        void setLightManager(std::shared_ptr<LightManager> lightManager);

        /**
         * @brief Set the entity registry to render from
         * @param registry Entity registry
         */
        void setRegistry(std::shared_ptr<ECS::Registry> registry);

        /**
         * @brief Set the shadow map resolution
         * @param resolution Resolution for shadow maps (e.g. 1024, 2048)
         */
        void setShadowMapResolution(int resolution);

        /**
         * @brief Get the current shadow map resolution
         * @return Shadow map resolution
         */
        int getShadowMapResolution() const;

        /**
         * @brief Set the shadow bias to prevent shadow acne
         * @param bias Shadow depth bias value
         */
        void setShadowBias(float bias);

        /**
         * @brief Get the current shadow bias
         * @return Shadow bias value
         */
        float getShadowBias() const;

        /**
         * @brief Set the number of cascades for directional light shadows
         * @param cascadeCount Number of cascades (typically 2-4)
         */
        void setCascadeCount(int cascadeCount);

        /**
         * @brief Get the current cascade count
         * @return Number of cascades
         */
        int getCascadeCount() const;

        /**
         * @brief Set the cascade split lambda for distributing cascade distances
         * @param lambda Lambda value (0.0-1.0, 0=uniform, 1=logarithmic)
         */
        void setCascadeSplitLambda(float lambda);

        /**
         * @brief Get the current cascade split lambda
         * @return Cascade split lambda value
         */
        float getCascadeSplitLambda() const;

        /**
         * @brief Enable or disable soft shadows
         * @param enabled Whether to enable soft shadows
         */
        void setSoftShadowsEnabled(bool enabled);

        /**
         * @brief Check if soft shadows are enabled
         * @return True if soft shadows are enabled
         */
        bool isSoftShadowsEnabled() const;

    private:
        /**
         * @brief Render shadow map for a specific light
         * @param light Light to render shadows for
         * @param context Render context
         */
        void renderLightShadowMap(std::shared_ptr<Light> light, const std::shared_ptr<RenderContext> context);

        /**
         * @brief Render directional light shadow map with cascades
         * @param light Directional light
         * @param context Render context
         */
        void renderDirectionalShadowMap(std::shared_ptr<Light> light, const std::shared_ptr<RenderContext> context);

        /**
         * @brief Render point light shadow map (cube map)
         * @param light Point light
         * @param context Render context
         */
        void renderPointShadowMap(std::shared_ptr<Light> light, const std::shared_ptr<RenderContext> context);

        /**
         * @brief Render spot light shadow map
         * @param light Spot light
         * @param context Render context
         */
        void renderSpotShadowMap(std::shared_ptr<Light> light, const std::shared_ptr<RenderContext> context);

        /**
         * @brief Setup render states for shadow mapping
         */
        void setupShadowRenderState();

        /**
         * @brief Restore render states after shadow mapping
         */
        void restoreRenderState();

        /**
         * @brief Configure LightManager with current shadow settings
         */
        void configureLightManagerShadows();

        /**
         * @brief Get shadow map from LightManager for a specific light
         * @param light The light to get shadow map for
         * @param cascadeIndex Optional cascade index for directional lights
         * @return Shadow map render target from LightManager
         */
        std::shared_ptr<RenderTarget> getShadowMapFromLightManager(std::shared_ptr<Light> light, int cascadeIndex = -1);

        /**
         * @brief Render shadow casting entities with the given light view-projection matrix
         * @param lightViewProj The light's view-projection matrix
         */
        void renderShadowCasters(const glm::mat4& lightViewProj);

        /** Shader for shadow mapping */
        std::shared_ptr<Shader> m_shadowShader;

        /** Light manager */
        std::shared_ptr<LightManager> m_lightManager;

        /** Entity registry */
        std::shared_ptr<ECS::Registry> m_registry;

        /** Shadow map resolution */
        int m_shadowMapResolution;

        /** Shadow depth bias */
        float m_shadowBias;

        /** Number of cascades for directional lights */
        int m_cascadeCount;

        /** Cascade split lambda */
        float m_cascadeSplitLambda;

        /** Whether soft shadows are enabled */
        bool m_softShadowsEnabled;

        /** Shadow map render targets */
        std::vector<std::shared_ptr<RenderTarget>> m_shadowMaps;

        /** Cube map shadow targets for point lights */
        std::vector<std::shared_ptr<RenderTarget>> m_cubeShadowMaps;

        /** Profiling marker for performance tracking */
        std::string m_profilingMarker;
    };

} // namespace PixelCraft::Rendering::Stages
