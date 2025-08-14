// -------------------------------------------------------------------------
// RenderSystem.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Subsystem.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/ClusteredLightCuller.h"
#include "Rendering/Cameras/Camera.h"
#include "Rendering/Cameras/OrthographicCamera.h"
#include "Utility/AABB.h"
#include "Utility/SpatialPartitioning.h"
#include "Utility/Quadtree.h"
#include "glm/glm.hpp"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

// In namespace PixelCraft
namespace PixelCraft::Rendering
{

    // Forward declarations
    class LightManager;
    class PaletteManager;
    class ShaderManager;
    class Shader;
    class Texture;
    class Material;
    class Mesh;
    struct Renderable;

    /**
     * @brief Represents an object that can be rendered
     */
    struct RenderableObject
    {
        std::shared_ptr<Mesh> mesh;            ///< The mesh to render
        std::shared_ptr<Material> material;    ///< The material to use for rendering
        glm::mat4 transform = glm::mat4(1.0f); ///< The transformation matrix
        Utility::AABB bounds;                  ///< Bounds for culling
        uint32_t layer = 1;                    ///< Render layer
        bool castShadows = true;               ///< Whether the object casts shadows
        bool receiveShadows = true;            ///< Whether the object receives shadows
    };

    /**
     * @brief Core rendering subsystem with forward+ pipeline management
     *
     * The RenderSystem manages the forward+ rendering pipeline, spatial partitioning,
     * and render queues. It's specialized for isometric 3D games with clustered lighting.
     * All actual rendering is delegated to the pipeline stages while this system handles
     * high-level coordination and resource management.
     */
    class RenderSystem : public Core::Subsystem
    {
    public:
        /**
         * @brief Constructor
         */
        RenderSystem();

        /**
         * @brief Destructor
         */
        virtual ~RenderSystem();

        /**
         * @brief Initialize the rendering subsystem
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the rendering state
         * @param deltaTime Time elapsed since the last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Render the scene via the active pipeline
         */
        void render() override;

        /**
         * @brief Clean up resources and shutdown the rendering subsystem
         */
        void shutdown() override;

        /**
         * @brief Get the name of the subsystem
         * @return The subsystem name
         */
        std::string getName() const override
        {
            return "RenderSystem";
        }

        /**
         * @brief Get the dependencies required by this subsystem
         * @return Vector of dependency subsystem names
         */
        std::vector<std::string> getDependencies() const override;

        /**
         * @brief Getter to check if render system is initialized
         */
        bool isInitialized() const
        {
            return m_initialized;
        }

        /**
         * @brief Submit an object to the render queue
         * @param renderable The object to render
         */
        void submitRenderable(const RenderableObject& renderable);

        /**
         * @brief Register a renderable object with spatial partitioning
         * @param renderable The renderable to register
         * @return ID assigned to the renderable
         */
        uint64_t registerRenderable(std::shared_ptr<Renderable> renderable);

        /**
         * @brief Unregister a renderable object
         * @param id The ID of the renderable to unregister
         * @return True if the renderable was found and unregistered
         */
        bool unregisterRenderable(uint64_t id);

        /**
         * @brief Update a renderable's position in spatial partitioning
         * @param id The ID of the renderable to update
         * @return True if the update was successful
         */
        bool updateRenderable(uint64_t id);

        /**
         * @brief Clear all render queues
         */
        void clearRenderQueue();

        /**
         * @brief Set the main camera for rendering
         * @param camera The camera to use for rendering
         */
        void setMainCamera(const std::shared_ptr<Rendering::Cameras::Camera> camera);

        /**
         * @brief Get the main camera for rendering
         * @return The camera used for rendering
         */
        std::shared_ptr<Rendering::Cameras::Camera> getMainCamera() const;

        /**
         * @brief Get the current render pipeline
         * @return The active render pipeline
         */
        std::shared_ptr<RenderPipeline> getRenderPipeline() const;

        /**
         * @brief Set the viewport dimensions
         * @param x The x coordinate of the viewport
         * @param y The y coordinate of the viewport
         * @param width The width of the viewport
         * @param height The height of the viewport
         */
        void setViewport(int x, int y, int width, int height);

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
         * @brief Toggle for enabling/disabling debugging at runtime
         * @param enabled Whether to enable/disable debugging at runtime
         */
        void toggleDebugging(bool enabled);

        /**
         * @brief Get a shader by delegating to ShaderManager
         * @param name The name of the shader
         * @return The shader or nullptr if not found
         */
        std::shared_ptr<Shader> getShader(const std::string& name);

        /**
         * @brief Create a new material
         * @param name The name of the material
         * @return The created material
         */
        std::shared_ptr<Material> createMaterial(const std::string& name);

        /**
         * @brief Create a new mesh
         * @param name The name of the mesh
         * @return The created mesh
         */
        std::shared_ptr<Mesh> createMesh(const std::string& name);

        /**
         * @brief Get the light manager
         * @return Reference to the light manager
         */
        LightManager& getLightManager();

        /**
         * @brief Get the palette manager
         * @return Reference to the palette manager
         */
        PaletteManager& getPaletteManager();

        /**
         * @brief Get the clustered light culler
         * @return Reference to the clustered light culler
         */
        ClusteredLightCuller& getClusteredLightCuller();

        /**
         * @brief Draw a debug line
         * @param start The start point of the line
         * @param end The end point of the line
         * @param color The color of the line
         */
        void drawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);

        /**
         * @brief Draw a debug box
         * @param box The box to draw
         * @param color The color of the box
         */
        void drawBox(const Utility::AABB& box, const glm::vec3& color);

        /**
         * @brief Draw a debug sphere
         * @param center The center of the sphere
         * @param radius The radius of the sphere
         * @param color The color of the sphere
         */
        void drawSphere(const glm::vec3& center, float radius, const glm::vec3& color);

        /**
         * @brief Get the number of draw calls in the last frame
         * @return The draw call count
         */
        int getDrawCallCount() const;

        /**
         * @brief Get the number of triangles rendered in the last frame
         * @return The triangle count
         */
        int getTriangleCount() const;

        /**
         * @brief Get the number of visible objects in the last frame
         * @return The visible object count
         */
        int getVisibleObjectCount() const;

        /**
         * @brief Get statistics about the rendering
         * @return String containing rendering statistics
         */
        std::string getRenderStats() const;

        /**
         * @brief Get the current render context
         * @return Reference to the render context
         */
        const RenderContext& getRenderContext() const;

        /**
         * @brief Get the opaque render queue (for pipeline stages)
         * @return Const reference to the opaque queue
         */
        const std::vector<RenderableObject>& getOpaqueQueue() const;

        /**
         * @brief Get the transparent render queue (for pipeline stages)
         * @return Const reference to the transparent queue
         */
        const std::vector<RenderableObject>& getTransparentQueue() const;

        /**
         * @brief Get the UI render queue (for pipeline stages)
         * @return Const reference to the UI queue
         */
        const std::vector<RenderableObject>& getUIQueue() const;

    private:
        /**
         * @brief Sort render queues by material and depth
         */
        void sortRenderQueues();

        /**
         * @brief Perform frustum culling on objects using spatial partitioning
         */
        void cullObjects();

        /**
         * @brief Update render statistics
         */
        void updateRenderStats();

        /**
         * @brief Convert a Renderable to a RenderableObject for submission
         * @param renderable The Renderable to convert
         * @return A RenderableObject suitable for rendering
         */
        RenderableObject convertToRenderableObject(const std::shared_ptr<Renderable>& renderable);

        /**
         * @brief Set up the isometric camera
         */
        void setupIsometricCamera();

        /**
         * @brief Get visible renderables using spatial partitioning
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
         * @brief Setup debugging for the render pipeline
         */
        void setupPipelineDebugging();

        /**
         * @brief Pipeline debugging execution
         */
        void diagnosePipelineIssues();

        /**
         * @brief Quick pipeline debugging setup for immediate use
         */
        void quickDebugSetup();

        /**
         * @brief Custom stage configuration for pipeline debugging
         */
        void configureAdvancedDebugging();

        /**
         * @brief Collect rendering statistics
         */
        void collectRenderStats();

        /**
         * @brief Configure forward plus pipeline stages
         */
        void configurePipelineStages();

        // Core rendering state
        RenderContext m_renderContext;
        std::shared_ptr<Rendering::Cameras::Camera> m_mainCamera;

        // Rendering pipeline
        std::shared_ptr<RenderPipeline> m_forwardPlusPipeline;

        // Forward+ rendering components
        std::unique_ptr<ClusteredLightCuller> m_clusteredLightCuller;

        // Spatial partitioning for efficient culling
        std::shared_ptr<Utility::Quadtree> m_spatialTree;
        std::unordered_map<uint64_t, std::shared_ptr<Renderable>> m_registeredRenderables;

        // Rendering queues
        std::vector<RenderableObject> m_opaqueQueue;
        std::vector<RenderableObject> m_transparentQueue;
        std::vector<RenderableObject> m_uiQueue;

        // Viewport information
        int m_viewportX;
        int m_viewportY;
        int m_viewportWidth;
        int m_viewportHeight;

        // Isometric-specific settings
        float m_isometricAngle;
        bool m_pixelGridAlignmentEnabled;
        bool m_depthSortingEnabled;
        bool m_debugVisualizationEnabled;

        // Subsystems
        std::shared_ptr<LightManager> m_lightManager;
        std::shared_ptr<PaletteManager> m_paletteManager;

        // Stats
        int m_drawCallCount;
        int m_triangleCount;
        int m_visibleObjectCount;

        // Initialization flag
        bool m_initialized;
    };

} // namespace PixelCraft::Rendering
