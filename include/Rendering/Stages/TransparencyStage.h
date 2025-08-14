// -------------------------------------------------------------------------
// TransparencyStage.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/RenderStage.h"

#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace PixelCraft::Rendering
{
    class Shader;
    class RenderContext;
    class ForwardPlusShaderManager;
    class Material;
    class Mesh;
    enum class BlendMode;
}

namespace PixelCraft::ECS
{
    class Registry;
    class TransformComponent;
    class MeshRendererComponent;
    using EntityID = uint32_t;
}

namespace PixelCraft::Rendering::Stages
{

    struct TransparentRenderItem
    {
        ECS::EntityID entity;
        ECS::TransformComponent* transform;        // Pointer from View
        ECS::MeshRendererComponent* meshRenderer;  // Pointer from View
        glm::vec3 position;
        float distanceFromCamera;
        std::shared_ptr<Rendering::Material> material;
        std::shared_ptr<Rendering::Mesh> mesh;
    };

    /**
     * @brief Forward+ transparency stage with clustered lighting integration
     * 
     * The transparency stage handles rendering of transparent objects with proper
     * depth sorting, alpha blending, and order-independent transparency techniques
     * specifically designed for forward+ rendering with clustered lighting.
     */
    class TransparencyStage : public RenderStage
    {
    public:
        /**
         * @brief Transparency technique optimized for forward+ rendering
         */
        enum class TransparencyTechnique
        {
            AlphaBlending,      ///< Traditional alpha blending (back-to-front) with clusters
            WeightedBlended,    ///< Weighted blended OIT with forward+ optimization
            DepthPeeling,       ///< Depth peeling with clustered lighting
            DualDepthPeeling    ///< Dual depth peeling with forward+ integration
        };

        /**
         * @brief Constructor
         */
        TransparencyStage();

        /**
         * @brief Destructor
         */
        virtual ~TransparencyStage();

        /**
         * @brief Initialize the transparency stage with forward+ support
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Clean up resources
         */
        void shutdown() override;

        /**
         * @brief Execute the forward+ transparency rendering pass
         * @param context Current render context
         */
        void execute(const std::shared_ptr<RenderContext> context) override;

        /**
         * @brief Get the stage type name
         * @return Type name string
         */
        std::string getTypeName() const override
        {
            return "TransparencyStage";
        }

        /**
         * @brief Set the ForwardPlusShaderManager for automatic cluster integration
         * @param shaderManager Forward+ shader manager instance
         */
        void setForwardPlusShaderManager(std::shared_ptr<ForwardPlusShaderManager> shaderManager);

        /**
         * @brief Get the ForwardPlusShaderManager
         * @return Forward+ shader manager instance
         */
        std::shared_ptr<ForwardPlusShaderManager> getForwardPlusShaderManager() const;

        /**
         * @brief Set the transparency technique to use
         * @param technique The transparency technique
         */
        void setTransparencyTechnique(TransparencyTechnique technique);

        /**
         * @brief Get the current transparency technique
         * @return The current technique
         */
        TransparencyTechnique getTransparencyTechnique() const;

        /**
         * @brief Set the shader for transparent object rendering
         * @param shader Shader program with forward+ support
         */
        void setTransparencyShader(std::shared_ptr<Shader> shader);

        /**
         * @brief Get the transparency shader
         * @return Shader program
         */
        std::shared_ptr<Shader> getTransparencyShader() const;

        /**
         * @brief Set the entity registry to render from
         * @param registry Entity registry
         */
        void setRegistry(std::shared_ptr<ECS::Registry> registry);

        /**
         * @brief Set the maximum number of depth peeling layers
         * @param layers Maximum layers (for depth peeling techniques)
         */
        void setMaxDepthLayers(int layers);

        /**
         * @brief Get the maximum depth layers
         * @return Maximum depth layers
         */
        int getMaxDepthLayers() const;

        /**
         * @brief Enable or disable alpha-to-coverage
         * @param enabled Whether alpha-to-coverage is enabled
         */
        void setAlphaToCoverageEnabled(bool enabled);

        /**
         * @brief Check if alpha-to-coverage is enabled
         * @return True if enabled
         */
        bool isAlphaToCoverageEnabled() const;

        /**
         * @brief Set the alpha discard threshold
         * @param threshold Alpha threshold below which fragments are discarded
         */
        void setAlphaDiscardThreshold(float threshold);

        /**
         * @brief Get the alpha discard threshold
         * @return Alpha discard threshold
         */
        float getAlphaDiscardThreshold() const;

        /**
         * @brief Enable or disable soft particles
         * @param enabled Whether soft particles are enabled
         */
        void setSoftParticlesEnabled(bool enabled);

        /**
         * @brief Check if soft particles are enabled
         * @return True if enabled
         */
        bool isSoftParticlesEnabled() const;

        /**
         * @brief Set the depth buffer from Z-prepass for proper depth testing
         * @param depthBuffer Depth buffer from Z-prepass stage
         */
        void setDepthBuffer(std::shared_ptr<RenderTarget> depthBuffer);

        /**
         * @brief Get the depth buffer used for testing
         * @return Depth buffer render target
         */
        std::shared_ptr<RenderTarget> getDepthBuffer() const;

    protected:
        /**
         * @brief Render transparent objects with forward+ alpha blending
         * @param context Render context
         * @param items Render items to render
         */
        void renderAlphaBlended(const std::shared_ptr<RenderContext> context,
                                const std::vector<TransparentRenderItem>& items);

        /**
         * @brief Render with forward+ weighted blended OIT
         * @param context Render context
         * @param items Render items to render
         */
        void renderWeightedBlended(const std::shared_ptr<RenderContext> context,
                                  const std::vector<TransparentRenderItem>& items);

        /**
         * @brief Render with forward+ depth peeling
         * @param context Render context
         */
        void renderDepthPeeling(const std::shared_ptr<RenderContext> context);

        /**
         * @brief Collect and sort transparent items with forward+ optimization
         * @param context Render context
         * @return Sorted list of render items
         */
        std::vector<TransparentRenderItem> collectTransparentItems(const std::shared_ptr<RenderContext> context);

        /**
         * @brief Sort items for transparency rendering with cluster considerations
         * @param items Render items to sort
         */
        void sortTransparentItems(std::vector<TransparentRenderItem>& items);

        /**
         * @brief Create resources for forward+ OIT techniques
         */
        void createOITResources();

        /**
         * @brief Setup render state for forward+ transparency
         * @param blendMode The blend mode to use
         */
        void setupTransparencyRenderState(BlendMode blendMode);

        /**
         * @brief Restore render state after transparency
         */
        void restoreRenderState();

        /**
         * @brief Setup depth testing against Z-prepass buffer
         * @param shader Shader to configure depth testing for
         */
        void setupDepthTesting(std::shared_ptr<Shader> shader);

        /**
         * @brief Render a fullscreen quad for OIT composition
         */
        void renderFullscreenQuad();

        /**
         * @brief Calculate cluster index for a world position
         * @param worldPos World position
         * @param context Render context for transformations
         * @return Cluster index, or -1 if outside cluster bounds
         */
        int calculateClusterIndex(const glm::vec3& worldPos, const std::shared_ptr<RenderContext> context) const;

    private:
        /** Forward+ shader manager for cluster integration */
        std::shared_ptr<ForwardPlusShaderManager> m_forwardPlusShaderManager;

        /** Current transparency technique */
        TransparencyTechnique m_technique = TransparencyTechnique::AlphaBlending;

        /** Shader for forward+ transparency rendering */
        std::shared_ptr<Shader> m_transparencyShader;

        /** Composite shader for OIT techniques */
        std::shared_ptr<Shader> m_compositeShader;

        /** Entity registry */
        std::shared_ptr<ECS::Registry> m_registry;

        /** Depth buffer from Z-prepass for proper depth testing */
        std::shared_ptr<RenderTarget> m_depthBuffer;

        /** Maximum depth layers for depth peeling */
        int m_maxDepthLayers = 4;

        /** Whether alpha-to-coverage is enabled */
        bool m_alphaToCoverageEnabled = false;

        /** Alpha discard threshold */
        float m_alphaDiscardThreshold = 0.01f;

        /** Whether soft particles are enabled */
        bool m_softParticlesEnabled = true;

        /** Render targets for forward+ OIT techniques */
        std::shared_ptr<RenderTarget> m_accumulationTarget;
        std::shared_ptr<RenderTarget> m_revealageTarget;
        std::shared_ptr<RenderTarget> m_output;
        std::vector<std::shared_ptr<RenderTarget>> m_depthLayers;
    };

} // namespace PixelCraft::Rendering::Stages
