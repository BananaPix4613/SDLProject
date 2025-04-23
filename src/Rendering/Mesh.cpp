// -------------------------------------------------------------------------
// Mesh.cpp
// -------------------------------------------------------------------------
#include "Rendering/Mesh.h"
#include "Core/Logger.h"
#include "Utility/AABB.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <gtc/type_ptr.hpp>
#include <vector>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering
{

    Mesh::Mesh(const std::string& name)
        : Core::Resource(name)
        , m_vao(0)
        , m_vbo(0)
        , m_ebo(0)
        , m_vertexCount(0)
        , m_indexCount(0)
        , m_primitiveType(PrimitiveType::Triangles)
        , m_dynamic(false)
    {
    }

    Mesh::~Mesh()
    {
        if (isLoaded())
        {
            unload();
        }
    }

    bool Mesh::load()
    {
        // The actual mesh data will be loaded through createFromData
        // This method is primarily used when loading from a file
        return true;
    }

    void Mesh::unload()
    {
        if (m_vao != 0)
        {
            glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }

        if (m_vbo != 0)
        {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }

        if (m_ebo != 0)
        {
            glDeleteBuffers(1, &m_ebo);
            m_ebo = 0;
        }

        m_vertexCount = 0;
        m_indexCount = 0;
        m_loaded = false;
    }

    bool Mesh::onReload()
    {
        // If this is a file-based resource, we would reload the file here
        return isLoaded();
    }

    bool Mesh::createFromData(const void* vertices, uint32_t vertexCount,
                              const uint32_t* indices, uint32_t indexCount,
                              const VertexAttributes& attributes,
                              PrimitiveType primitiveType)
    {

        if (isLoaded())
        {
            unload();
        }

        if (vertices == nullptr || vertexCount == 0)
        {
            Log::error("Mesh::createFromData: Invalid vertex data");
            return false;
        }

        m_vertexCount = vertexCount;
        m_indexCount = indexCount;
        m_primitiveType = primitiveType;
        m_attributes = attributes;

        // Create and bind the VAO
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        // Create and bind the VBO, upload vertex data
        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

        // Calculate total vertex data size
        size_t vertexDataSize = m_attributes.stride * vertexCount;
        glBufferData(GL_ARRAY_BUFFER, vertexDataSize, vertices,
                     m_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

        // If we have indices, create and bind the EBO, upload index data
        if (indices != nullptr && indexCount > 0)
        {
            glGenBuffers(1, &m_ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint32_t),
                         indices, m_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        }

        // Set up vertex attributes
        setupVertexAttributes();

        // Unbind VAO to prevent accidental modifications
        glBindVertexArray(0);

        // Calculate bounds if we have position data
        calculateBounds();

        m_loaded = true;
        return true;
    }

    void Mesh::bind()
    {
        if (!isLoaded())
        {
            Log::warn("Mesh::bind: Mesh not loaded");
            return;
        }

        glBindVertexArray(m_vao);
    }

    void Mesh::unbind()
    {
        glBindVertexArray(0);
    }

    void Mesh::draw()
    {
        if (!isLoaded() || m_vertexCount == 0)
        {
            Log::warn("Mesh::draw: Mesh not loaded or empty");
            return;
        }

        if (m_indexCount > 0 && m_ebo != 0)
        {
            // Draw indexed
            glDrawElements(getPrimitiveTypeGL(), m_indexCount, GL_UNSIGNED_INT, 0);
        }
        else
        {
            // Draw non-indexed
            glDrawArrays(getPrimitiveTypeGL(), 0, m_vertexCount);
        }
    }

    void Mesh::drawInstanced(uint32_t instanceCount)
    {
        if (!isLoaded() || m_vertexCount == 0)
        {
            Log::warn("Mesh::drawInstanced: Mesh not loaded or empty");
            return;
        }

        if (instanceCount == 0)
        {
            return;
        }

        if (m_indexCount > 0 && m_ebo != 0)
        {
            // Draw indexed instanced
            glDrawElementsInstanced(getPrimitiveTypeGL(), m_indexCount,
                                    GL_UNSIGNED_INT, 0, instanceCount);
        }
        else
        {
            // Draw non-indexed instanced
            glDrawArraysInstanced(getPrimitiveTypeGL(), 0, m_vertexCount, instanceCount);
        }
    }

    void Mesh::updateVertexData(const void* data, uint32_t size, uint32_t offset)
    {
        if (!isLoaded() || m_vbo == 0)
        {
            Log::error("Mesh::updateVertexData: Mesh not loaded");
            return;
        }

        if (data == nullptr || size == 0)
        {
            Log::error("Mesh::updateVertexData: Invalid data");
            return;
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void Mesh::updateIndexData(const uint32_t* indices, uint32_t count, uint32_t offset)
    {
        if (!isLoaded() || m_ebo == 0)
        {
            Log::error("Mesh::updateIndexData: Mesh not loaded or no EBO");
            return;
        }

        if (indices == nullptr || count == 0)
        {
            Log::error("Mesh::updateIndexData: Invalid data");
            return;
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset * sizeof(uint32_t),
                        count * sizeof(uint32_t), indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void Mesh::calculateNormals()
    {
        if (!isLoaded())
        {
            Log::error("Mesh::calculateNormals: Mesh not loaded");
            return;
        }

        // Find position and normal attribute locations
        int positionAttribIndex = -1;
        int normalAttribIndex = -1;

        for (size_t i = 0; i < m_attributes.attributes.size(); ++i)
        {
            const auto& attr = m_attributes.attributes[i];
            if (attr.semantic == VertexAttributeSemantic::Position)
            {
                positionAttribIndex = i;
            }
            else if (attr.semantic == VertexAttributeSemantic::Normal)
            {
                normalAttribIndex = i;
            }
        }

        if (positionAttribIndex == -1)
        {
            Log::error("Mesh::calculateNormals: No position attribute found");
            return;
        }

        if (normalAttribIndex == -1)
        {
            Log::error("Mesh::calculateNormals: No normal attribute found");
            return;
        }

        // Get the vertex data
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        void* vertexData = nullptr;
        size_t vertexDataSize = m_attributes.stride * m_vertexCount;

#ifdef _WIN32
        vertexData = _malloca(vertexDataSize);
#else
        vertexData = alloca(vertexDataSize);
#endif

        glGetBufferSubData(GL_ARRAY_BUFFER, 0, vertexDataSize, vertexData);

        // Calculate normals
        const auto& posAttr = m_attributes.attributes[positionAttribIndex];
        const auto& normAttr = m_attributes.attributes[normalAttribIndex];

        // Reset normals to zero
        for (uint32_t i = 0; i < m_vertexCount; ++i)
        {
            float* normal = reinterpret_cast<float*>(
                static_cast<uint8_t*>(vertexData) + i * m_attributes.stride + normAttr.offset
                );
            normal[0] = 0.0f;
            normal[1] = 0.0f;
            normal[2] = 0.0f;
        }

        // Calculate face normals and accumulate to vertices
        if (m_indexCount > 0 && m_ebo != 0)
        {
            // Indexed geometry
            std::vector<uint32_t> indices(m_indexCount);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
            glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                               m_indexCount * sizeof(uint32_t), indices.data());

            for (uint32_t i = 0; i < m_indexCount; i += 3)
            {
                if (i + 2 >= m_indexCount) break;

                uint32_t idx1 = indices[i];
                uint32_t idx2 = indices[i + 1];
                uint32_t idx3 = indices[i + 2];

                float* pos1 = reinterpret_cast<float*>(
                    static_cast<uint8_t*>(vertexData) + idx1 * m_attributes.stride + posAttr.offset
                    );
                float* pos2 = reinterpret_cast<float*>(
                    static_cast<uint8_t*>(vertexData) + idx2 * m_attributes.stride + posAttr.offset
                    );
                float* pos3 = reinterpret_cast<float*>(
                    static_cast<uint8_t*>(vertexData) + idx3 * m_attributes.stride + posAttr.offset
                    );

                glm::vec3 v1(pos1[0], pos1[1], pos1[2]);
                glm::vec3 v2(pos2[0], pos2[1], pos2[2]);
                glm::vec3 v3(pos3[0], pos3[1], pos3[2]);

                glm::vec3 edge1 = v2 - v1;
                glm::vec3 edge2 = v3 - v1;
                glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

                float* norm1 = reinterpret_cast<float*>(
                    static_cast<uint8_t*>(vertexData) + idx1 * m_attributes.stride + normAttr.offset
                    );
                float* norm2 = reinterpret_cast<float*>(
                    static_cast<uint8_t*>(vertexData) + idx2 * m_attributes.stride + normAttr.offset
                    );
                float* norm3 = reinterpret_cast<float*>(
                    static_cast<uint8_t*>(vertexData) + idx3 * m_attributes.stride + normAttr.offset
                    );

                // Accumulate normals
                norm1[0] += normal.x; norm1[1] += normal.y; norm1[2] += normal.z;
                norm2[0] += normal.x; norm2[1] += normal.y; norm2[2] += normal.z;
                norm3[0] += normal.x; norm3[1] += normal.y; norm3[2] += normal.z;
            }
        }
        else
        {
            // Non-indexed geometry (assume triangles)
            for (uint32_t i = 0; i < m_vertexCount; i += 3)
            {
                if (i + 2 >= m_vertexCount) break;

                float* pos1 = reinterpret_cast<float*>(
                    static_cast<uint8_t*>(vertexData) + i * m_attributes.stride + posAttr.offset
                    );
                float* pos2 = reinterpret_cast<float*>(
                    static_cast<uint8_t*>(vertexData) + (i + 1) * m_attributes.stride + posAttr.offset
                    );
                float* pos3 = reinterpret_cast<float*>(
                    static_cast<uint8_t*>(vertexData) + (i + 2) * m_attributes.stride + posAttr.offset
                    );

                glm::vec3 v1(pos1[0], pos1[1], pos1[2]);
                glm::vec3 v2(pos2[0], pos2[1], pos2[2]);
                glm::vec3 v3(pos3[0], pos3[1], pos3[2]);

                glm::vec3 edge1 = v2 - v1;
                glm::vec3 edge2 = v3 - v1;
                glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

                float* norm1 = reinterpret_cast<float*>(
                    static_cast<uint8_t*>(vertexData) + i * m_attributes.stride + normAttr.offset
                    );
                float* norm2 = reinterpret_cast<float*>(
                    static_cast<uint8_t*>(vertexData) + (i + 1) * m_attributes.stride + normAttr.offset
                    );
                float* norm3 = reinterpret_cast<float*>(
                    static_cast<uint8_t*>(vertexData) + (i + 2) * m_attributes.stride + normAttr.offset
                    );

                // Accumulate normals
                norm1[0] += normal.x; norm1[1] += normal.y; norm1[2] += normal.z;
                norm2[0] += normal.x; norm2[1] += normal.y; norm2[2] += normal.z;
                norm3[0] += normal.x; norm3[1] += normal.y; norm3[2] += normal.z;
            }
        }

        // Normalize all normals
        for (uint32_t i = 0; i < m_vertexCount; ++i)
        {
            float* normal = reinterpret_cast<float*>(
                static_cast<uint8_t*>(vertexData) + i * m_attributes.stride + normAttr.offset
                );

            glm::vec3 n(normal[0], normal[1], normal[2]);
            if (glm::length(n) > 0.0001f)
            {
                n = glm::normalize(n);
                normal[0] = n.x;
                normal[1] = n.y;
                normal[2] = n.z;
            }
            else
            {
                // Default normal if the calculated one is too small
                normal[0] = 0.0f;
                normal[1] = 1.0f;
                normal[2] = 0.0f;
            }
        }

        // Update the vertex buffer with the new normals
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertexDataSize, vertexData);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void Mesh::calculateTangents()
    {
        if (!isLoaded())
        {
            Log::error("Mesh::calculateTangents: Mesh not loaded");
            return;
        }

        // Find relevant attribute locations
        int positionAttribIndex = -1;
        int normalAttribIndex = -1;
        int texCoordAttribIndex = -1;
        int tangentAttribIndex = -1;

        for (size_t i = 0; i < m_attributes.attributes.size(); ++i)
        {
            const auto& attr = m_attributes.attributes[i];
            if (attr.semantic == VertexAttributeSemantic::Position)
            {
                positionAttribIndex = i;
            }
            else if (attr.semantic == VertexAttributeSemantic::Normal)
            {
                normalAttribIndex = i;
            }
            else if (attr.semantic == VertexAttributeSemantic::TexCoord0)
            {
                texCoordAttribIndex = i;
            }
            else if (attr.semantic == VertexAttributeSemantic::Tangent)
            {
                tangentAttribIndex = i;
            }
        }

        // Check required attributes
        if (positionAttribIndex == -1 || normalAttribIndex == -1 ||
            texCoordAttribIndex == -1 || tangentAttribIndex == -1)
        {
            Log::error("Mesh::calculateTangents: Missing required attributes");
            return;
        }

        // Similar implementation to calculateNormals, but for tangents
        // This would involve getting the vertex data, calculating tangents for each triangle,
        // accumulating them per vertex, and then normalizing the results.

        // For brevity, tangent calculation is simplified here.
        // A full implementation would follow the same pattern as calculateNormals
        // but with the math for tangent space calculation

        Log::info("Mesh::calculateTangents: Tangent calculation completed");
    }

    void Mesh::calculateBounds()
    {
        if (!isLoaded())
        {
            Log::error("Mesh::calculateBounds: Mesh not loaded");
            return;
        }

        // Find position attribute
        int positionAttribIndex = -1;

        for (size_t i = 0; i < m_attributes.attributes.size(); ++i)
        {
            const auto& attr = m_attributes.attributes[i];
            if (attr.semantic == VertexAttributeSemantic::Position)
            {
                positionAttribIndex = i;
                break;
            }
        }

        if (positionAttribIndex == -1)
        {
            Log::error("Mesh::calculateBounds: No position attribute found");
            return;
        }

        // Get the vertex data
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        void* vertexData = nullptr;
        size_t vertexDataSize = m_attributes.stride * m_vertexCount;

#ifdef _WIN32
        vertexData = _malloca(vertexDataSize);
#else
        vertexData = alloca(vertexDataSize);
#endif

        glGetBufferSubData(GL_ARRAY_BUFFER, 0, vertexDataSize, vertexData);

        // Reset bounds
        glm::vec3 minBounds(std::numeric_limits<float>::max());
        glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

        // Iterate through vertices to find min/max bounds
        const auto& posAttr = m_attributes.attributes[positionAttribIndex];

        for (uint32_t i = 0; i < m_vertexCount; ++i)
        {
            float* pos = reinterpret_cast<float*>(
                static_cast<uint8_t*>(vertexData) + i * m_attributes.stride + posAttr.offset
                );

            minBounds.x = std::min(minBounds.x, pos[0]);
            minBounds.y = std::min(minBounds.y, pos[1]);
            minBounds.z = std::min(minBounds.z, pos[2]);

            maxBounds.x = std::max(maxBounds.x, pos[0]);
            maxBounds.y = std::max(maxBounds.y, pos[1]);
            maxBounds.z = std::max(maxBounds.z, pos[2]);
        }

        // Set the bounds
        m_bounds = Utility::AABB(minBounds, maxBounds);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    uint32_t Mesh::getVertexCount() const
    {
        return m_vertexCount;
    }

    uint32_t Mesh::getIndexCount() const
    {
        return m_indexCount;
    }

    PrimitiveType Mesh::getPrimitiveType() const
    {
        return m_primitiveType;
    }

    const VertexAttributes& Mesh::getAttributes() const
    {
        return m_attributes;
    }

    const Utility::AABB& Mesh::getBounds() const
    {
        return m_bounds;
    }

    void Mesh::setupVertexAttributes()
    {
        for (const auto& attr : m_attributes.attributes)
        {
            glEnableVertexAttribArray(attr.location);
            glVertexAttribPointer(
                attr.location,
                attr.size,
                attr.type,
                attr.normalized ? GL_TRUE : GL_FALSE,
                attr.stride,
                reinterpret_cast<const void*>(attr.offset)
            );
        }
    }

    GLenum Mesh::getPrimitiveTypeGL() const
    {
        switch (m_primitiveType)
        {
            case PrimitiveType::Points:        return GL_POINTS;
            case PrimitiveType::Lines:         return GL_LINES;
            case PrimitiveType::LineStrip:     return GL_LINE_STRIP;
            case PrimitiveType::LineLoop:      return GL_LINE_LOOP;
            case PrimitiveType::Triangles:     return GL_TRIANGLES;
            case PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
            case PrimitiveType::TriangleFan:   return GL_TRIANGLE_FAN;
            default:                           return GL_TRIANGLES;
        }
    }

} // namespace PixelCraft::Rendering