#include "CubeGrid.h"

CubeGrid::CubeGrid(int gridSize, float gridSpacing) : size(gridSize), spacing(gridSpacing)
{
    initialize();
}

CubeGrid::~CubeGrid()
{
    // Clean up if needed
}

void CubeGrid::initialize()
{
    grid.resize(size);
    for (int x = 0; x < size; x++)
    {
        grid[x].resize(size);
        for (int y = 0; y < size; y++)
        {
            grid[x][y].resize(size);
            for (int z = 0; z < size; z++)
            {
                // Center the grid around origin
                glm::vec3 position = glm::vec3(
                    (x - size / 2.0f) * spacing,
                    (y - size / 2.0f) * spacing,
                    (z - size / 2.0f) * spacing
                );

                // Default to inactive
                grid[x][y][z] = Cube(position, glm::vec3(0.8f, 0.8f, 0.8f));
                grid[x][y][z].active = false;

                // Create base floor platform
                if (y == 0)
                {
                    grid[x][y][z].active = true;
                    grid[x][y][z].color = glm::vec3(0.9f, 0.9f, 0.9f);
                }

                // Create some columns for shadow testing
                if ((x == size / 4 || x == size / 2 || x == 3 * size / 4) &&
                    (z == size / 4 || z == size / 2 || z == 3 * size / 4) &&
                    y > 0 && y < size / 3)
                {
                    grid[x][y][z].active = true;
                    // Make columns different colors for better visualization
                    if (x == size / 4) grid[x][y][z].color = glm::vec3(0.8f, 0.2f, 0.2f);
                    else if (x == size / 2) grid[x][y][z].color = glm::vec3(0.2f, 0.8f, 0.2f);
                    else grid[x][y][z].color = glm::vec3(0.2f, 0.2f, 0.8f);
                }

                // Create a wall for shadow casting
                if (x == size / 3 && z > size / 3 && z < 2 * size / 3 && y > 0 && y < size / 4)
                {
                    grid[x][y][z].active = true;
                    grid[x][y][z].color = glm::vec3(0.8f, 0.8f, 0.2f);
                }

                // Create an overhang/floating platform
                if (y == size / 5 && x > size / 2 && x < 3 * size / 4 && z > size / 2 && z < 3 * size / 4)
                {
                    grid[x][y][z].active = true;
                    grid[x][y][z].color = glm::vec3(0.2f, 0.8f, 0.8f);
                }

                // Create a staircase for testing shadows on different heights
                if (z == 3 * size / 4 && x > size / 2 && x < 3 * size / 4 && y > 0 && y <= (x - size / 2))
                {
                    grid[x][y][z].active = true;
                    grid[x][y][z].color = glm::vec3(0.7f, 0.4f, 0.7f);
                }
            }
        }
    }
}

void CubeGrid::update(float deltaTime)
{
    // Add any grid update logic here
    // This could include animations, cube creation/destruction, etc.
}

void CubeGrid::setCube(int x, int y, int z, const Cube& cube)
{
    if (x >= 0 && x < size && y >= 0 && y < size && z >= 0 && z < size)
    {
        grid[x][y][z] = cube;
    }
}

const Cube& CubeGrid::getCube(int x, int y, int z) const
{
    static Cube defaultCube;
    if (x >= 0 && x < size && y >= 0 && y < size && z >= 0 && z < size)
    {
        return grid[x][y][z];
    }
    return defaultCube;
}

bool CubeGrid::isCubeActive(int x, int y, int z) const
{
    if (x >= 0 && x < size && y >= 0 && y < size && z >= 0 && z < size)
    {
        return grid[x][y][z].active;
    }
    return false;
}