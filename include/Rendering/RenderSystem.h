// -------------------------------------------------------------------------
// RenderSystem.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

#include "Core/Subsystem.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include "Rendering/Material.h"
#include "Rendering/Mesh.h"
#include "Utility/AABB.h"

namespace PixelCraft::Rendering
{

    // Forward declarations
    class LightManager;
    class PaletteManager;
    class RenderContext;
    class Camera;

    /**
     * @brief Represents an object that can be rendered
     */
    struct RenderableObject
    {
        std::shared_ptr<Mesh> mesh;               ///< The mesh to render
        std::shared_ptr<Material> material;       ///< The material to use for rendering
        glm::mat4 transform;                      ///< The transformation matrix
        Utility::AABB bounds;                     ///< Bounds for culling
        uint32_t layer;                           ///< Render layer
        bool castShadows;                         ///< Whether the object casts shadows
        bool receiveShadows;                      ///< Whether the object receives shadows
    };

    /**
     * @brief Rendering layers for categorizing objects
     */
    enum class RenderLayer
    {
        Default = 1,       ///< Default rendering layer
        Transparent = 2,   ///< Transparent objects
        UI = 4,            ///< User interface elements
        Overlay = 8,       ///< Overlay elements
        Debug = 16,        ///< Debug visualization
        // Add more as needed
    };

    /**
     * @brief Core rendering subsystem with pipeline management
     *
     * The RenderSystem is responsible for managing rendering pipelines,
     * handling rendering resources, and coordinating all rendering operations.
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
        ~RenderSystem() override;

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
         * @brief Render the scene
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
         * @brief Create a new render pipeline
         * @param name The name of the pipeline
         * @param pipeline The pipeline to register
         */
        void createRenderPipeline(const std::string& name, std::shared_ptr<RenderPipeline> pipeline);

        /**
         * @brief Get a render pipeline by name
         * @param name The name of the pipeline to retrieve
         * @return The requested pipeline or nullptr if not found
         */
        std::shared_ptr<RenderPipeline> getRenderPipeline(const std::string& name);

        /**
         * @brief Set the active render pipeline
         * @param name The name of the pipeline to set as active
         */
        void setActivePipeline(const std::string& name);

        /**
         * @brief Get the name of the currently active pipeline
         * @return The name of the active pipeline
         */
        const std::string& getActivePipeline() const;

        /**
         * @brief Submit an object to the render queue
         * @param renderable The object to render
         */
        void submitRenderable(const RenderableObject& renderable);

        /**
         * @brief Clear all render queues
         */
        void clearRenderQueue();

        /**
         * @brief Set the main camera for rendering
         * @param camera The camera to use for rendering
         */
        void setMainCamera(const std::shared_ptr<Camera::Camera> camera);

        /**
         * @brief Get the main camera for rendering
         * @return The camera used for rendering
         */
        std::shared_ptr<Camera::Camera> getMainCamera() const;

        /**
         * @brief Set the viewport dimensions
         * @param x The x coordinate of the viewport
         * @param y The y coordinate of the viewport
         * @param width The width of the viewport
         * @param height The height of the viewport
         */
        void setViewport(int x, int y, int width, int height);

        /**
         * @brief Create a new shader
         * @param name The name of the shader
         * @return The created shader
         */
        std::shared_ptr<Shader> createShader(const std::string& name);

        /**
         * @brief Create a new texture
         * @param name The name of the texture
         * @return The created texture
         */
        std::shared_ptr<Texture> createTexture(const std::string& name);

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
         * @brief Take a screenshot
         * @param filepath The path to save the screenshot to
         */
        void takeScreenshot(const std::string& filepath);

        /**
         * @brief Render the scene to a texture
         * @param texture The texture to render to
         */
        void renderToTexture(std::shared_ptr<Texture> texture);

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

    private:
        /**
         * @brief Sort render queues by material and depth
         */
        void sortRenderQueues();

        /**
         * @brief Submit a queue of renderables to the renderer
         * @param queue The queue to render
         */
        void submitQueueToRenderer(const std::vector<RenderableObject>& queue);

        /**
         * @brief Perform frustum culling on objects
         */
        void cullObjects();

        /**
         * @brief Update render statistics
         */
        void updateRenderStats();

        // Core rendering state
        RenderContext m_renderContext;
        std::unordered_map<std::string, std::shared_ptr<RenderPipeline>> m_pipelines;
        std::string m_activePipeline;

        // Rendering queues
        std::vector<RenderableObject> m_opaqueQueue;
        std::vector<RenderableObject> m_transparentQueue;
        std::vector<RenderableObject> m_uiQueue;

        // Viewport information
        int m_viewportX;
        int m_viewportY;
        int m_viewportWidth;
        int m_viewportHeight;

        // Subsystems
        std::unique_ptr<LightManager> m_lightManager;
        std::unique_ptr<PaletteManager> m_paletteManager;

        // Debug rendering
        std::unique_ptr<Utility::LineBatchRenderer> m_debugRenderer;

        // Stats
        int m_drawCallCount;
        int m_triangleCount;
        int m_visibleObjectCount;

        // Initialization flag
        bool m_initialized = false;
    };

} // namespace PixelCraft::Rendering