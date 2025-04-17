#pragma once

#include "GridCore.h"

// Definition of a Cube object
struct Cube
{
    glm::vec3 position;
    glm::vec3 color;
    bool active;

    Cube() : position(0.0f), color(1.0f), active(false)
    {
    }
    Cube(glm::vec3 pos, glm::vec3 col) : position(pos), color(col), active(true)
    {
    }
};

// Specialized chunk for Cubes
class CubeChunk : public GridChunk<Cube>
{
public:
    CubeChunk(const glm::ivec3& position);

    // Override to check cube activity
    bool isCellActive(int localX, int localY, int localZ) const override;
};

// The main CubeGrid class (specialization of Grid<Cube>)
class CubeGrid : public Grid<Cube>
{
public:
    CubeGrid(int initialSize, float gridSpacing);
    ~CubeGrid();

    // Grid operations specialization
    void setCube(int x, int y, int z, const Cube& cube);
    const Cube& getCube(int x, int y, int z) const;
    bool isCubeActive(int x, int y, int z) const;

    // Specialized queries for voxel operations
    std::vector<Cube> getVisibleCubesInFrustum(const glm::mat4& viewProj) const;

    // Improved implementation with Cube-specific optimizations
    std::vector<std::tuple<glm::ivec3, Cube>> querySphere(const glm::vec3& center, float radius) const override;

    // Create initial grid with a basic floor
    void createInitialFloor(int size);

protected:
    // Create the specialized chunk type
    GridChunk<Cube>* createChunk(const glm::ivec3& position) override;
};