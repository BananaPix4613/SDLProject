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

                // Example pattern - modify as needed
                bool shouldCreateCube = false;

                // Create some interesting pattern
                if ((x + y + z) % 3 == 0 || (x == 0 && y == 0) || (y == 0 && z == 0) || (x == 0 && z == 0))
                {
                    shouldCreateCube = true;
                }

                if (shouldCreateCube)
                {
                    // Create gradient colors based on position
                    float r = static_cast<float>(x) / size;
                    float g = static_cast<float>(y) / size;
                    float b = static_cast<float>(z) / size;
                    glm::vec3 color = glm::vec3(r, g, b);

                    grid[x][y][z] = Cube(position, color);
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