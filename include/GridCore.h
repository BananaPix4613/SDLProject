#pragma once

#include <algorithm>
#include <cmath>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <glm.hpp>
#include <gtx/norm.hpp>
#include <memory>
#include <functional>

// Hash function for vec3 coordinates
struct Vec3Hash
{
    size_t operator()(const glm::ivec3& v) const
    {
        return std::hash<int>()(v.x) ^
            (std::hash<int>()(v.y) << 1) ^
            (std::hash<int>()(v.z) << 2);
    }
};

// Equality operator for map key
struct Vec3Equal
{
    bool operator()(const glm::ivec3& lhs, const glm::ivec3& rhs) const
    {
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }
};

// Forward declarations
template <typename T>
class Grid;

// A fixed-size chunk of the grid
template <typename T>
class GridChunk
{
public:
    static const int CHUNK_SIZE = 16;

    // Constructor and core functions
    GridChunk(const glm::ivec3& position);
    virtual ~GridChunk() = default;

    // Cell operations
    void setCell(int localX, int localY, int localZ, const T& value);
    const T& getCell(int localX, int localY, int localZ) const;

    // Custom activity checker (specialized by derived classes)
    virtual bool isCellActive(int localX, int localY, int localZ) const;

    // Check if any cells are active in this chunk
    bool hasAnyCells() const;
    
    // Get all chunks that would be affected by an operation at given position with radius
    std::vector<glm::ivec3> getAffectedChunks(const glm::vec3& worldPos, float radius) const;

    // Accessors
    const glm::ivec3& getPosition() const
    {
        return m_chunkPosition;
    }
    bool isActive() const
    {
        return m_active;
    }

protected:
    std::vector<T> m_cells;
    glm::ivec3 m_chunkPosition; // Position in chunk coordinates
    bool m_active; // Is this chunk loaded/used

    // Helper to convert 3D coordinates to array index
    int coordsToIndex(int x, int y, int z) const;

    friend class Grid<T>;
};

// Base grid class using a chunk-based system
template <typename T>
class Grid
{
public:
    Grid(float gridSpacing);
    virtual ~Grid();

    // Initialize and update
    void clear();
    virtual void update(float deltaTime);

    // Grid operations
    virtual void setCell(int x, int y, int z, const T& value);
    const T& getCell(int x, int y, int z) const;
    virtual bool isCellActive(int x, int y, int z) const;

    // Chunk management
    GridChunk<T>* getOrCreateChunk(const glm::ivec3& chunkPos);
    void updateLoadedChunks(const glm::ivec3& centerGridPos, int viewDistance);
    bool unloadChunk(const glm::ivec3& chunkPos);

    // Coordinate conversion
    glm::vec3 gridToWorldPosition(int x, int y, int z) const;
    glm::ivec3 worldToGridCoordinates(const glm::vec3& worldPos) const;

    // Bounds tracking
    void expandBounds(const glm::ivec3& pos);
    const glm::ivec3& getMinBounds() const
    {
        return m_minBounds;
    }
    const glm::ivec3& getMaxBounds() const
    {
        return m_maxBounds;
    }

    // Queries
    virtual std::vector<std::tuple<glm::ivec3, T>> querySphere(const glm::vec3& center, float radius) const;
    virtual std::vector<std::tuple<glm::ivec3, T>> queryBox(const glm::vec3& min, const glm::vec3& max) const;

    // Iterator/visitor pattern
    void forEachCell(const std::function<void(const glm::ivec3&, const T&)>& func) const;
    void forEachActiveCell(const std::function<void(const glm::ivec3&, const T&)>& func) const;

    // Statistics
    int getActiveChunkCount() const;
    int getTotalActiveCellCount() const;

    // Accessors
    float getSpacing() const
    {
        return m_spacing;
    }
    const std::unordered_map<glm::ivec3, GridChunk<T>*, Vec3Hash>& getChunks() const
    {
        return m_chunks;
    }

protected:
    // Create a new chunk at the specified position
    virtual GridChunk<T>* createChunk(const glm::ivec3& position);

    float m_spacing;
    std::unordered_map<glm::ivec3, GridChunk<T>*, Vec3Hash> m_chunks;
    glm::ivec3 m_minBounds, m_maxBounds;
    T m_defaultValue;
};

// Include implementation in header since this is a template class
#include "GridCore.inl"