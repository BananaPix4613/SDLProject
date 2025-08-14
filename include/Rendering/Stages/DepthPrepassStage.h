// -------------------------------------------------------------------------
// DepthPrepassStage.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/RenderStage.h"
#include "Rendering/Material.h"
#include "ECS/ComponentTypes.h"

#include <memory>

namespace PixelCraft::ECS
{
    class Registry;
}

namespace PixelCraft::Rendering
{
    class Shader;
    class RenderTarget;
    class RenderContext;
    class RenderSystem;
    struct RenderableObject;
}

namespace PixelCraft::Rendering::Stages
{

    /**
     * @brief Material filtering modes for depth prepass
     */
    enum class MaterialFilterMode
    {
        OpaqueOnly,     ///< Render only opaque materials
        AlphaTestedOnly, ///< Render only alpha-tested materials
        Both           ///< Render both opaque and alpha-tested materials
    };

    /**
     * @brief Render stage for depth pre-pass optimized for forward+ rendering
     *
     * The depth pre-pass stage renders only depth information to establish
     * depth buffer values before the main geometry pass. This reduces overdraw
     * and improves performance for complex scenes. Optimized for forward+ 
     * rendering with early-z rejection and proper material filtering.
     */
    class DepthPrepassStage : public RenderStage
    {
    public:
        /**
         * @brief Constructor
         */
        DepthPrepassStage();

        /**
         * @brief Destructor
         */
        virtual ~DepthPrepassStage();

        /**
         * @brief Initialize the depth pre-pass stage
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Clean up resources
         */
        void shutdown() override;

        /**
         * @brief Execute the depth pre-pass
         * @param context Current render context
         */
        void execute(const std::shared_ptr<RenderContext> context) override;

        /**
         * @brief Get the stage type name
         * @return Type name string
         */
        std::string getTypeName() const override
        {
            return "DepthPrepassStage";
        }

        /**
         * @brief Set the depth shader to use for rendering
         * @param shader Depth-only shader program
         */
        void setDepthShader(std::shared_ptr<Shader> shader);

        /**
         * @brief Get the current depth shader
         * @return Depth shader program
         */
        std::shared_ptr<Shader> getDepthShader() const;

        /**
         * @brief Set the entity registry to render from
         * @param registry Entity registry
         */
        void setRegistry(std::shared_ptr<ECS::Registry> registry);

        /**
         * @brief Set the render system to get render queues from
         * @param renderSystem Pointer to render system
         */
        void setRenderSystem(std::shared_ptr<RenderSystem> renderSystem);

        /**
         * @brief Set the material filtering mode
         * @param mode Material filtering mode
         */
        void setMaterialFilterMode(MaterialFilterMode mode);

        /**
         * @brief Get the current material filtering mode
         * @return Material filtering mode
         */
        MaterialFilterMode getMaterialFilterMode() const;

        /**
         * @brief Set whether to render only opaque objects (legacy compatibility)
         * @param opaqueOnly True to render only opaque objects
         */
        void setOpaqueOnly(bool opaqueOnly);

        /**
         * @brief Check if only opaque objects are rendered (legacy compatibility)
         * @return True if only opaque objects are rendered
         */
        bool isOpaqueOnly() const;

        /**
         * @brief Set the alpha threshold for alpha-tested materials
         * @param threshold Alpha threshold value [0.0-1.0]
         */
        void setAlphaThreshold(float threshold);

        /**
         * @brief Get the current alpha threshold
         * @return Alpha threshold value
         */
        float getAlphaThreshold() const;

        /**
         * @brief Set the depth bias for shadow mapping compatibility
         * @param bias Depth bias value
         */
        void setDepthBias(float bias);

        /**
         * @brief Get the current depth bias
         * @return Depth bias value
         */
        float getDepthBias() const;

        /**
         * @brief Enable optimizations for orthographic projection (isometric views)
         * @param enable True to enable orthographic optimizations
         */
        void setOrthographicOptimization(bool enable);

        /**
         * @brief Check if orthographic optimizations are enabled
         * @return True if orthographic optimizations are enabled
         */
        bool isOrthographicOptimizationEnabled() const;

        /**
         * @brief Set the early-z rejection mode
         * @param enable True to enable early-z rejection for subsequent passes
         */
        void setEarlyZRejection(bool enable);

        /**
         * @brief Check if early-z rejection is enabled
         * @return True if early-z rejection is enabled
         */
        bool isEarlyZRejectionEnabled() const;

    private:
        /**
         * @brief Render ECS entities for depth only
         * @param context Render context
         */
        void renderECSEntityDepth(const std::shared_ptr<RenderContext> context);

        /**
         * @brief Render geometry objects for depth only
         * @param context Render context
         */
        void renderGeometryDepth(const std::shared_ptr<RenderContext> context);

        /**
         * @brief Render a single render object for depth only
         * @param renderable The renderable to render
         * @param context Render context
         */
        void renderObjectDepth(const RenderableObject& renderable,
                               const std::shared_ptr<RenderContext> context);

        /**
         * @brief Setup render states for depth pre-pass
         * @param context Render context for projection type detection
         */
        void setupDepthRenderState(const RenderContext& context);

        /**
         * @brief Restore render states after depth pre-pass
         */
        void restoreRenderState();

        /**
         * @brief Check if a material should be rendered based on filter mode
         * @param material Material to check
         * @return True if material should be rendered
         */
        bool shouldRenderMaterial(std::shared_ptr<Material> material) const;

        /**
         * @brief Detect if the current projection is orthographic
         * @param context Render context
         * @return True if projection is orthographic
         */
        bool isOrthographicProjection(const RenderContext& context) const;

        /** Shader for depth-only rendering */
        std::shared_ptr<Shader> m_depthShader;

        /** Entity registry */
        std::shared_ptr<ECS::Registry> m_registry;

        /** Render system for accessing render queues */
        std::shared_ptr<RenderSystem> m_renderSystem;

        /** Material filtering mode */
        MaterialFilterMode m_materialFilterMode;

        /** Alpha threshold for alpha-tested materials */
        float m_alphaThreshold;

        /** Depth bias for shadow mapping */
        float m_depthBias;

        /** Whether to enable orthographic projection optimizations */
        bool m_orthographicOptimization;

        /** Whether to enable early-z rejection */
        bool m_earlyZRejection;

        /** Whether depth testing is enabled */
        bool m_depthTest;

        /** Whether depth writing is enabled */
        bool m_depthWrite;
    };

} // namespace PixelCraft::Rendering::Stages
