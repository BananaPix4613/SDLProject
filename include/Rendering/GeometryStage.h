// -------------------------------------------------------------------------
// GeometryStage.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/RenderStage.h"
#include "Rendering/Shader.h"
#include "ECS/Registry.h"

namespace PixelCraft::Rendering
{

    /**
     * @brief Render stage for geometry rendering
     */
    class GeometryStage : public RenderStage
    {
    public:
        /**
         * @brief Constructor
         */
        GeometryStage();

        /**
         * @brief Destructor
         */
        ~GeometryStage() override;

        /**
         * @brief Initialize the geometry stage
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Clean up resources
         */
        void shutdown() override;

        /**
         * @brief Execute the geometry rendering
         * @param context Current render context
         */
        void execute(const RenderContext& context) override;

        /**
         * @brief Set the shader to use for rendering
         * @param shader Shader program
         */
        void setShader(std::shared_ptr<Shader> shader);

        /**
         * @brief Get the current shader
         * @return Shader program
         */
        std::shared_ptr<Shader> getShader() const;

        /**
         * @brief Set the entity registry to render from
         * @param registry Entity registry
         */
        void setRegistry(std::shared_ptr<ECS::Registry> registry);

    private:
        /** Shader for rendering */
        std::shared_ptr<Shader> m_shader;

        /** Entity registry */
        std::shared_ptr<ECS::Registry> m_registry;

        /** Render only opaque geometry */
        bool m_opaqueOnly;

        /** Render only transparent geometry */
        bool m_transparentOnly;

        /** Whether depth testing is enabled */
        bool m_depthTest;

        /** Whether depth writing is enabled */
        bool m_depthWrite;

        /** Whether to use instancing when possible */
        bool m_useInstancing;

        /** Whether to sort objects before rendering */
        bool m_sortObjects;

        /** Profiling marker for performance tracking */
        uint32_t m_profilingMarker;

        /**
         * @brief Render a single object
         * @param context Render context
         * @param entity Entity ID
         */
        void renderObject(const RenderContext& context, ECS::EntityID entity);

        /**
         * @brief Setup render states
         */
        void setupRenderState();

        /**
         * @brief Restore render states
         */
        void restoreRenderState();
    };

} // namespace PixelCraft::Rendering