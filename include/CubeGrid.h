#pragma once

#include <vector>
#include <glm.hpp>

struct Cube {
    glm::vec3 position;
    glm::vec3 color;
    bool active;

    Cube() : position(0.0f), color(1.0f), active(false) {}
    Cube(glm::vec3 pos, glm::vec3 col) : position(pos), color(col), active(true) {}
};

class CubeGrid
{
private:
    int size;
    float spacing;
    std::vector<std::vector<std::vector<Cube>>> grid;

public:
    CubeGrid(int gridSize, float gridSpacing);
    ~CubeGrid();

    void initialize();
    void update(float deltaTime);

    // Getters
    const std::vector<std::vector<std::vector<Cube>>>& getGrid() const { return grid; }
    int getSize() const { return size; }
    float getSpacing() const { return spacing; }

    // Grid operations
    void setCube(int x, int y, int z, const Cube& cube);
    const Cube& getCube(int x, int y, int z) const;
    bool isCubeActive(int x, int y, int z) const;
};