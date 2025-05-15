// -------------------------------------------------------------------------
// IsometricRenderSystem.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Subsystem.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/Camera/OrthographicCamera.h"
#include "Utility/SpatialPartitioning.h"
#include "Utility/Octree.h"
#include "Utility/Quadtree.h"
#include "Utility/Frustum.h"

#include <memory>
#include <vector>
#include <unordered_map>

namespace PixelCraft::Rendering
{

    /**
     * @brief Specialized renderer for isometric 3D games with optimized spatial partitioning
     */
    class IsometricRenderSystem : public Core::Subsystem
    {
    public:
        /**
         * @brief Constructor
         */
        IsometricRenderSystem();

        /**
         * @brief Destructor
         */
        virtual ~IsometricRenderSystem();

        /**
         * @brief Initialize the isometric renderer
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the rendering system
         * @param deltaTime Time since last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Render the scene
         */
        void render() override;

        /**
         * @brief Shut down the rendering system
         */
        void shutdown() override;

        /**
         * @brief Get the name of the subsystem
         * @return The subsystem name
         */
        std::string getName() const override
        {
            return "IsometricRenderSystem";
        }

        /**
         * @brief Get the subsystem dependencies
         * @return Vector of dependency subsystem names
         */
        std::vector<std::string> getDependencies() const override;

        /**
         * @brief Register a renderable object with the system
         * @param renderable The renderable to register
         * @return ID assigned to the renderable
         */
        uint64_t registerRenderable(std::shared_ptr<class Renderable> renderable);

        /**
         * @brief Unregister a renderable object
         * @param id The ID of the renderable to unregister
         * @return True if the renderable was found and unregistered
         */
        bool unregisterRenderable(uint64_t id);

        /**
         * @brief Update a renderable's position
         * @param id The ID of the renderable to update
         * @return True if the update was successful
         */
        bool updateRenderable(uint64_t id);

        /**
         * @brief Set the main camera for rendering
         * @param camera The camera to use
         */
        void setMainCamera(std::shared_ptr<OrthographicCamera> camera);

        /**
         * @brief Get the main camera
         * @return The main camera
         */
        std::shared_ptr<OrthographicCamera> getMainCamera() const;

        /**
         * @brief Set the isometric view angle
         * @param angle The isometric angle in degrees (default is 45)
         */
        void setIsometricAngle(float angle);

        /**
         * @brief Get the isometric view angle
         * @return The isometric angle in degrees
         */
        float getIsometricAngle() const;

        /**
         * @brief Set the pixel grid alignment
         * @param enabled Whether pixel alignment is enabled
         */
        void setPixelGridAlignment(bool enabled);

        /**
         * @brief Check if pixel grid alignment is enabled
         * @return True if pixel grid alignment is enabled
         */
        bool isPixelGridAlignmentEnabled() const;

        /**
         * @brief Set whether to use depth sorting for transparent objects
         * @param enabled Whether depth sorting is enabled
         */
        void setDepthSortingEnabled(bool enabled);

        /**
         * @brief Check if depth sorting is enabled
         * @return True if depth sorting is enabled
         */
        bool isDepthSortingEnabled() const;

        /**
         * @brief Set whether to show debug visualization
         * @param enabled Whether debug visualization is enabled
         */
        void setDebugVisualizationEnabled(bool enabled);

        /**
         * @brief Check if debug visualization is enabled
         * @return True if debug visualization is enabled
         */
        bool isDebugVisualizationEnabled() const;

        /**
         * @brief Get statistics about the rendering
         * @return String containing rendering statistics
         */
        std::string getRenderStats() const;

    private:
        std::shared_ptr<RenderSystem> m_renderSystem;                   ///< Main render system
        std::shared_ptr<RenderPipeline> m_isometricPipeline;           ///< Isometric rendering pipeline
        std::shared_ptr<OrthographicCamera> m_camera;                  ///< Main orthographic camera
        std::shared_ptr<Utility::Quadtree> m_spatialTree;              ///< Spatial partitioning for efficient culling
        std::unordered_map<uint64_t, std::shared_ptr<Renderable>> m_renderables; ///< Registered renderables

        float m_isometricAngle;                                        ///< Isometric view angle
        bool m_pixelGridAlignmentEnabled;                             ///< Whether pixel grid alignment is enabled
        bool m_depthSortingEnabled;                                   ///< Whether depth sorting is enabled
        bool m_debugVisualizationEnabled;                             ///< Whether debug visualization is enabled
        bool m_initialized;                                           ///< Whether the system is initialized

        /**
         * @brief Create the isometric rendering pipeline
         * @return The created pipeline
         */
        std::shared_ptr<RenderPipeline> createIsometricPipeline();

        /**
         * @brief Set up the isometric camera
         */
        void setupIsometricCamera();

        /**
         * @brief Perform frustum culling to get visible objects
         * @return Vector of visible renderables
         */
        std::vector<std::shared_ptr<Renderable>> getVisibleRenderables();

        /**
         * @brief Sort transparent objects by depth
         * @param renderables The renderables to sort
         */
        void sortByDepth(std::vector<std::shared_ptr<Renderable>>& renderables);

        /**
         * @brief Apply pixel grid alignment to renderable transforms
         * @param renderable The renderable to align
         */
        void applyPixelGridAlignment(std::shared_ptr<Renderable> renderable);

        /**
         * @brief Render debug visualization
         */
        void renderDebugVisualization();

        /**
         * @brief Collect rendering statistics
         */
        void collectRenderStats();

        // Rendering statistics
        struct
        {
            int visibleObjects;
            int drawCalls;
            int triangles;
            float cullingTime;
            float renderTime;
        } m_stats;
    };

} // namespace PixelCraft::Rendering