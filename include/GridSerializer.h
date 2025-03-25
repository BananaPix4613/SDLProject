#pragma once

#include "CubeGrid.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

using json = nlohmann::json;

class GridSerializer
{
public:
    // Save grid to file (JSON format)
    static bool saveGridToFile(const CubeGrid* grid, const std::string& filename)
    {
        try
        {
            json gridData;

            // Save metadata
            gridData["version"] = 1;
            gridData["spacing"] = grid->getSpacing();

            // Save active cubes only
            json cubesArray = json::array();

            // Get bounds for optimization
            glm::ivec3 minBounds = grid->getMinBounds();
            glm::ivec3 maxBounds = grid->getMaxBounds();

            // For chunks-based system, iterate through active chunks
            for (int x = minBounds.x; x <= maxBounds.x; x++)
            {
                for (int y = minBounds.y; y <= maxBounds.y; y++)
                {
                    for (int z = minBounds.z; z <= maxBounds.z; z++)
                    {
                        if (grid->isCubeActive(x, y, z))
                        {
                            const Cube& cube = grid->getCube(x, y, z);

                            json cubeData;
                            cubeData["pos"] = {x, y, z};
                            cubeData["color"] = {cube.color.r, cube.color.g, cube.color.b};

                            cubesArray.push_back(cubeData);
                        }
                    }
                }
            }

            gridData["cubes"] = cubesArray;

            // Write to file
            std::ofstream file(filename);
            if (!file.is_open())
            {
                std::cerr << "Failed to open file for writing: " << filename << std::endl;
                return false;
            }

            file << std::setw(4) << gridData << std::endl;
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error saving grid: " << e.what() << std::endl;
            return false;
        }
    }

    // Load grid from file
    static bool loadGridFromFile(CubeGrid* grid, const std::string& filename)
    {
        try
        {
            std::ifstream file(filename);
            if (!file.is_open())
            {
                std::cerr << "Failed to open file for reading: " << filename << std::endl;
                return false;
            }

            json gridData;
            file >> gridData;

            // Verify version
            int version = gridData["version"];
            if (version != 1)
            {
                std::cerr << "Unsupported grid file version: " << version << std::endl;
                return false;
            }

            // Clear existing grid
            grid->clear();

            // Set grid properties
            float spacing = gridData["spacing"];
            // (grid spacing is typically set in constructor, but could be updated if needed)

            // Load cubes
            for (const auto& cubeData : gridData["cubes"])
            {
                int x = cubeData["pos"][0];
                int y = cubeData["pos"][1];
                int z = cubeData["pos"][2];

                glm::vec3 color(
                    cubeData["color"][0],
                    cubeData["color"][1],
                    cubeData["color"][2]
                );

                glm::vec3 position = grid->calculatePosition(x, y, z);
                Cube cube(position, color);
                cube.active = true;
                grid->setCube(x, y, z, cube);
            }

            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error loading grid: " << e.what() << std::endl;
            return false;
        }
    }

    // For binary format, faster but less human-readable
    static bool saveGridToBinary(const CubeGrid* grid, const std::string& filename)
    {
        try
        {
            std::ofstream file(filename, std::ios::binary);
            if (!file.is_open())
            {
                std::cerr << "Failed to open file for binary writing: " << filename << std::endl;
                return false;
            }

            // Write header
            uint32_t version = 1;
            file.write(reinterpret_cast<char*>(&version), sizeof(version));

            float spacing = grid->getSpacing();
            file.write(reinterpret_cast<char*>(&spacing), sizeof(spacing));

            // Get bounds for iteration
            glm::ivec3 minBounds = grid->getMinBounds();
            glm::ivec3 maxBounds = grid->getMaxBounds();

            // Count active cubes first
            uint32_t activeCubeCount = 0;
            for (int x = minBounds.x; x <= maxBounds.x; x++)
            {
                for (int y = minBounds.y; y <= maxBounds.y; y++)
                {
                    for (int z = minBounds.z; z <= maxBounds.z; z++)
                    {
                        if (grid->isCubeActive(x, y, z))
                        {
                            activeCubeCount++;
                        }
                    }
                }
            }

            // Write active cube count
            file.write(reinterpret_cast<char*>(&activeCubeCount), sizeof(activeCubeCount));

            // Write each active cube
            for (int x = minBounds.x; x <= maxBounds.x; x++)
            {
                for (int y = minBounds.y; y <= maxBounds.y; y++)
                {
                    for (int z = minBounds.z; z <= maxBounds.z; z++)
                    {
                        if (grid->isCubeActive(x, y, z))
                        {
                            const Cube& cube = grid->getCube(x, y, z);

                            // Position
                            int32_t pos[3] = {x, y, z};
                            file.write(reinterpret_cast<char*>(pos), sizeof(pos));

                            // Color
                            float color[3] = {cube.color.r, cube.color.g, cube.color.b};
                            file.write(reinterpret_cast<char*>(color), sizeof(color));
                        }
                    }
                }
            }

            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error saving grid to binary: " << e.what() << std::endl;
            return false;
        }
    }

    static bool loadGridFromBinary(CubeGrid* grid, const std::string& filename)
    {
        try
        {
            std::ifstream file(filename, std::ios::binary);
            if (!file.is_open())
            {
                std::cerr << "Failed to open file for binary reading: " << filename << std::endl;
                return false;
            }

            // Read header
            uint32_t version;
            file.read(reinterpret_cast<char*>(&version), sizeof(version));
            if (version != 1)
            {
                std::cerr << "Unsupported binary grid file version: " << version << std::endl;
                return false;
            }

            // Read spacing
            float spacing;
            file.read(reinterpret_cast<char*>(&spacing), sizeof(spacing));

            // Clear existing grid
            grid->clear();

            // Read cube count
            uint32_t cubeCount;
            file.read(reinterpret_cast<char*>(&cubeCount), sizeof(cubeCount));

            // Read each cube
            for (uint32_t i = 0; i < cubeCount; i++)
            {
                // Position
                int32_t pos[3];
                file.read(reinterpret_cast<char*>(pos), sizeof(pos));

                // Color
                float color[3];
                file.read(reinterpret_cast<char*>(color), sizeof(color));

                // Create and add cube
                glm::vec3 position = grid->calculatePosition(pos[0], pos[1], pos[2]);
                Cube cube(position, glm::vec3(color[0], color[1], color[2]));
                cube.active = true;
                grid->setCube(pos[0], pos[1], pos[2], cube);
            }

            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error loading grid from binary: " << e.what() << std::endl;
            return false;
        }
    }
};