// -------------------------------------------------------------------------
// Mesh.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Resource.h"
#include "Utility/AABB.h"

#include <memory>
#include <string>
#include <vector>

namespace PixelCraft::Rendering
{

    /**
     * @brief Defines the primitive type for rendering
     */
    enum class PrimitiveType
    {
        Points,
        Lines,
        LineStrip,
        LineLoop,
        Triangles,
        TriangleStrip,
        TriangleFan
    };

    /**
     * @brief Defines the semantic meaning of vertex attributes
     */
    enum class VertexAttributeSemantic
    {
        Position,
        Normal,
        Tangent,
        TexCoord0,
        TexCoord1,
        Color,
        BoneIndices,
        BoneWeights,
        Custom
    };

    /**
     * @brief Describes a single vertex attribute
     */
    struct VertexAttribute
    {
        VertexAttributeSemantic semantic;
        int location;
        int size;      // Components per vertex (1-4)
        unsigned int type;   // GL_FLOAT, GL_INT, etc.
        bool normalized;
        int stride;
        size_t offset;
        std::string name; // For custom attributes
    };

    /**
     * @brief 3D geometry data with efficient GPU storage
     *
     * Manages vertex and index buffer objects with support for different vertex
     * attribute layouts, dynamic updates, and automatic geometry calculations.
     */
    class Mesh : public Core::Resource
    {
    public:
        /**
         * @brief Default constructor
         */
        Mesh();

        /**
         * @brief Constructor
         * @param name Resource name for identification
         */
        Mesh(const std::string& name);

        /**
         * @brief Destructor
         */
        virtual ~Mesh();

        /**
         * @brief Load the mesh resource
         * @return True if loading was successful
         */
        bool load() override;

        /**
         * @brief Unload the mesh resource and free GPU resources
         */
        void unload() override;

        /**
         * @brief Called when the resource needs to be reloaded
         * @return True if reload was successful
         */
        bool onReload() override;

        /**
         * @brief Create a mesh from vertex and index data
         * @param vertexData Pointer to vertex data
         * @param vertexCount Number of vertices
         * @param vertexStride Stride between vertices in bytes
         * @param attributes Vertex attribute layout
         * @param indexData Pointer to index data
         * @param indexCount Number of indices
         * @return True if creation was successful
         */
        bool createFromData(const void* vertexData,
                            size_t vertexCount,
                            size_t vertexStride,
                            const std::vector<VertexAttribute>& attributes,
                            const void* indexData,
                            size_t indexCount);

        /**
         * @brief Create a box mesh with the specified dimensions
         * @param width Width of the box
         * @param height Height of the box
         * @param depth Depth of the box
         * @param createUVs Whether to create texture coordinates
         * @return True if the box mesh was created successfully
         */
        bool createBox(float width = 1.0f, float height = 1.0f,
                       float depth = 1.0f, bool createUVs = true);

        /**
         * @brief Create a sphere mesh with the specified parameters
         * @param radius Radius of the sphere
         * @param rings Number of horizontal subdivisions
         * @param sectors Number of vertical subdivisions
         * @param createUVs Whether to create texture coordinates
         * @return True if the sphere was created successfully
         */
        bool createSphere(float radius = 1.0f, uint32_t rings = 16,
                          uint32_t sectors = 32, bool createUVs = true);

        /**
         * @brief Create a cylinder mesh with the specified parameters
         * @param radius Radius of the cylinder
         * @param height Height of the cylinder
         * @param sectors Number of sides around the cylinder
         * @param capped Whether to create top and bottom caps
         * @param createUVs Whether to create texture coordinates
         * @return True if the cylinder was successfully created
         */
        bool createCylinder(float radius = 1.0f, float height = 2.0f,
                            uint32_t sectors = 24, bool capped = true,
                            bool createUVs = true);

        /**
         * @brief Create a cone mesh with the specified parameters
         * @param radius Radius of the cone base
         * @param height Height of the cone
         * @param sectors Number of sides around the cone
         * @param capped Whether to create bottom cap
         * @param createUVs Whether to create texture coordinates
         * @return True if the cone was successfully created
         */
        bool createCone(float radius = 1.0f, float height = 2.0f,
                        uint32_t sectors = 24, bool capped = true,
                        bool createUVs = true);

        /**
         * @brief Bind the mesh for rendering
         */
        void bind();

        /**
         * @brief Unbind the mesh after rendering
         */
        void unbind();

        /**
         * @brief Draw the mesh
         */
        void draw();

        /**
         * @brief Draw the mesh multiple times
         * @param instanceCount Number of instances to draw
         */
        void drawInstanced(uint32_t instanceCount);

        /**
         * @brief Update a portion of the vertex data
         *
         * @param data Pointer to new vertex data
         * @param size Size of data in bytes
         * @param offset Byte offset into the vertex buffer
         */
        void updateVertexData(const void* data, uint32_t size, uint32_t offset = 0);

        /**
         * @brief Update a portion of the index data
         *
         * @param indices Pointer to new index data
         * @param count Number of indices to update
         * @param offset Index offset into the index buffer
         */
        void updateIndexData(const uint32_t* indices, uint32_t count, uint32_t offset = 0);

        /**
         * @brief Calculate vertex normals based on triangles
         *
         * Requires Position and Normal vertex attributes.
         */
        void calculateNormals();

        /**
         * @brief Calculate tangent vectors for normal mapping
         *
         * Requires Position, Normal, TexCoord0, and Tangent vertex attributes.
         */
        void calculateTangents();

        /**
         * @brief Calculate axis-aligned bounding box
         *
         * Requires Position vertex attribute.
         */
        void calculateBounds();

        /**
         * @brief Get the number of vertices
         * @return Vertex count
         */
        uint32_t getVertexCount() const;

        /**
         * @brief Get the number of indices
         * @return Index count (0 for non-indexed geometry)
         */
        uint32_t getIndexCount() const;

        /**
         * @brief Get the primitive type
         * @return Primitive type used for rendering
         */
        PrimitiveType getPrimitiveType() const;

        /**
         * @brief Get the vertex attribute layout
         * @return Vertex attributes
         */
        const std::vector<VertexAttribute>& getAttributes() const;

        /**
         * @brief Get the bounding box
         * @return Axis-aligned bounding box
         */
        const Utility::AABB& getBounds() const;

    private:
        uint32_t m_vao;     // Vertex Array Object
        uint32_t m_vbo;     // Vertex Buffer Object
        uint32_t m_ebo;     // Element Buffer Object (indices)

        uint32_t m_vertexCount;
        uint32_t m_indexCount;
        PrimitiveType m_primitiveType;
        std::vector<VertexAttribute> m_attributes;
        Utility::AABB m_bounds;
        bool m_dynamic;

        /**
         * @brief Set up vertex attribute pointers
         */
        void setupVertexAttributes();

        /**
         * @brief Convert primitive type to GL enum
         * @return OpenGL primitive type enum
         */
        unsigned int getPrimitiveTypeGL() const;
    };

} // namespace PixelCraft::Rendering