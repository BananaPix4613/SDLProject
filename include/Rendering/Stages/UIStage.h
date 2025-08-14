// -------------------------------------------------------------------------
// UIStage.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/RenderStage.h"

#include <memory>

namespace PixelCraft::Rendering
{
    class Shader;
    class Mesh;
}

namespace PixelCraft::ECS
{
    class Registry;
}

namespace PixelCraft::Rendering::Stages
{

    /**
     * @brief Render stage for user interface elements
     * 
     * The UI stage renders all user interface elements on top of the 3D scene
     * with proper depth sorting and transparency handling.
     */
    class UIStage : public RenderStage
    {
    public:
        /**
         * @brief Constructor
         */
        UIStage();

        /**
         * @brief Destructor
         */
        virtual ~UIStage();

        /**
         * @brief Initialize the UI stage
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Clean up resources
         */
        void shutdown() override;

        /**
         * @brief Execute UI rendering
         * @param context Current render context
         */
        void execute(const std::shared_ptr<RenderContext> context) override;

        /**
         * @brief Get the stage type name
         * @return Type name string
         */
        std::string getTypeName() const override
        {
            return "UIStage";
        }

        /**
         * @brief Set the UI shader to use for rendering
         * @param shader UI shader program
         */
        void setUIShader(std::shared_ptr<Shader> shader);

        /**
         * @brief Get the current UI shader
         * @return UI shader program
         */
        std::shared_ptr<Shader> getUIShader() const;

        /**
         * @brief Set the entity registry to render from
         * @param registry Entity registry
         */
        void setRegistry(std::shared_ptr<ECS::Registry> registry);

        /**
         * @brief Set the screen dimensions for UI scaling
         * @param width Screen width in pixels
         * @param height Screen height in pixels
         */
        void setScreenDimensions(int width, int height);

        /**
         * @brief Get the current screen width
         * @return Screen width in pixels
         */
        int getScreenWidth() const;

        /**
         * @brief Get the current screen height
         * @return Screen height in pixels
         */
        int getScreenHeight() const;

        /**
         * @brief Enable or disable UI depth testing
         * @param enabled Whether depth testing is enabled for UI
         */
        void setDepthTestEnabled(bool enabled);

        /**
         * @brief Check if UI depth testing is enabled
         * @return True if depth testing is enabled
         */
        bool isDepthTestEnabled() const;

    private:
        /**
         * @brief Setup render states for UI rendering
         */
        void setupUIRenderState();

        /**
         * @brief Restore render states after UI rendering
         */
        void restoreRenderState();

        /**
         * @brief Setup orthographic projection for UI
         */
        void setupUIProjection();

        /** Shader for UI rendering */
        std::shared_ptr<Shader> m_uiShader;

        /** Entity registry */
        std::shared_ptr<ECS::Registry> m_registry;

        /** Screen dimensions */
        int m_screenWidth;
        int m_screenHeight;

        /** Whether depth testing is enabled for UI */
        bool m_depthTestEnabled;

        /** Profiling marker */
        std::string m_profilingMarker;
    };

} // namespace PixelCraft::Rendering::Stages