// -------------------------------------------------------------------------
// VelocityStage.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/RenderStage.h"

#include <memory>
#include <glm/glm.hpp>

namespace PixelCraft::Rendering
{
    class Shader;
    class RenderContext;
}

namespace PixelCraft::ECS
{
    class Registry;
    using EntityID = uint32_t;
}

namespace PixelCraft::Rendering::Stages
{

    /**
     * @brief Render stage for velocity buffer generation
     * 
     * The velocity stage generates per-pixel motion vectors for temporal effects
     * such as motion blur, temporal anti-aliasing (TAA), and temporal upsampling.
     */
    class VelocityStage : public RenderStage
    {
    public:
        /**
         * @brief Constructor
         */
        VelocityStage();

        /**
         * @brief Destructor
         */
        virtual ~VelocityStage();

        /**
         * @brief Initialize the velocity stage
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Clean up resources
         */
        void shutdown() override;

        /**
         * @brief Execute the velocity buffer generation pass
         * @param context Current render context
         */
        void execute(const std::shared_ptr<RenderContext> context) override;

        /**
         * @brief Get the stage type name
         * @return Type name string
         */
        std::string getTypeName() const override
        {
            return "VelocityStage";
        }

        /**
         * @brief Set the velocity shader
         * @param shader Shader program for velocity generation
         */
        void setVelocityShader(std::shared_ptr<Shader> shader);

        /**
         * @brief Get the velocity shader
         * @return Velocity shader program
         */
        std::shared_ptr<Shader> getVelocityShader() const;

        /**
         * @brief Set the entity registry to render from
         * @param registry Entity registry
         */
        void setRegistry(std::shared_ptr<ECS::Registry> registry);

        /**
         * @brief Set the previous view-projection matrix
         * @param matrix Previous frame's view-projection matrix
         */
        void setPreviousViewProjectionMatrix(const glm::mat4& matrix);

        /**
         * @brief Get the previous view-projection matrix
         * @return Previous frame's view-projection matrix
         */
        const glm::mat4& getPreviousViewProjectionMatrix() const;

        /**
         * @brief Enable or disable camera motion vectors
         * @param enabled Whether to include camera motion
         */
        void setCameraMotionEnabled(bool enabled);

        /**
         * @brief Check if camera motion is enabled
         * @return True if camera motion is included
         */
        bool isCameraMotionEnabled() const;

        /**
         * @brief Set the velocity scale factor
         * @param scale Scale factor for velocity vectors
         */
        void setVelocityScale(float scale);

        /**
         * @brief Get the velocity scale factor
         * @return Velocity scale factor
         */
        float getVelocityScale() const;

        /**
         * @brief Set whether to encode velocity in normalized device coordinates
         * @param enabled Whether to use NDC encoding
         */
        void setNDCEncoding(bool enabled);

        /**
         * @brief Check if NDC encoding is enabled
         * @return True if using NDC encoding
         */
        bool isNDCEncodingEnabled() const;

        /**
         * @brief Set the maximum velocity magnitude for clamping
         * @param maxVelocity Maximum velocity in pixels
         */
        void setMaxVelocity(float maxVelocity);

        /**
         * @brief Get the maximum velocity magnitude
         * @return Maximum velocity in pixels
         */
        float getMaxVelocity() const;

    protected:
        /**
         * @brief Render velocity buffer for dynamic objects
         * @param context Render context
         */
        void renderDynamicVelocity(const std::shared_ptr<RenderContext> context);

        /**
         * @brief Generate camera motion velocity
         * @param context Render context
         */
        void generateCameraVelocity(const std::shared_ptr<RenderContext> context);

        /**
         * @brief Setup render state for velocity generation
         */
        void setupVelocityRenderState();

        /**
         * @brief Restore render state after velocity generation
         */
        void restoreRenderState();

        /**
         * @brief Update previous frame matrices for objects
         * @param entity Entity ID
         */
        void updatePreviousTransform(ECS::EntityID entity);

    private:
        /** Shader for velocity buffer generation */
        std::shared_ptr<Shader> m_velocityShader;

        /** Shader for camera motion velocity */
        std::shared_ptr<Shader> m_cameraVelocityShader;

        /** Entity registry */
        std::shared_ptr<ECS::Registry> m_registry;

        /** Previous frame's view-projection matrix */
        glm::mat4 m_previousViewProjectionMatrix = glm::mat4(1.0f);

        /** Whether to include camera motion */
        bool m_cameraMotionEnabled = true;

        /** Velocity scale factor */
        float m_velocityScale = 1.0f;

        /** Whether to encode velocity in NDC */
        bool m_ndcEncoding = false;

        /** Maximum velocity magnitude for clamping */
        float m_maxVelocity = 64.0f;

        /** Map of entity to previous transform matrix */
        std::unordered_map<ECS::EntityID, glm::mat4> m_previousTransforms;
    };

} // namespace PixelCraft::Rendering::Stages