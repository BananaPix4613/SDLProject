// -------------------------------------------------------------------------
// Renderable.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/Mesh.h"
#include "Rendering/Material.h"
#include "Utility/AABB.h"

#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace PixelCraft::Rendering
{

    /**
     * @brief Enumeration of rendering layers for categorizing objects
     */
    enum class RenderLayer
    {
        Default = 1,       ///< Default rendering layer
        Transparent = 2,   ///< Transparent objects
        UI = 4,            ///< User interface elements
        Overlay = 8,       ///< Overlay elements
        Debug = 16,        ///< Debug visualization
        Voxel = 32         ///< Voxel terrain
    };

    /**
     * @brief Represents an object that can be rendered
     */
    struct Renderable
    {
        std::shared_ptr<Mesh> mesh;               ///< The mesh to render
        std::shared_ptr<Material> material;       ///< The material to use for rendering
        glm::mat4 transform = glm::mat4(1.0f);    ///< The transformation matrix
        Utility::AABB bounds;                     ///< Bounds for culling
        uint32_t layer = static_cast<uint32_t>(RenderLayer::Default); ///< Render layer
        bool castShadows = true;                  ///< Whether the object casts shadows
        bool receiveShadows = true;               ///< Whether the object receives shadows

        /**
         * @brief Default constructor
         */
        Renderable() = default;

        /**
         * @brief Constructor with mesh and material
         * @param _mesh The mesh to render
         * @param _material The material to use
         */
        Renderable(std::shared_ptr<Mesh> _mesh, std::shared_ptr<Material> _material)
            : mesh(_mesh)
            , material(_material)
        {
            // Initialize bounds from mesh if available
            if (mesh)
            {
                bounds = mesh->getBounds();
            }
        }

        /**
         * @brief Set the world position
         * @param position The world position
         */
        void setPosition(const glm::vec3& position)
        {
            transform[3] = glm::vec4(position, 1.0f);
        }

        /**
         * @brief Get the world position
         * @return The world position
         */
        glm::vec3 getPosition() const
        {
            return glm::vec3(transform[3]);
        }

        /**
         * @brief Set the transformation matrix
         * @param newTransform The new transformation matrix
         */
        void setTransform(const glm::mat4& newTransform)
        {
            transform = newTransform;
        }

        /**
         * @brief Set the render layer
         * @param newLayer The new render layer
         */
        void setLayer(RenderLayer newLayer)
        {
            layer = static_cast<uint32_t>(newLayer);
        }

        /**
         * @brief Update the bounding box from the mesh and transform
         */
        void updateBounds()
        {
            if (mesh)
            {
                bounds = mesh->getBounds();

                // Transform the bounds using the transform matrix
                bounds.transform(transform);
            }
        }
    };

} // namespace PixelCraft::Rendering