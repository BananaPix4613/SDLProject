// -------------------------------------------------------------------------
// GeometryStage.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/RenderStage.h"

#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace PixelCraft::Rendering
{
    class Material;
    class Mesh;
    class Shader;
    class RenderContext;
    class RenderSystem;
    struct RenderableObject;
    class ForwardPlusShaderManager;
}

namespace PixelCraft::ECS
{
    class TransformComponent;
    class MeshRendererComponent;
    class Registry;
    using EntityID = uint32_t;
}

namespace PixelCraft::Rendering::Stages
{

    // Helper for hashing pairs in unordered_map
    struct PairHash
    {
        template <class T1, class T2>
        std::size_t operator () (const std::pair<T1, T2>& p) const
        {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ (h2 << 1);
        }
    };

    // Render data structure for efficient sorting
    struct RenderItem
    {
        ECS::EntityID entity;
        ECS::TransformComponent* transform;        // Pointer from View
        ECS::MeshRendererComponent* meshRenderer;  // Pointer from View
        glm::vec3 position;
        float distanceFromCamera;
        std::shared_ptr<Material> material;
        std::shared_ptr<Mesh> mesh;
    };

    /**
     * @brief Forward+ rendering stage with clustered lighting support
     * 
     * The geometry stage handles forward+ shading with clustered light culling,
     * performing complete lighting calculations in a single pass. It supports
     * both opaque and transparent objects with efficient cluster-based light
     * accumulation, eliminating the need for separate lighting stages.
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
        virtual ~GeometryStage();

        /**
         * @brief Initialize the forward+ geometry stage
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Clean up resources
         */
        void shutdown() override;

        /**
         * @brief Execute the forward+ rendering pass with clustered lighting
         * @param context Current render context
         */
        void execute(const std::shared_ptr<RenderContext> context) override;

        /**
         * @brief Get the stage type name
         * @return Type name string
         */
        std::string getTypeName() const override
        {
            return "GeometryStage";
        }

        /**
         * @brief Get the number of draw calls issued by this stage in the current frame
         * @return Draw call count for current frame
         */
        uint32_t getFrameDrawCalls() const override
        {
            return m_frameDrawCalls;
        }

        /**
         * @brief Get the number of triangles rendered by this stage in the current frame
         * @return Triangle count for current frame
         */
        uint32_t getFrameTriangles() const override
        {
            return m_frameTriangles;
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
         * @brief Set the entity registry to render from
         * @param registry Entity registry
         */
        void setRegistry(std::shared_ptr<ECS::Registry> registry);

        /**
         * @brief Set the render system to pull queues from
         * @param renderSystem Render system
         */
        void setRenderSystem(RenderSystem* renderSystem);

        /**
         * @brief Set whether to render opaque objects
         * @param opaque True to render only opaque objects
         */
        void setRenderOpaque(bool opaque);

        /**
         * @brief Set whether to render  transparent objects
         * @param transparent True to render only transparent objects
         */
        void setRenderTransparent(bool transparent);

        /**
         * @brief Enable or disable depth testing
         * @param enabled Whether depth testing is enabled
         */
        void setDepthTest(bool enabled);

        /**
         * @brief Enable or disable depth writing
         * @param enabled Whether depth writing is enabled
         */
        void setDepthWrite(bool enabled);

        /**
         * @brief Enable or disable instanced rendering
         * @param enabled Whether instanced rendering is enabled
         */
        void setUseInstancing(bool enabled);

        /**
         * @brief Enable or disable material sorting
         * @param enabled Whether to sort objects by material
         */
        void setSortObjects(bool enabled);

    protected:
        /**
         * @brief Render geometry objects with forward+ shading
         * @param context Render context
         */
        void renderGeometry(const std::shared_ptr<RenderContext> context);

        /**
         * @brief Render ECS entities with clustered lighting
         * @param context Render context
         * @param lastMaterial last material used
         * @param lastMesh last mesh used
         */
        void renderECSEntities(const std::shared_ptr<RenderContext> context,
                               std::shared_ptr<Material>& lastMaterial,
                               std::shared_ptr<Mesh>& lastMesh);

        /**
         * @brief Render system geometry with clustered lighting
         * @param context Render context
         * @param lastMaterial last material used
         * @param lastMesh last mesh used
         */
        void renderSystemGeometry(const std::shared_ptr<RenderContext> context,
                                  std::shared_ptr<Material>& lastMaterial,
                                  std::shared_ptr<Mesh>& lastMesh);

        /**
         * @brief Render single item with forward+ shading
         * @param context Render context
         * @param lastMaterial last material used
         * @param lastMesh last mesh used
         */
        void renderItem(const RenderItem& item, 
                        const std::shared_ptr<RenderContext> context,
                        std::shared_ptr<Material>& lastMaterial,
                        std::shared_ptr<Mesh>& lastMesh);

        /**
         * @brief Render items instanced with clustered lighting
         * @param items List of items to render
         * @param context Render context
         */
        void renderItemsInstanced(const std::vector<RenderItem>& items, 
                                  const std::shared_ptr<RenderContext> context);

        /**
         * @brief Render a single render item with forward+ shading
         * @param item The item to render
         * @param lastMaterial last material used
         * @param lastMesh last mesh used
         * @param context Render context
         */
        void renderObject(const RenderableObject& item,
                          std::shared_ptr<Material>& lastMaterial,
                          std::shared_ptr<Mesh>& lastMesh,
                          const std::shared_ptr<RenderContext> context);

        /**
         * @brief Setup render states for forward+ geometry rendering
         */
        void setupRenderState();

        /**
         * @brief Restore render states after geometry rendering
         */
        void restoreRenderState();

        /**
         * @brief Sort items for efficient rendering
         * @param items Vector of render items to sort
         */
        void sortRenderItems(std::vector<RenderItem>& items);

        /**
         * @brief Track a draw call and calculate triangles from mesh
         * @param mesh The mesh being drawn
         * @param instanceCount Number of instances (default 1)
         */
        void trackDrawCall(const std::shared_ptr<Mesh>& mesh, uint32_t instanceCount = 1);

    private:
        /** Forward+ shader manager for cluster integration */
        std::shared_ptr<ForwardPlusShaderManager> m_forwardPlusShaderManager;

        /** Entity registry */
        std::shared_ptr<ECS::Registry> m_registry;

        /** Render system */
        RenderSystem* m_renderSystem;

        /** Whether to render opaque objects */
        bool m_renderOpaque = false;

        /** Whether to render transparent objects */
        bool m_renderTransparent = false;

        /** Whether depth testing is enabled */
        bool m_depthTest = true;

        /** Whether depth writing is enabled */
        bool m_depthWrite = true;

        /** Whether to use instanced rendering */
        bool m_useInstancing = true;

        /** Whether to sort objects by material */
        bool m_sortObjects = true;

        /** Frame statistics - draw calls issued this frame */
        uint32_t m_frameDrawCalls = 0;

        /** Frame statistics - triangles rendered this frame */
        uint32_t m_frameTriangles = 0;
    };

} // namespace PixelCraft::Rendering::Stages
