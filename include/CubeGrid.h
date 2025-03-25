#pragma once

#include <algorithm>
#include <cmath>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <glm.hpp>

struct Cube {
    glm::vec3 position;
    glm::vec3 color;
    bool active;

    Cube() : position(0.0f), color(1.0f), active(false) {}
    Cube(glm::vec3 pos, glm::vec3 col) : position(pos), color(col), active(true) {}
};

// Hash function for vec3 coordinates
struct Vec3Hash {
    size_t operator()(const glm::ivec3& v) const
    {
        return std::hash<int>()(v.x) ^
               std::hash<int>()(v.y) ^
               std::hash<int>()(v.z);
    }
};

// Equality operator for map key
struct Vec3Equal {
    bool operator()(const glm::ivec3& lhs, const glm::ivec3& rhs) const
    {
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }
};

// Forward declaration
class CubeGrid;

// A fixed-size chunk of the grid
class GridChunk
{
public:
    static const int CHUNK_SIZE = 16;

    GridChunk(const glm::ivec3& position);

    void setCube(int localX, int localY, int localZ, const Cube& cube);
    const Cube& getCube(int localX, int localY, int localZ) const;
    bool hasAnyCubes() const; // Check if any cube is active

    // Get all chunks that would be affected by an operation at given position with radius
    std::vector<glm::ivec3> getAffectedChunks(const glm::vec3& worldPos, float radius) const;

    // Accessors
    const glm::ivec3& getPosition() const { return chunkPosition; }
    bool isActive() const { return active; }

private:
    Cube cubes[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
    glm::ivec3 chunkPosition; // Position in chunk coordinates
    bool active; // Is this chunk loaded/used
    
    friend class CubeGrid;
};

// The main grid class using a chunk-based system
class CubeGrid
{
public:
    CubeGrid(int initialSize, float gridSpacing);
    ~CubeGrid();

    // Initialize and update
    void clear();
    void update(float deltaTime);

    // Grid operations
    void setCube(int x, int y, int z, const Cube& cube);
    const Cube& getCube(int x, int y, int z) const;
    bool isCubeActive(int x, int y, int z) const;

    // Chunk management
    GridChunk* getOrCreateChunk(const glm::ivec3& chunkPos);
    void updateLoadedChunks(const glm::ivec3& centerGridPos, int viewDistance);

    // Coordinate conversion
    glm::vec3 calculatePosition(int x, int y, int z) const;
    glm::ivec3 worldToGridCoordinates(const glm::vec3& worldPos) const;

    // Bounds tracking
    void expandBounds(const glm::ivec3& pos);
    const glm::ivec3& getMinBounds() const { return minBounds; }
    const glm::ivec3& getMaxBounds() const { return maxBounds; }

    // Statistics
    int getActiveChunkCount() const;
    int getTotalActiveCubeCount() const;

    // Accessors
    float getSpacing() const { return spacing; }
    const std::unordered_map<glm::ivec3, GridChunk*, Vec3Hash>& getChunks() const { return chunks; }

private:
    float spacing;
    std::unordered_map<glm::ivec3, GridChunk*, Vec3Hash> chunks;
    glm::ivec3 minBounds, maxBounds;
};