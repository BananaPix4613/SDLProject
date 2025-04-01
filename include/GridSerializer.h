#pragma once

#include "CubeGrid.h"
#include "FileDialog.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class GridSerializer
{
public:
    // Save grid to binary file using a file dialog
    static bool saveGridToFile(const CubeGrid* grid, HWND ownerWindow = NULL)
    {
        try
        {
            // Open save file dialog
            std::string filename = FileDialog::saveFile("Binary Grid Files (*.bin)\0*.bin\0", ownerWindow);

            // User cancelled
            if (filename.empty())
            {
                return false;
            }

            // Ensure .bin extension
            if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".bin")
            {
                filename += ".bin";
            }

            return saveGridToBinary(grid, filename);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error saving grid: " << e.what() << std::endl;
            return false;
        }
    }

    // Load grid from binary file using a file dialog
    static bool loadGridFromFile(CubeGrid* grid, HWND ownerWindow = NULL)
    {
        try
        {
            // Open file dialog
            std::string filename = FileDialog::openFile("Binary Grid Files (*.bin)\0*.bin\0", ownerWindow);

            // User cancelled
            if (filename.empty())
            {
                return false;
            }

            return loadGridFromBinary(grid, filename);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error loading grid: " << e.what() << std::endl;
            return false;
        }
    }

    // Binary format implementation
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

            // Write header and version
            const char* header = "CUBEGRID";
            file.write(header, 8);

            uint32_t version = 2; // Increment version number
            file.write(reinterpret_cast<char*>(&version), sizeof(version));

            // Write grid properties
            float spacing = grid->getSpacing();
            file.write(reinterpret_cast<char*>(&spacing), sizeof(spacing));

            // Get bounds for iteration
            glm::ivec3 minBounds = grid->getMinBounds();
            glm::ivec3 maxBounds = grid->getMaxBounds();

            // Write grid bounds
            file.write(reinterpret_cast<const char*>(&minBounds), sizeof(minBounds));
            file.write(reinterpret_cast<const char*>(&maxBounds), sizeof(maxBounds));

            // Count active cubes first
            uint32_t activeCubeCount = grid->getTotalActiveCubeCount();

            // Write active cube count
            file.write(reinterpret_cast<char*>(&activeCubeCount), sizeof(activeCubeCount));

            // Count of active cubes written
            uint32_t cubesWritten = 0;

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

                            cubesWritten++;
                        }
                    }
                }
            }

            // Verify that we wrote the correct number of cubes
            if (cubesWritten != activeCubeCount)
            {
                std::cerr << "Warning: Active cube count mismatch. Expected: " << activeCubeCount << ", Wrote: " << cubesWritten << std::endl;
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

            // Read and verify header
            char header[9] = { 0 };
            file.read(header, 8);
            if (std::string(header) != "CUBEGRID")
            {
                std::cerr << "Invalid file format: not a CUBEGRID file" << std::endl;
                return false;
            }

            // Read version
            uint32_t version;
            file.read(reinterpret_cast<char*>(&version), sizeof(version));

            // Handle different versions
            if (version < 1 || version > 2)
            {
                std::cerr << "Unsupported binary grid file version: " << version << std::endl;
                return false;
            }

            // Read spacing
            float spacing;
            file.read(reinterpret_cast<char*>(&spacing), sizeof(spacing));

            // Clear existing grid
            grid->clear();

            // Read bounds if version >= 2
            glm::ivec3 minBounds = glm::ivec3(0.0f), maxBounds = glm::ivec3(0.0f);
            if (version >= 2)
            {
                file.read(reinterpret_cast<char*>(&minBounds), sizeof(minBounds));
                file.read(reinterpret_cast<char*>(&maxBounds), sizeof(maxBounds));
            }

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

            // Print stats about the loaded world
            std::cout << "Loaded world with " << cubeCount << " cubes" << std::endl;
            if (version >= 2)
            {
                std::cout << "World bounds: (" << minBounds.x << ", " << minBounds.y << ", " << minBounds.z << ") to ("
                          << maxBounds.x << ", " << maxBounds.y << ", " << maxBounds.z << ")" << std::endl;
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