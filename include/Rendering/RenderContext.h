// -------------------------------------------------------------------------
// RenderContext.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <glm/glm.hpp>
#include "Utility/Frustum.h"

namespace PixelCraft::Rendering
{

    // Forward declaration
    class Shader;

    /**
     * @brief Manages rendering state including matrices, camera info, and viewport
     * 
     * RenderContext stores and manages the current rendering state including view and
     * projection matrices, camera information, and other rendering parameters.
     */
    class RenderContext
    {
    public:
        /**
         * @brief Constructor with default initialization
         */
        RenderContext();

        /**
         * @brief Destructor
         */
        ~RenderContext();

        /**
         * @brief Set the view matrix
         * @param view The new view matrix
         */
        void setViewMatrix(const glm::mat4& view);

        /**
         * @brief Set the projection matrix
         * @param projection The new projection matrix
         */
        void setProjectionMatrix(const glm::mat4& projection);

        /**
         * @brief Update the combined view-projection matrix
         */
        void updateMatrices();

        /**
         * @brief Get the current view matrix
         * @return The view matrix
         */
        const glm::mat4& getViewMatrix() const;

        /**
         * @brief Get the current projection matrix
         * @return The projection matrix
         */
        const glm::mat4& getProjectionMatrix() const;

        /**
         * @brief Get the combined view-projection matrix
         * @return The view-projection matrix
         */
        const glm::mat4& getViewProjectionMatrix() const;

        /**
         * @brief Set the camera position
         * @param position The new camera position in world space
         */
        void setCameraPosition(const glm::vec3& position);

        /**
         * @brief Set the camera direction
         * @param direction The new camera direction vector
         */
        void setCameraDirection(const glm::vec3& direction);

        /**
         * @brief Get the current camera position
         * @return The camera position in world space
         */
        const glm::vec3& getCameraPosition() const;

        /**
         * @brief Get the current camera direction
         * @return The camera direction vector
         */
        const glm::vec3& getCameraDirection() const;

        /**
         * @brief Set the viewport dimensions
         * @param x The x-coordinate of the viewport
         * @param y The y-coordinate of the viewport
         * @param width The width of the viewport
         * @param height The height of the viewport
         */
        void setViewport(int x, int y, int width, int height);

        /**
         * @brief Set the rendering resolution
         * @param width The width of the render target
         * @param height The height of the render target
         */
        void setRenderSize(int width, int height);

        /**
         * @brief Get the current viewport
         * @return The viewport as (x, y, width, height)
         */
        const glm::ivec4& getViewport() const;

        /**
         * @brief Get the current render size
         * @return The render size as (width, height)
         */
        const glm::ivec2& getRenderSize() const;

        /**
         * @brief Get the view frustum for culling
         * @return The current view frustum
         */
        const Utility::Frustum& getFrustum() const;

        /**
         * @brief Update the view frustum based on current matrices
         */
        void updateFrustum();

        /**
         * @brief Bind common uniforms to a shader
         * @param shader The shader to bind uniforms to
         */
        void bindShaderUniforms(std::shared_ptr<Shader> shader);

        /**
         * @brief Transform a point from world space to view space
         * @param worldPos The point in world space
         * @return The point in view space
         */
        glm::vec3 worldToViewSpace(const glm::vec3& worldPos) const;

        /**
         * @brief Transform a point from world space to clip space
         * @param worldPos The point in world space
         * @return The point in clip space
         */
        glm::vec4 worldToClipSpace(const glm::vec3& worldPos) const;

    private:
        // Matrix data
        glm::mat4 m_viewMatrix;
        glm::mat4 m_projMatrix;
        glm::mat4 m_viewProjMatrix;

        // Camera information
        glm::vec3 m_cameraPosition;
        glm::vec3 m_cameraDirection;

        // Viewport imformation
        glm::ivec4 m_viewport;  // (x, y, width, height)
        glm::ivec2 m_renderSize; // (width, height)

        // Frustum for culling
        Utility::Frustum m_frustum;

        // Dirty flags for efficient updates
        bool m_matricesDirty;
        bool m_frustumDirty;
    };

} // namespace PixelCraft::Rendering