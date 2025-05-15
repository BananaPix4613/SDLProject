// -------------------------------------------------------------------------
// ChunkMesh.cpp
// -------------------------------------------------------------------------
#include "Voxel/ChunkMesh.h"
#include "Voxel/Chunk.h"
#include "Voxel/Voxel.h"
#include "Core/Logger.h"
#include "Core/Application.h"
#include "Utility/Profiler.h"
#include "Utility/Ray.h"
#include "Utility/Math.h"
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtx/quaternion.hpp"

#include <algorithm>
#include <queue>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Voxel
{

    namespace
    {
        // Constants for mesh generation
        constexpr float VOXEL_SIZE = 1.0f;
        constexpr int MAX_LOD_LEVELS = 4;
        constexpr float LOD_DISTANCE_THRESHOLDS[MAX_LOD_LEVELS] = {64.0f, 128.0f, 192.0f, 256.0f};

        // Constants for ambient occlusion
        constexpr int AO_RAY_SAMPLES = 16;    // Number of rays per vertex
        constexpr float AO_RAY_LENGTH = 8.0f; // Maximum ray length
        constexpr float AO_RAY_BIAS = 0.05f;  // Bias to avoid self-intersection

        // Face directions (right, left, top, bottom, front, back)
        const std::array<glm::vec3, 6> NORMALS = {
            glm::vec3(1.0f, 0.0f, 0.0f),  // +X
            glm::vec3(-1.0f, 0.0f, 0.0f), // -X
            glm::vec3(0.0f, 1.0f, 0.0f),  // +Y
            glm::vec3(0.0f, -1.0f, 0.0f), // -Y
            glm::vec3(0.0f, 0.0f, 1.0f),  // +Z
            glm::vec3(0.0f, 0.0f, -1.0f)  // -Z
        };

        // Vertex positions for a unit cube
        const std::array<glm::vec3, 8> CUBE_VERTICES = {
            glm::vec3(0.0f, 0.0f, 0.0f), // 0: left-bottom-back
            glm::vec3(1.0f, 0.0f, 0.0f), // 1: right-bottom-back
            glm::vec3(0.0f, 1.0f, 0.0f), // 2: left-top-back
            glm::vec3(1.0f, 1.0f, 0.0f), // 3: right-top-back
            glm::vec3(0.0f, 0.0f, 1.0f), // 4: left-bottom-front
            glm::vec3(1.0f, 0.0f, 1.0f), // 5: right-bottom-front
            glm::vec3(0.0f, 1.0f, 1.0f), // 6: left-top-front
            glm::vec3(1.0f, 1.0f, 1.0f)  // 7: right-top-front
        };

        // Indices for the 6 faces of a cube (CW winding)
        const std::array<std::array<int, 4>, 6> CUBE_FACES = {
            std::array<int, 4>{1, 3, 7, 5}, // Right face (+X)
            std::array<int, 4>{0, 4, 6, 2}, // Left face (-X)
            std::array<int, 4>{2, 6, 7, 3}, // Top face (+Y)
            std::array<int, 4>{0, 1, 5, 4}, // Bottom face (-Y)
            std::array<int, 4>{4, 5, 7, 6}, // Front face (+Z)
            std::array<int, 4>{0, 2, 3, 1}  // Back face (-Z)
        };

        // Texture coordinates for each face
        const std::array<std::array<glm::vec2, 4>, 6> TEX_COORDS = {
            std::array<glm::vec2, 4>{
            glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, 0.0f),
                glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f)
        },
            std::array<glm::vec2, 4>{
            glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f),
                glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f)
        },
            std::array<glm::vec2, 4>{
            glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, 0.0f),
                glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f)
        },
            std::array<glm::vec2, 4>{
            glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f),
                glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)
        },
            std::array<glm::vec2, 4>{
            glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f),
                glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f)
        },
            std::array<glm::vec2, 4>{
            glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f),
                glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f)
        }
        };

        // Colors for each face (for debugging or simple coloring
        const std::array<uint32_t, 6> FACE_COLORS = {
            0xFFE57373, // Right face: Red
            0xFF81C784, // Left face: Green
            0xFF4FC3F7, // Top face: Blue
            0xFFFFF176, // Bottom face: Yellow
            0xFFFFB74D, // Front face: Orange
            0xFFBA68C8  // Back face: Purple
        };

        // Helper for 3D array access
        template<typename T>
        class Array3D
        {
        public:
            Array3D() : m_data()
            {
            }

            Array3D(int sizeX, int sizeY, int sizeZ)
                : m_sizeX(sizeX), m_sizeY(sizeY), m_sizeZ(sizeZ)
            {
                m_data.resize(sizeX * sizeY * sizeZ);
            }

            T& at(int x, int y, int z)
            {
                return m_data[index(x, y, z)];
            }

            const T& at(int x, int y, int z) const
            {
                return m_data[index(x, y, z)];
            }

            void fill(const T& value)
            {
                std::fill(m_data.begin(), m_data.end(), value);
            }

        private:
            size_t index(int x, int y, int z) const
            {
                return (z * m_sizeY * m_sizeX) + (y * m_sizeX) + x;
            }

            std::vector<T> m_data;
            int m_sizeX, m_sizeY, m_sizeZ;
        };

        // Holds edge data for mesh simplification
        struct Edge
        {
            uint32_t v1, v2;
            float error;

            Edge(uint32_t _v1, uint32_t _v2, float _error)
                : v1(_v1), v2(_v2), error(_error)
            {
            }

            bool operator<(const Edge& other) const
            {
                return error > other.error; // Use > for min-heap
            }
        };
    }

    ChunkMesh::ChunkMesh(const ChunkCoord& coord)
        : m_currentLOD(0)
        , m_coord(coord)
        , m_dirty(true)
    {
        updateBounds();
    }

    ChunkMesh::~ChunkMesh()
    {
        // Clear all data
        clear();
    }

    bool ChunkMesh::generate(const Chunk& chunk,
                             const std::array<std::weak_ptr<Chunk>, 6>& neighbors,
                             std::function<bool()> cancelCheck)
    {
        Utility::Profiler::getInstance()->beginSample("ChunkMesh::generate");
        
        // Clear existing mesh data
        clear();

        // If the chunk is empty, nothing to do
        if (chunk.isEmpty())
        {
            Utility::Profiler::getInstance()->endSample();
            return true;
        }

        // Generate using greedy meshing algorithm for better performance
        bool success = generateGreedyMesh(chunk, neighbors, cancelCheck);

        // Check if generation was canceled
        if (cancelCheck && cancelCheck())
        {
            clear();
            Utility::Profiler::getInstance()->endSample();
            return false;
        }

        // Calculate ambient occlusion if generation was successful
        if (success)
        {
            calculateAmbientOcclusion(chunk, neighbors);
        }

        // Update the GPU mesh
        if (success)
        {
            success = updateRenderMesh();
        }

        if (success)
        {
            m_dirty = false;
        }

        Utility::Profiler::getInstance()->endSample();
        return success;
    }

    bool ChunkMesh::generateSimple(const Chunk& chunk, std::function<bool()> cancelCheck)
    {
        Utility::Profiler::getInstance()->beginSample("ChunkMesh::generateSimple");

        // Clear existing mesh data
        clear();

        // If the chunk is empty, nothing to do
        if (chunk.isEmpty())
        {
            Utility::Profiler::getInstance()->endSample();
            return true;
        }

        // Generate using simple cube method (less efficient but doesn't need neighbors)
        bool success = generateCubeMesh(chunk, cancelCheck);

        // Check if generation was canceled
        if (cancelCheck && cancelCheck())
        {
            clear();
            Utility::Profiler::getInstance()->endSample();
            return false;
        }

        // Update the GPU mesh
        if (success)
        {
            success = updateRenderMesh();
        }

        if (success)
        {
            m_dirty = false;
        }

        Utility::Profiler::getInstance()->endSample();
        return success;
    }

    size_t ChunkMesh::getMemoryUsage() const
    {
        size_t usage = 0;

        // Vertex and index data
        usage += m_vertices.size() * sizeof(Vertex);
        usage += m_indices.size() * sizeof(uint32_t);

        // Render mesh
        if (m_renderMesh)
        {
            // Approximate rendering mesh memory
            usage += (m_vertices.size() * sizeof(Vertex)) + (m_indices.size() * sizeof(uint32_t));
        }

        // LOD meshes
        for (const auto& lodMesh : m_lodMeshes)
        {
            if (lodMesh)
            {
                // Approximate each LOD mesh
                usage += (m_vertices.size() * sizeof(Vertex) / (1 << m_currentLOD)) +
                    (m_indices.size() * sizeof(uint32_t) / (1 << m_currentLOD));
            }
        }

        // Other member variables
        usage += sizeof(*this);

        return usage;
    }

    size_t ChunkMesh::getVertexCount() const
    {
        return m_vertices.size();
    }

    size_t ChunkMesh::getIndexCount() const
    {
        return m_indices.size();
    }

    bool ChunkMesh::updateLOD(float distance)
    {
        Utility::Profiler::getInstance()->beginSample("ChunkMesh::updateLOD");

        // Determine appropriate LOD level based on distance thresholds
        int newLOD = 0;
        for (int i = 0; i < MAX_LOD_LEVELS - 1; i++)
        {
            if (distance > LOD_DISTANCE_THRESHOLDS[i])
            {
                newLOD = i + 1;
            }
        }

        // If LOD hasn't changed, nothing to do
        if (newLOD == m_currentLOD)
        {
            Utility::Profiler::getInstance()->endSample();
            return false;
        }

        // Update LOD level
        m_currentLOD = newLOD;

        // Ensure we have LODs created
        if (m_lodMeshes.empty() || m_lodMeshes.size() <= static_cast<size_t>(m_currentLOD))
        {
            createLODs();
        }

        // Use the appropriate LOD mesh
        if (m_currentLOD < static_cast<int>(m_lodMeshes.size()) && m_lodMeshes[m_currentLOD])
        {
            m_renderMesh = m_lodMeshes[m_currentLOD];
        }
        else
        {
            m_renderMesh = m_lodMeshes[0]; // Fallback to base LOD
        }

        Utility::Profiler::getInstance()->endSample();
        return true;
    }

    bool ChunkMesh::createLODs()
    {
        Utility::Profiler::getInstance()->beginSample("ChunkMesh::createLODs");

        // Ensure we have the base mesh
        if (m_vertices.empty() || m_indices.empty() || !m_renderMesh)
        {
            Utility::Profiler::getInstance()->endSample();
            return false;
        }

        // Resize to hold LODs
        m_lodMeshes.resize(MAX_LOD_LEVELS);

        // LOD0 is the original mesh
        m_lodMeshes[0] = m_renderMesh;

        // Define progressively stronger simplification ratios
        const float lodRatios[MAX_LOD_LEVELS - 1] = {0.5f, 0.25f, 0.125f};

        // Create progressively simplified LODs
        for (int lodLevel = 1; lodLevel < MAX_LOD_LEVELS; lodLevel++)
        {
            std::vector<Vertex> simplifiedVertices;
            std::vector<uint32_t> simplifiedIndices;

            // Perform mesh simplification using quadric error metrics
            simplifyMesh(m_vertices, m_indices, simplifiedVertices, simplifiedIndices, lodRatios[lodLevel - 1]);

            // Skip if no vertices generated
            if (simplifiedVertices.empty() || simplifiedIndices.empty())
            {
                continue;
            }

            // Create mesh for this LOD level
            auto lodMesh = std::make_shared<Rendering::Mesh>();

            // Set up vertex attributes (same as in updateRenderMesh)
            Rendering::VertexAttributes attributes;
            attributes.stride = sizeof(Vertex);

            // Set up attributes
            Rendering::VertexAttribute posAttr;
            posAttr.semantic = Rendering::VertexAttributeSemantic::Position;
            posAttr.location = 0;
            posAttr.size = 3;
            posAttr.type = 0x1406; // GL_FLOAT
            posAttr.normalized = false;
            posAttr.stride = sizeof(Vertex);
            posAttr.offset = offsetof(Vertex, position);
            posAttr.name = "position";
            attributes.attributes.push_back(posAttr);

            Rendering::VertexAttribute normAttr;
            normAttr.semantic = Rendering::VertexAttributeSemantic::Normal;
            normAttr.location = 1;
            normAttr.size = 3;
            normAttr.type = 0x1406; // GL_FLOAT
            normAttr.normalized = false;
            normAttr.stride = sizeof(Vertex);
            normAttr.offset = offsetof(Vertex, normal);
            normAttr.name = "normal";
            attributes.attributes.push_back(normAttr);

            Rendering::VertexAttribute texCoordAttr;
            texCoordAttr.semantic = Rendering::VertexAttributeSemantic::TexCoord0;
            texCoordAttr.location = 2;
            texCoordAttr.size = 2;
            texCoordAttr.type = 0x1406; // GL_FLOAT
            texCoordAttr.normalized = false;
            texCoordAttr.stride = sizeof(Vertex);
            texCoordAttr.offset = offsetof(Vertex, texCoord);
            texCoordAttr.name = "texCoord";
            attributes.attributes.push_back(texCoordAttr);

            Rendering::VertexAttribute colorAttr;
            colorAttr.semantic = Rendering::VertexAttributeSemantic::Color;
            colorAttr.location = 3;
            colorAttr.size = 4;
            colorAttr.type = 0x1401; // GL_UNSIGNED_BYTE
            colorAttr.normalized = true;
            colorAttr.stride = sizeof(Vertex);
            colorAttr.offset = offsetof(Vertex, color);
            colorAttr.name = "color";
            attributes.attributes.push_back(colorAttr);

            Rendering::VertexAttribute materialAttr;
            materialAttr.semantic = Rendering::VertexAttributeSemantic::Custom;
            materialAttr.location = 4;
            materialAttr.size = 1;
            materialAttr.type = 0x1403; // GL_UNSIGNED_SHORT
            materialAttr.normalized = false;
            materialAttr.stride = sizeof(Vertex);
            materialAttr.offset = offsetof(Vertex, material);
            materialAttr.name = "material";
            attributes.attributes.push_back(materialAttr);

            Rendering::VertexAttribute occlusionAttr;
            occlusionAttr.semantic = Rendering::VertexAttributeSemantic::Custom;
            occlusionAttr.location = 5;
            occlusionAttr.size = 1;
            occlusionAttr.type = 0x1403; // GL_UNSIGNED_SHORT
            occlusionAttr.normalized = true;
            occlusionAttr.stride = sizeof(Vertex);
            occlusionAttr.offset = offsetof(Vertex, occlusion);
            occlusionAttr.name = "occlusion";
            attributes.attributes.push_back(occlusionAttr);

            // Create the LOD mesh
            if (lodMesh->createFromData(
                simplifiedVertices.data(), static_cast<uint32_t>(simplifiedVertices.size()),
                simplifiedIndices.data(), static_cast<uint32_t>(simplifiedIndices.size()),
                attributes,
                Rendering::PrimitiveType::Triangles))
            {
                m_lodMeshes[lodLevel] = lodMesh;
            }
        }

        Utility::Profiler::getInstance()->endSample();
        return true;
    }

    void ChunkMesh::clear()
    {
        m_vertices.clear();
        m_indices.clear();
        m_renderMesh.reset();
        m_lodMeshes.clear();
        m_dirty = true;
    }

    bool ChunkMesh::raycastVoxel(const Chunk& chunk,
                                 const std::array<std::weak_ptr<Chunk>, 6>& neighbors,
                                 const glm::vec3& origin,
                                 const glm::vec3& direction,
                                 float maxDistance,
                                 float& hitDistance)
    {
        // Initialize ray parameters
        Utility::Ray ray(origin, direction);
        int chunkSize = chunk.getSize();

        // Convert origin to voxel space
        glm::vec3 voxelOrigin = origin / VOXEL_SIZE;
        glm::vec3 voxelDir = direction;

        // Calculate chunk bounds
        glm::vec3 chunkMin(
            chunk.getCoord().x * chunkSize,
            chunk.getCoord().y * chunkSize,
            chunk.getCoord().z * chunkSize
        );
        glm::vec3 chunkMax = chunkMin + glm::vec3(chunkSize);

        // Setup DDA ray casting
        glm::vec3 deltaDist = glm::abs(glm::vec3(1.0f) / (voxelDir + glm::vec3(0.0001f))); // Avoid div by zero

        // Calculate step direction
        glm::ivec3 step;
        step.x = (voxelDir.x >= 0) ? 1 : -1;
        step.y = (voxelDir.y >= 0) ? 1 : -1;
        step.z = (voxelDir.z >= 0) ? 1 : -1;

        // Current voxel coordinates
        glm::ivec3 voxelPos = glm::ivec3(glm::floor(voxelOrigin));

        // Distance to next voxel boundary
        glm::vec3 sideDist;
        sideDist.x = (step.x > 0) ?
            (glm::ceil(voxelOrigin.x) - voxelOrigin.x) * deltaDist.x :
            (voxelOrigin.x - glm::floor(voxelOrigin.x)) * deltaDist.x;
        sideDist.y = (step.y > 0) ?
            (glm::ceil(voxelOrigin.y) - voxelOrigin.y) * deltaDist.y :
            (voxelOrigin.y - glm::floor(voxelOrigin.y)) * deltaDist.y;
        sideDist.z = (step.z > 0) ?
            (glm::ceil(voxelOrigin.z) - voxelOrigin.z) * deltaDist.z :
            (voxelOrigin.z - glm::floor(voxelOrigin.z)) * deltaDist.z;

        // Current distance traveled
        float distance = 0.0f;

        // DDA algorithm
        while (distance < maxDistance)
        {
            // Check current voxel
            int maskX = (voxelPos.x >= chunkMin.x && voxelPos.x < chunkMax.x) ? 1 : 0;
            int maskY = (voxelPos.y >= chunkMin.y && voxelPos.y < chunkMax.y) ? 1 : 0;
            int maskZ = (voxelPos.z >= chunkMin.z && voxelPos.z < chunkMax.z) ? 1 : 0;

            if (maskX & maskY & maskZ)
            {
                // Inside the chunk
                glm::ivec3 localPos = voxelPos - glm::ivec3(chunkMin);
                const Voxel& voxel = chunk.getVoxel(localPos.x, localPos.y, localPos.z);

                if (!voxel.isEmpty())
                {
                    hitDistance = distance;
                    return true;
                }
            }
            else
            {
                // In a neighbor chunk
                int neighborIdx = -1;
                glm::ivec3 neighborLocalPos;

                if (voxelPos.x < chunkMin.x)
                {
                    neighborIdx = 1; // -X neighbor
                    neighborLocalPos = glm::ivec3(voxelPos) - glm::ivec3(chunkMin) + glm::ivec3(chunkSize, 0, 0);
                }
                else if (voxelPos.x >= chunkMax.x)
                {
                    neighborIdx = 0; // +X neighbor
                    neighborLocalPos = glm::ivec3(voxelPos) - glm::ivec3(chunkMax) + glm::ivec3(0, 0, 0);
                }
                else if (voxelPos.y < chunkMin.y)
                {
                    neighborIdx = 3; // -Y neighbor
                    neighborLocalPos = glm::ivec3(voxelPos) - glm::ivec3(chunkMin) + glm::ivec3(0, chunkSize, 0);
                }
                else if (voxelPos.y >= chunkMax.y)
                {
                    neighborIdx = 2; // +Y neighbor
                    neighborLocalPos = glm::ivec3(voxelPos) - glm::ivec3(chunkMax) + glm::ivec3(0, 0, 0);
                }
                else if (voxelPos.z < chunkMin.z)
                {
                    neighborIdx = 5; // -Z neighbor
                    neighborLocalPos = glm::ivec3(voxelPos) - glm::ivec3(chunkMin) + glm::ivec3(0, 0, chunkSize);
                }
                else if (voxelPos.z >= chunkMax.z)
                {
                    neighborIdx = 4; // +Z neighbor
                    neighborLocalPos = glm::ivec3(voxelPos) - glm::ivec3(chunkMax) + glm::ivec3(0, 0, 0);
                }

                if (neighborIdx >= 0)
                {
                    auto neighbor = neighbors[neighborIdx].lock();
                    if (neighbor)
                    {
                        // Get voxel from neighboring chunk
                        if (neighborLocalPos.x >= 0 && neighborLocalPos.x < chunkSize &&
                            neighborLocalPos.y >= 0 && neighborLocalPos.y < chunkSize &&
                            neighborLocalPos.z >= 0 && neighborLocalPos.z < chunkSize)
                        {
                            const Voxel& voxel = neighbor->getVoxel(
                                neighborLocalPos.x, neighborLocalPos.y, neighborLocalPos.z);

                            if (!voxel.isEmpty())
                            {
                                hitDistance = distance;
                                return true;
                            }
                        }
                    }
                }
            }

            // Step to next voxel
            if (sideDist.x < sideDist.y && sideDist.x < sideDist.z)
            {
                distance = sideDist.x;
                sideDist.x += deltaDist.x;
                voxelPos.x += step.x;
            }
            else if (sideDist.y < sideDist.z)
            {
                distance = sideDist.y;
                sideDist.y += deltaDist.y;
                voxelPos.y += step.y;
            }
            else
            {
                distance = sideDist.z;
                sideDist.z += deltaDist.z;
                voxelPos.z += step.z;
            }
        }

        // No hit within max distance
        return false;
    }

    std::vector<glm::vec3> ChunkMesh::generateHemisphereRays(const glm::vec3& normal, int sampleCount)
    {
        std::vector<glm::vec3> rays;
        rays.reserve(sampleCount);

        // Create a rotation that aligns +Y to the normal
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        glm::quat rotation;
        float dot = glm::dot(up, normal);

        if (dot > 0.999f)
        {
            // Normal already points up, no rotation needed
            rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }
        else if (dot < -0.999f)
        {
            // Normal points down, rotate 180 degrees around X
            rotation = glm::quat(0.0f, 1.0f, 0.0f, 0.0f);
        }
        else
        {
            // Create rotation from cross product
            glm::vec3 axis = glm::cross(up, normal);
            float angle = std::acos(dot);
            rotation = glm::angleAxis(angle, glm::normalize(axis));
        }

        // Generate rays using stratified sampling
        int sqrtSamples = static_cast<int>(std::sqrt(sampleCount));
        float invSqrtSamples = 1.0f / static_cast<float>(sqrtSamples);

        for (int x = 0; x < sqrtSamples; x++)
        {
            for (int y = 0; y < sqrtSamples; y++)
            {
                // Stratified position in grid
                float u = (x + Utility::Math::randomRange(0.0f, 1.0f)) * invSqrtSamples;
                float v = (y + Utility::Math::randomRange(0.0f, 1.0f)) * invSqrtSamples;

                // Convert to spherical coordinates - use cosine-weighted distribution
                float phi = 2.0f * glm::pi<float>() * u;
                float cosTheta = std::sqrt(v); // Cosine-weighted
                float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);

                // Convert to cartesian coordinates in local space
                glm::vec3 localRay(
                    std::cos(phi) * sinTheta,
                    cosTheta,                  // Points up in local space
                    std::sin(phi) * sinTheta
                );

                // Rotate into world space
                glm::vec3 worldRay = rotation * localRay;

                rays.push_back(glm::normalize(worldRay));
            }
        }

        return rays;
    }

    bool ChunkMesh::updateRenderMesh()
    {
        if (m_vertices.empty() || m_indices.empty())
        {
            return false;
        }

        try
        {
            // Create render mesh
            m_renderMesh = std::make_shared<Rendering::Mesh>();

            // Set up vertex attributes
            Rendering::VertexAttributes attributes;
            attributes.stride = sizeof(Vertex);

            // Add position attribute
            Rendering::VertexAttribute posAttr;
            posAttr.semantic = Rendering::VertexAttributeSemantic::Position;
            posAttr.location = 0;
            posAttr.size = 3;  // 3 components (x, y, z)
            posAttr.type = 0x1406; // GL_FLOAT
            posAttr.normalized = false;
            posAttr.stride = sizeof(Vertex);
            posAttr.offset = offsetof(Vertex, position);
            posAttr.name = "position";
            attributes.attributes.push_back(posAttr);

            // Add normal attribute
            Rendering::VertexAttribute normAttr;
            normAttr.semantic = Rendering::VertexAttributeSemantic::Normal;
            normAttr.location = 1;
            normAttr.size = 3;  // 3 components (x, y, z)
            normAttr.type = 0x1406; // GL_FLOAT
            normAttr.normalized = false;
            normAttr.stride = sizeof(Vertex);
            normAttr.offset = offsetof(Vertex, normal);
            normAttr.name = "normal";
            attributes.attributes.push_back(normAttr);

            // Add texcoord attribute
            Rendering::VertexAttribute texCoordAttr;
            texCoordAttr.semantic = Rendering::VertexAttributeSemantic::TexCoord0;
            texCoordAttr.location = 2;
            texCoordAttr.size = 2;  // 2 components (u, v)
            texCoordAttr.type = 0x1406; // GL_FLOAT
            texCoordAttr.normalized = false;
            texCoordAttr.stride = sizeof(Vertex);
            texCoordAttr.offset = offsetof(Vertex, texCoord);
            texCoordAttr.name = "texCoord";
            attributes.attributes.push_back(texCoordAttr);

            // Add color attribute
            Rendering::VertexAttribute colorAttr;
            colorAttr.semantic = Rendering::VertexAttributeSemantic::Color;
            colorAttr.location = 3;
            colorAttr.size = 4;  // 4 components (r, g, b, a) packed in uint32
            colorAttr.type = 0x1401; // GL_UNSIGNED_BYTE
            colorAttr.normalized = true;
            colorAttr.stride = sizeof(Vertex);
            colorAttr.offset = offsetof(Vertex, color);
            colorAttr.name = "color";
            attributes.attributes.push_back(colorAttr);

            // Add material attribute (custom)
            Rendering::VertexAttribute materialAttr;
            materialAttr.semantic = Rendering::VertexAttributeSemantic::Custom;
            materialAttr.location = 4;
            materialAttr.size = 1;  // 1 component
            materialAttr.type = 0x1403; // GL_UNSIGNED_SHORT
            materialAttr.normalized = false;
            materialAttr.stride = sizeof(Vertex);
            materialAttr.offset = offsetof(Vertex, material);
            materialAttr.name = "material";
            attributes.attributes.push_back(materialAttr);

            // Add occlusion attribute (custom)
            Rendering::VertexAttribute occlusionAttr;
            occlusionAttr.semantic = Rendering::VertexAttributeSemantic::Custom;
            occlusionAttr.location = 5;
            occlusionAttr.size = 1;  // 1 component
            occlusionAttr.type = 0x1403; // GL_UNSIGNED_SHORT
            occlusionAttr.normalized = true;
            occlusionAttr.stride = sizeof(Vertex);
            occlusionAttr.offset = offsetof(Vertex, occlusion);
            occlusionAttr.name = "occlusion";
            attributes.attributes.push_back(occlusionAttr);

            // Set mesh data
            if (!m_renderMesh->createFromData(
                m_vertices.data(), static_cast<uint32_t>(m_vertices.size()),
                m_indices.data(), static_cast<uint32_t>(m_indices.size()),
                attributes,
                Rendering::PrimitiveType::Triangles))
            {
                Log::error("Failed to create render mesh for chunk at " + m_coord.toString());
                return false;
            }

            // Store this as the full detail LOD
            if (m_lodMeshes.empty())
            {
                m_lodMeshes.push_back(m_renderMesh);
            }
            else
            {
                m_lodMeshes[0] = m_renderMesh;
            }

            return true;
        }
        catch (const std::exception& e)
        {
            Log::error("Exception creating render mesh: " + std::string(e.what()));
            return false;
        }
    }

    bool ChunkMesh::generateGreedyMesh(const Chunk& chunk,
                                       const std::array<std::weak_ptr<Chunk>, 6>& neighbors,
                                       std::function<bool()> cancelCheck)
    {
        int chunkSize = chunk.getSize();

        // Prepare merged face tracking
        // For each face direction, we need a 2D mask to track which faces have been merged
        std::array<Array3D<bool>, 6> processed;
        for (int dir = 0; dir < 6; dir++)
        {
            processed[dir] = Array3D<bool>(chunkSize, chunkSize, chunkSize);
            processed[dir].fill(false);
        }

        // World offset for this chunk
        glm::vec3 chunkOffset(
            m_coord.x * chunkSize * VOXEL_SIZE,
            m_coord.y * chunkSize * VOXEL_SIZE,
            m_coord.z * chunkSize * VOXEL_SIZE
        );

        // Process each direction separately
        for (int faceDir = 0; faceDir < 6; faceDir++)
        {
            // Check for cancellation periodically
            if (cancelCheck && cancelCheck())
            {
                return false;
            }

            // Get neighbor in this direction if available
            std::shared_ptr<Chunk> neighbor = neighbors[faceDir].lock();

            // Get normal for this face direction
            const glm::vec3& normal = NORMALS[faceDir];

            // Determine the primary and secondary axes for this face
            int primaryAxis = 0, secondaryAxis = 0;
            for (int i = 0; i < 3; i++)
            {
                if (normal[i] != 0)
                {
                    primaryAxis = i;
                    break;
                }
            }
            secondaryAxis = (primaryAxis + 1) % 3;
            int tertiaryAxis = (primaryAxis + 2) % 3;

            // Iterate through each slice along the primary axis
            for (int primaryPos = 0; primaryPos < chunkSize; primaryPos++)
            {
                // For each voxel in the slice
                for (int secondaryPos = 0; secondaryPos < chunkSize; secondaryPos++)
                {
                    for (int tertiaryPos = 0; tertiaryPos < chunkSize; tertiaryPos++)
                    {
                        // Convert to x,y,z
                        int x = 0, y = 0, z = 0;
                        if (primaryAxis == 0)
                        {
                            x = primaryPos;
                        }
                        else if (primaryAxis == 1)
                        {
                            y = primaryPos;
                        }
                        else
                        {
                            z = primaryPos;
                        }

                        if (secondaryAxis == 0)
                        {
                            x = secondaryPos;
                        }
                        else if (secondaryAxis == 1)
                        {
                            y = secondaryPos;
                        }
                        else
                        {
                            z = secondaryPos;
                        }

                        if (tertiaryAxis == 0)
                        {
                            x = tertiaryPos;
                        }
                        else if (tertiaryAxis == 1)
                        {
                            y = tertiaryPos;
                        }
                        else
                        {
                            z = tertiaryPos;
                        }

                        // Skip if already processed
                        if (processed[faceDir].at(x, y, z))
                        {
                            continue;
                        }

                        // Get the current voxel
                        const Voxel& voxel = chunk.getVoxel(x, y, z);

                        // Skip empty voxels
                        if (voxel.isEmpty())
                        {
                            processed[faceDir].at(x, y, z) = true;
                            continue;
                        }

                        // Check if the face is visible (adjacent voxel is empty or different material)
                        bool isVisible = false;

                        // Calculate adjacent position
                        int nx = x + static_cast<int>(normal.x);
                        int ny = y + static_cast<int>(normal.y);
                        int nz = z + static_cast<int>(normal.z);

                        // Check if adjacent position is out of bounds
                        if (nx < 0 || nx >= chunkSize || ny < 0 || ny >= chunkSize || nz < 0 || nz >= chunkSize)
                        {
                            // Check neighbor chunk if available
                            if (neighbor)
                            {
                                // Convert to neighbor local coordinates
                                int nnx = (nx < 0) ? chunkSize + nx : (nx >= chunkSize) ? nx - chunkSize : nx;
                                int nny = (ny < 0) ? chunkSize + ny : (ny >= chunkSize) ? ny - chunkSize : ny;
                                int nnz = (nz < 0) ? chunkSize + nz : (nz >= chunkSize) ? nz - chunkSize : nz;

                                // Get voxel from neighbor
                                const Voxel& neighborVoxel = neighbor->getVoxel(nnx, nny, nnz);
                                isVisible = neighborVoxel.isEmpty() || neighborVoxel.type != voxel.type;
                            }
                            else
                            {
                                // No neighbor, face is visible
                                isVisible = true;
                            }
                        }
                        else
                        {
                            // Check adjacent voxel in this chunk
                            const Voxel& adjacentVoxel = chunk.getVoxel(nx, ny, nz);
                            isVisible = adjacentVoxel.isEmpty() || adjacentVoxel.type != voxel.type;
                        }

                        // Skip non-visible faces
                        if (!isVisible)
                        {
                            processed[faceDir].at(x, y, z) = true;
                            continue;
                        }

                        // Greedy meshing: Try to expand this face in both secondary and tertiary axes
                        int endSecondary = secondaryPos;
                        int endTertiary = tertiaryPos;

                        // Expand along tertiary axis first
                        for (int t = tertiaryPos + 1; t < chunkSize; t++)
                        {
                            // Convert to x,y,z
                            int tx = x, ty = y, tz = z;

                            if (tertiaryAxis == 0)
                            {
                                tx = t;
                            }
                            else if (tertiaryAxis == 1)
                            {
                                ty = t;
                            }
                            else
                            {
                                tz = t;
                            }

                            // Check if this voxel matches and has a visible face
                            const Voxel& nextVoxel = chunk.getVoxel(tx, ty, tz);

                            // If already processed, different type, or not visible, stop expansion
                            if (processed[faceDir].at(tx, ty, tz) || nextVoxel.type != voxel.type)
                            {
                                break;
                            }

                            // Check visiblity of the face
                            int ntx = tx + static_cast<int>(normal.x);
                            int nty = ty + static_cast<int>(normal.y);
                            int ntz = tz + static_cast<int>(normal.z);

                            bool nextVisible = false;

                            if (ntx < 0 || ntx >= chunkSize || nty < 0 || nty >= chunkSize || ntz < 0 || ntz >= chunkSize)
                            {
                                if (neighbor)
                                {
                                    int nntx = (ntx < 0) ? chunkSize + ntx : (ntx >= chunkSize) ? ntx - chunkSize : ntx;
                                    int nnty = (nty < 0) ? chunkSize + nty : (nty >= chunkSize) ? nty - chunkSize : nty;
                                    int nntz = (ntz < 0) ? chunkSize + ntz : (ntz >= chunkSize) ? ntz - chunkSize : ntz;

                                    const Voxel& neighborVoxel = neighbor->getVoxel(nntx, nnty, nntz);
                                    nextVisible = neighborVoxel.isEmpty() || neighborVoxel.type != voxel.type;
                                }
                                else
                                {
                                    nextVisible = true;
                                }
                            }
                            else
                            {
                                const Voxel& adjacentVoxel = chunk.getVoxel(ntx, nty, ntz);
                                nextVisible = adjacentVoxel.isEmpty() || adjacentVoxel.type != voxel.type;
                            }

                            if (!nextVisible)
                            {
                                break;
                            }

                            // Expand
                            endTertiary = t;
                        }

                        // Now try to expand along secondary axis
                        bool canExpandSecondary = true;
                        for (int s = secondaryPos + 1; s < chunkSize && canExpandSecondary; s++)
                        {
                            // For each position in the expanded tertiary axis, check if it can be included
                            for (int t = tertiaryPos; t <= endTertiary; t++)
                            {
                                // Convert to x,y,z
                                int sx = x, sy = y, sz = z;
                                int tx = x, ty = y, tz = z;

                                if (secondaryAxis == 0)
                                {
                                    sx = s;
                                }
                                else if (secondaryAxis == 1)
                                {
                                    sy = s;
                                }
                                else
                                {
                                    sz = s;
                                }

                                if (tertiaryAxis == 0)
                                {
                                    tx = t;
                                }
                                else if (tertiaryAxis == 1)
                                {
                                    ty = t;
                                }
                                else
                                {
                                    tz = t;
                                }

                                // Combined position
                                int cx = 0, cy = 0, cz = 0;

                                if (primaryAxis == 0)
                                {
                                    cx = x;
                                }
                                else if (primaryAxis == 1)
                                {
                                    cy = y;
                                }
                                else
                                {
                                    cz = z;
                                }

                                if (secondaryAxis == 0)
                                {
                                    cx = s;
                                }
                                else if (secondaryAxis == 1)
                                {
                                    cy = s;
                                }
                                else
                                {
                                    cz = s;
                                }

                                if (tertiaryAxis == 0)
                                {
                                    cx = t;
                                }
                                else if (tertiaryAxis == 1)
                                {
                                    cy = t;
                                }
                                else
                                {
                                    cz = t;
                                }

                                // Check if this voxel matches and has a visible face
                                const Voxel& nextVoxel = chunk.getVoxel(cx, cy, cz);

                                // If already processed, different type, or not visible, stop expansion
                                if (processed[faceDir].at(cx, cy, cz) || nextVoxel.type != voxel.type)
                                {
                                    canExpandSecondary = false;
                                    break;
                                }

                                // Check visibility of the face
                                int ncx = cx + static_cast<int>(normal.x);
                                int ncy = cy + static_cast<int>(normal.y);
                                int ncz = cz + static_cast<int>(normal.z);

                                bool nextVisible = false;

                                if (ncx < 0 || ncx >= chunkSize || ncy < 0 || ncy >= chunkSize || ncz < 0 || ncz >= chunkSize)
                                {
                                    if (neighbor)
                                    {
                                        int nncx = (ncx < 0) ? chunkSize + ncx : (ncx >= chunkSize) ? ncx - chunkSize : ncx;
                                        int nncy = (ncy < 0) ? chunkSize + ncy : (ncy >= chunkSize) ? ncy - chunkSize : ncy;
                                        int nncz = (ncz < 0) ? chunkSize + ncz : (ncz >= chunkSize) ? ncz - chunkSize : ncz;

                                        const Voxel& neighborVoxel = neighbor->getVoxel(nncx, nncy, nncz);
                                        nextVisible = neighborVoxel.isEmpty() || neighborVoxel.type != voxel.type;
                                    }
                                    else
                                    {
                                        nextVisible = true;
                                    }
                                }
                                else
                                {
                                    const Voxel& adjacentVoxel = chunk.getVoxel(ncx, ncy, ncz);
                                    nextVisible = adjacentVoxel.isEmpty() || adjacentVoxel.type != voxel.type;
                                }

                                if (!nextVisible)
                                {
                                    canExpandSecondary = false;
                                    break;
                                }
                            }

                            // If expansion is valid, update the end position
                            if (canExpandSecondary)
                            {
                                endSecondary = s;
                            }
                        }

                        // Mark all included faces as processed
                        for (int s = secondaryPos; s <= endSecondary; s++)
                        {
                            for (int t = tertiaryPos; t <= endTertiary; t++)
                            {
                                // Convert to x,y,z
                                int px = 0, py = 0, pz = 0;

                                if (primaryAxis == 0)
                                {
                                    px = x;
                                }
                                else if (primaryAxis == 1)
                                {
                                    py = y;
                                }
                                else
                                {
                                    pz = z;
                                }

                                if (secondaryAxis == 0)
                                {
                                    px = s;
                                }
                                else if (secondaryAxis == 1)
                                {
                                    py = s;
                                }
                                else
                                {
                                    pz = s;
                                }

                                if (tertiaryAxis == 0)
                                {
                                    px = t;
                                }
                                else if (tertiaryAxis == 1)
                                {
                                    py = t;
                                }
                                else
                                {
                                    pz = t;
                                }

                                processed[faceDir].at(px, py, pz) = true;
                            }
                        }

                        // Create vertices for the merged face
                        uint32_t faceColor = FACE_COLORS[faceDir];

                        // Calculate merged face dimensions
                        float width = static_cast<float>(endSecondary - secondaryPos + 1) * VOXEL_SIZE;
                        float height = static_cast<float>(endTertiary - tertiaryPos + 1) * VOXEL_SIZE;

                        // Create quad vertices
                        std::array<Vertex, 4> quadVertices;

                        // Set positions based on face direction
                        glm::vec3 basePos(x * VOXEL_SIZE, y * VOXEL_SIZE, z * VOXEL_SIZE);
                        basePos += chunkOffset;

                        if (faceDir == 0)
                        { // +X
                            quadVertices[0].position = basePos + glm::vec3(VOXEL_SIZE, 0, 0);
                            quadVertices[1].position = basePos + glm::vec3(VOXEL_SIZE, height, 0);
                            quadVertices[2].position = basePos + glm::vec3(VOXEL_SIZE, height, width);
                            quadVertices[3].position = basePos + glm::vec3(VOXEL_SIZE, 0, width);
                        }
                        else if (faceDir == 1)
                        { // -X
                            quadVertices[0].position = basePos + glm::vec3(0, 0, width);
                            quadVertices[1].position = basePos + glm::vec3(0, height, width);
                            quadVertices[2].position = basePos + glm::vec3(0, height, 0);
                            quadVertices[3].position = basePos + glm::vec3(0, 0, 0);
                        }
                        else if (faceDir == 2)
                        { // +Y
                            quadVertices[0].position = basePos + glm::vec3(0, VOXEL_SIZE, 0);
                            quadVertices[1].position = basePos + glm::vec3(0, VOXEL_SIZE, width);
                            quadVertices[2].position = basePos + glm::vec3(height, VOXEL_SIZE, width);
                            quadVertices[3].position = basePos + glm::vec3(height, VOXEL_SIZE, 0);
                        }
                        else if (faceDir == 3)
                        { // -Y
                            quadVertices[0].position = basePos + glm::vec3(0, 0, 0);
                            quadVertices[1].position = basePos + glm::vec3(height, 0, 0);
                            quadVertices[2].position = basePos + glm::vec3(height, 0, width);
                            quadVertices[3].position = basePos + glm::vec3(0, 0, width);
                        }
                        else if (faceDir == 4)
                        { // +Z
                            quadVertices[0].position = basePos + glm::vec3(0, 0, VOXEL_SIZE);
                            quadVertices[1].position = basePos + glm::vec3(width, 0, VOXEL_SIZE);
                            quadVertices[2].position = basePos + glm::vec3(width, height, VOXEL_SIZE);
                            quadVertices[3].position = basePos + glm::vec3(0, height, VOXEL_SIZE);
                        }
                        else
                        { // -Z
                            quadVertices[0].position = basePos + glm::vec3(width, 0, 0);
                            quadVertices[1].position = basePos + glm::vec3(0, 0, 0);
                            quadVertices[2].position = basePos + glm::vec3(0, height, 0);
                            quadVertices[3].position = basePos + glm::vec3(width, height, 0);
                        }

                        // Set shared attributes for all vertices
                        for (int i = 0; i < 4; i++)
                        {
                            quadVertices[i].normal = normal;
                            quadVertices[i].color = faceColor;
                            quadVertices[i].material = voxel.type;
                            quadVertices[i].occlusion = 0; // Will be calculated later
                            quadVertices[i].texCoord = TEX_COORDS[faceDir][i];
                        }

                        // Add indices for the quad (two triangles)
                        uint32_t baseIndex = static_cast<uint32_t>(m_vertices.size());
                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 1);
                        m_indices.push_back(baseIndex + 2);

                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex + 3);

                        // Add vertices
                        for (const auto& vertex : quadVertices)
                        {
                            m_vertices.push_back(vertex);
                        }
                    }
                }
            }
        }

        return true;
    }

    bool ChunkMesh::generateCubeMesh(const Chunk& chunk, std::function<bool()> cancelCheck)
    {
        int chunkSize = chunk.getSize();

        // World offset for this chunk
        glm::vec3 chunkOffset(
            m_coord.x * chunkSize * VOXEL_SIZE,
            m_coord.y * chunkSize * VOXEL_SIZE,
            m_coord.z * chunkSize * VOXEL_SIZE
        );

        // Process each voxel
        for (int x = 0; x < chunkSize; x++)
        {
            for (int y = 0; y < chunkSize; y++)
            {
                for (int z = 0; z < chunkSize; z++)
                {
                    // Check for cancellation every 1000 voxels
                    if (cancelCheck && ((x * chunkSize * chunkSize + y * chunkSize + z) % 1000 == 0))
                    {
                        if (cancelCheck())
                        {
                            return false;
                        }
                    }

                    // Get the voxel
                    const Voxel& voxel = chunk.getVoxel(x, y, z);

                    // Skip empty voxels
                    if (voxel.isEmpty())
                    {
                        continue;
                    }

                    // Create voxel cube
                    glm::vec3 pos(x * VOXEL_SIZE, y * VOXEL_SIZE, z * VOXEL_SIZE);
                    pos += chunkOffset;

                    // Check each face
                    for (int faceDir = 0; faceDir < 6; faceDir++)
                    {
                        // Check if face is visible
                        const glm::vec3& normal = NORMALS[faceDir];

                        // Calculate adjacent position
                        int nx = x + static_cast<int>(normal.x);
                        int ny = y + static_cast<int>(normal.y);
                        int nz = z + static_cast<int>(normal.z);

                        // If adjacent voxel is outside chunk, face is visible
                        bool isVisible = true;

                        // If adjacent voxel is inside chunk, check if it's empty
                        if (nx >= 0 && nx < chunkSize && ny >= 0 && ny < chunkSize && nz >= 0 && nz < chunkSize)
                        {
                            const Voxel& adjacentVoxel = chunk.getVoxel(nx, ny, nz);
                            isVisible = adjacentVoxel.isEmpty() || adjacentVoxel.type != voxel.type;
                        }

                        // If face is not visible, skip
                        if (!isVisible)
                        {
                            continue;
                        }

                        // Create face vertices
                        uint32_t faceColor = FACE_COLORS[faceDir];

                        // Add vertices for the face
                        std::array<Vertex, 4> faceVertices;

                        // Set positions based on cube vertices for this face
                        for (int i = 0; i < 4; i++)
                        {
                            int vertIndex = CUBE_FACES[faceDir][i];
                            faceVertices[i].position = pos + CUBE_VERTICES[vertIndex] * VOXEL_SIZE;
                            faceVertices[i].normal = normal;
                            faceVertices[i].color = faceColor;
                            faceVertices[i].material = voxel.type;
                            faceVertices[i].occlusion = 0; // Will be calculated later
                            faceVertices[i].texCoord = TEX_COORDS[faceDir][i];
                        }

                        // Add indices for the face (two triangles)
                        uint32_t baseIndex = static_cast<uint32_t>(m_vertices.size());
                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 1);
                        m_indices.push_back(baseIndex + 2);

                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex + 3);

                        // Add vertices
                        for (const auto& vertex : faceVertices)
                        {
                            m_vertices.push_back(vertex);
                        }
                    }
                }
            }
        }

        return true;
    }

    void ChunkMesh::simplifyMesh(const std::vector<Vertex>& vertices,
                                 const std::vector<uint32_t>& indices,
                                 std::vector<Vertex>& outVertices,
                                 std::vector<uint32_t>& outIndices,
                                 float targetRatio)
    {
        // Early exit if mesh is too small
        if (vertices.size() < 8 || indices.size() < 12)
        {
            outVertices = vertices;
            outIndices = indices;
            return;
        }

        // Create output vertices (start with a copy)
        outVertices = vertices;

        // Build adjacency information
        std::vector<std::vector<uint32_t>> vertexFaces(vertices.size());
        std::vector<glm::vec3> faceNormals(indices.size() / 3);

        for (size_t i = 0; i < indices.size(); i += 3)
        {
            uint32_t idx1 = indices[i];
            uint32_t idx2 = indices[i + 1];
            uint32_t idx3 = indices[i + 2];

            vertexFaces[idx1].push_back(i / 3);
            vertexFaces[idx2].push_back(i / 3);
            vertexFaces[idx3].push_back(i / 3);

            // Calculate face normal
            glm::vec3 v1 = vertices[idx1].position;
            glm::vec3 v2 = vertices[idx2].position;
            glm::vec3 v3 = vertices[idx3].position;

            glm::vec3 normal = glm::normalize(glm::cross(v2 - v1, v3 - v1));
            faceNormals[i / 3] = normal;
        }

        // Calculates how many vertices to remove
        size_t targetVertexCount = static_cast<size_t>(vertices.size() * targetRatio);
        size_t verticesToRemove = vertices.size() - targetVertexCount;

        // Build edge collapse priority queue
        std::priority_queue<Edge> edgeQueue;
        std::unordered_set<std::pair<uint32_t, uint32_t>,
            std::hash<std::pair<uint32_t, uint32_t>>> processedEdges;

        // Process each triangle
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            for (size_t j = 0; j < 3; j++)
            {
                uint32_t v1 = indices[i + j];
                uint32_t v2 = indices[i + ((j + 1) % 3)];

                // Ensure consistent ordering
                if (v1 > v2) std::swap(v1, v2);

                // Skip if already processed
                auto edgePair = std::make_pair(v1, v2);
                if (processedEdges.find(edgePair) != processedEdges.end())
                {
                    continue;
                }

                processedEdges.insert(edgePair);

                // Calculate edge collapse error
                glm::vec3 midpoint = (outVertices[v1].position + outVertices[v2].position) * 0.5f;
                float error = 0.0f;

                // Sum squared distances to face planes
                for (uint32_t faceIdx : vertexFaces[v1])
                {
                    glm::vec3 normal = faceNormals[faceIdx];
                    glm::vec3 p = outVertices[indices[faceIdx * 3]].position;
                    float d = glm::dot(normal, p);
                    float dist = std::abs(glm::dot(normal, midpoint) - d);
                    error += dist * dist;
                }

                for (uint32_t faceIdx : vertexFaces[v2])
                {
                    // Skip faces already counted
                    bool alreadyCounted = false;
                    for (uint32_t f1 : vertexFaces[v1])
                    {
                        if (f1 == faceIdx)
                        {
                            alreadyCounted = true;
                            break;
                        }
                    }

                    if (alreadyCounted) continue;

                    glm::vec3 normal = faceNormals[faceIdx];
                    glm::vec3 p = outVertices[indices[faceIdx * 3]].position;
                    float d = glm::dot(normal, p);
                    float dist = std::abs(glm::dot(normal, midpoint) - d);
                    error += dist * dist;
                }

                // Also consider texture coordinates and other attributes
                float uvError = glm::distance(outVertices[v1].texCoord, outVertices[v2].texCoord);
                float normalError = 1.0f - glm::dot(
                    glm::normalize(outVertices[v1].normal),
                    glm::normalize(outVertices[v2].normal));
                float materialError = (outVertices[v1].material != outVertices[v2].material) ? 1000.0f : 0.0f;

                error += uvError * 5.0f + normalError * 10.0f + materialError;

                // Add to queue
                edgeQueue.push(Edge(v1, v2, error));
            }
        }

        // Track vertices that have been collapsed
        std::vector<int> vertexRemapped(vertices.size(), -1);
        std::vector<bool> vertexRemoved(vertices.size(), false);
        int collapseCount = 0;

        // Process edges in order of increasing order
        while (!edgeQueue.empty() && collapseCount < verticesToRemove)
        {
            Edge edge = edgeQueue.top();
            edgeQueue.pop();

            uint32_t v1 = edge.v1;
            uint32_t v2 = edge.v2;

            // Skip if either vertex has been removed
            if (vertexRemoved[v1] || vertexRemoved[v2])
            {
                continue;
            }

            // Collapse v2 into v1
            vertexRemoved[v2] = true;
            vertexRemapped[v2] = v1;

            // Merge vertex attributes
            outVertices[v1].position = (outVertices[v1].position + outVertices[v2].position) * 0.5f;
            outVertices[v1].normal = glm::normalize(outVertices[v1].normal + outVertices[v2].normal);
            // For simplicity, keep other attributes from v1

            collapseCount++;
        }

        // Build new index buffer
        outIndices.clear();
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            uint32_t idx1 = indices[i];
            uint32_t idx2 = indices[i + 1];
            uint32_t idx3 = indices[i + 2];

            // Remap vertices
            while (vertexRemapped[idx1] >= 0) idx1 = vertexRemapped[idx1];
            while (vertexRemapped[idx2] >= 0) idx2 = vertexRemapped[idx2];
            while (vertexRemapped[idx3] >= 0) idx3 = vertexRemapped[idx3];

            // Skip degenerate triangles
            if (idx1 == idx2 || idx2 == idx3 || idx3 == idx1)
            {
                continue;
            }

            outIndices.push_back(idx1);
            outIndices.push_back(idx2);
            outIndices.push_back(idx3);
        }

        // Compact vertex buffer (optional)
        if (collapseCount > 0)
        {
            std::vector<ChunkMesh::Vertex> compactVertices;
            std::vector<int> newIndices(vertices.size(), -1);

            for (size_t i = 0; i < outIndices.size(); i++)
            {
                uint32_t idx = outIndices[i];
                if (newIndices[idx] == -1)
                {
                    newIndices[idx] = compactVertices.size();
                    compactVertices.push_back(outVertices[idx]);
                }
                outIndices[i] = newIndices[idx];
            }

            outVertices = compactVertices;
        }
    }

    void ChunkMesh::calculateAmbientOcclusion(const Chunk& chunk,
                                              const std::array<std::weak_ptr<Chunk>, 6>& neighbors)
    {
        Utility::Profiler::getInstance()->beginSample("ChunkMesh::calculateAmbientOcclusion");

        // Skip if empty
        if (m_vertices.empty())
        {
            Utility::Profiler::getInstance()->endSample();
            return;
        }

        // Get world position of chunk
        int chunkSize = chunk.getSize();
        glm::vec3 chunkWorldPos(
            m_coord.x * chunkSize * VOXEL_SIZE,
            m_coord.y * chunkSize * VOXEL_SIZE,
            m_coord.z * chunkSize * VOXEL_SIZE
        );

        // Get light direction (assuming light comes from above and slightly to the side)
        glm::vec3 primaryLightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));

        // Process each vertex
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(m_vertices.size()); i++)
        {
            Vertex& vertex = m_vertices[i];

            // Get vertex in local space
            glm::vec3 localPos = vertex.position - chunkWorldPos;

            // Add small bias to prevent self-intersection
            glm::vec3 rayOrigin = vertex.position + vertex.normal * AO_RAY_BIAS;

            // Generate cosine-weighted ray directions in hemisphere
            std::vector<glm::vec3> rayDirs = generateHemisphereRays(vertex.normal, AO_RAY_SAMPLES);

            // Calculate occlusion value by ray casting
            float occlusion = 0.0f;
            int hitCount = 0;

            // Add bias toward primary light direction
            glm::vec3 biasedLightDir = glm::normalize(
                primaryLightDir + vertex.normal * 0.5f); // Blend with normal
            rayDirs[0] = biasedLightDir; // Replace first ray

            // Cast rays and count hits
            for (const auto& rayDir : rayDirs)
            {
                float hitDistance;
                bool hit = raycastVoxel(chunk, neighbors, rayOrigin, rayDir, AO_RAY_LENGTH, hitDistance);

                if (hit)
                {
                    // Weight by distance - closer hits contribute more to occlusion
                    float weight = 1.0f - (hitDistance / AO_RAY_LENGTH);
                    occlusion += weight * weight; // Square for more dramatic fallof
                    hitCount++;
                }
            }

            // Normalize occlusion
            if (hitCount > 0)
            {
                occlusion /= static_cast<float>(rayDirs.size());

                // Add directional component - add more occlusion if normal faces away from light
                float nDotL = glm::max(0.0f, glm::dot(vertex.normal, primaryLightDir));
                float directionalFactor = 1.0f - nDotL * 0.5f; // Scale to [0.5, 1.0]

                // Combine ambient occlusion with directional occlusion
                occlusion = glm::mix(occlusion, directionalFactor, 0.3f);

                // Scale and convert to uint16
                uint16_t occlusionValue = static_cast<uint16_t>(occlusion * 255.0f);

                // Invert for lighting (higher = brighter)
                vertex.occlusion = 255 - occlusionValue;
            }
            else
            {
                // No hits - fully lit
                vertex.occlusion = 255;
            }
        }

        Utility::Profiler::getInstance()->endSample();
    }

    void ChunkMesh::updateBounds()
    {
        int chunkSize = 16; // Default size

        // Calculate bounds based on chunk coordinates
        glm::vec3 minCorner(
            m_coord.x * chunkSize * VOXEL_SIZE,
            m_coord.y * chunkSize * VOXEL_SIZE,
            m_coord.z * chunkSize * VOXEL_SIZE
        );

        glm::vec3 maxCorner = minCorner + glm::vec3(chunkSize * VOXEL_SIZE);

        m_bounds = Utility::AABB(minCorner, maxCorner);
    }

} // namespace PixelCraft::Voxel