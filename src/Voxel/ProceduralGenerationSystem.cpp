// -------------------------------------------------------------------------
// ProceduralGenerationSystem.cpp
// -------------------------------------------------------------------------
#include "Voxel/ProceduralGenerationSystem.h"
#include "Voxel/NoiseGenerator.h"
#include "Voxel/GenerationParameters.h"
#include "Voxel/FeaturePlacement.h"
#include "Voxel/DistributionControl.h"
#include "Voxel/BiomeManager.h"
#include "Voxel/Voxel.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Utility/FileSystem.h"
#include "Utility/Random.h"
#include "Utility/Math.h"
#include "ECS/Serializer.h"
#include "ECS/Deserializer.h"
#include <glm/gtx/norm.hpp>
#include <Voxel/ChunkManager.h>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Voxel
{

    ProceduralGenerationSystem::ProceduralGenerationSystem() : m_shutdownThread(false)
    {
        // Constructor implementation
    }

    ProceduralGenerationSystem::~ProceduralGenerationSystem()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool ProceduralGenerationSystem::initialize()
    {
        if (m_initialized)
        {
            Log::warn("ProceduralGenerationSystem already initialized");
            return true;
        }

        Log::info("Initializing ProceduralGenerationSystem subsystem");

        // Create biome manager
        m_biomeManager = std::make_shared<BiomeManager>();

        // Create a default noise generator
        auto defaultNoise = createNoiseGenerator("default");
        if (!defaultNoise)
        {
            Log::error("Failed to create default noise generator");
            return false;
        }

        // Create default terrain noise generator
        auto terrainNoise = createNoiseGenerator("terrain");
        if (terrainNoise)
        {
            terrainNoise->presetTerrain();
        }

        // Create default cave noise generator
        auto caveNoise = createNoiseGenerator("caves");
        if (caveNoise)
        {
            caveNoise->presetCaves();
        }

        // Create default feature placement
        auto defaultPlacement = createFeaturePlacement("default");
        if (!defaultPlacement)
        {
            Log::error("Failed to create default feature placement");
            return false;
        }

        // Create default distribution control
        auto defaultDistribution = createDistributionControl("default");
        if (!defaultDistribution)
        {
            Log::error("Failed to create default distribution control");
            return false;
        }

        // Create default generation parameters
        auto defaultParams = createParameters("default");
        if (!defaultParams)
        {
            Log::error("Failed to create default generation parameters");
            return false;
        }

        // Set active parameters
        if (!setActiveParameters("default"))
        {
            Log::error("Failed to set active parameters");
            return false;
        }

        // Set default seeds
        setSeed(12345);

        // Start generation thread
        m_generationThread = std::thread(&ProceduralGenerationSystem::processGenerationQueue, this);

        m_initialized = true;
        return true;
    }

    void ProceduralGenerationSystem::update(float deltaTime)
    {
        if (!m_initialized)
        {
            return;
        }

        // Process any immediate generation tasks
    }

    void ProceduralGenerationSystem::render()
    {
        if (!m_initialized)
        {
            return;
        }

        // Render debug visualization if needed
    }

    void ProceduralGenerationSystem::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Log::info("Shutting down ProceduralGenerationSystem subsystem");

        // Signal generation thread to stop
        m_shutdownThread = true;

        // Join the generation thread
        if (m_generationThread.joinable())
        {
            m_generationThread.join();
        }

        // Clear all generation tasks
        {
            std::lock_guard<std::mutex> lock(m_taskMutex);
            m_generationTasks.clear();
        }

        // Clear all parameters
        m_parameters.clear();
        m_activeParametersId.clear();

        // Clear all generators
        m_noiseGenerators.clear();
        m_featurePlacements.clear();
        m_distributionControls.clear();

        // Clear biome manager
        m_biomeManager.reset();

        // Clear feature generators
        m_featureGenerators.clear();

        m_initialized = false;
    }

    std::vector<std::string> ProceduralGenerationSystem::getDependencies() const
    {
        return {"ChunkManager"};
    }

    bool ProceduralGenerationSystem::generateChunk(const ChunkCoord& coord, Chunk& chunk, const std::string& parametersId)
    {
        if (!m_initialized || m_paused)
        {
            return false;
        }

        // Create generation context
        GenerationContext context = createContext(coord, parametersId.empty() ? m_activeParametersId : parametersId);

        // Check if context is valid
        if (!context.parameters)
        {
            Log::error("Invalid generation parameters for chunk " + coord.toString());
            return false;
        }

        // Generate terrain
        if (!generateTerrainForChunk(context, chunk))
        {
            Log::error("Failed to generate terrain for chunk " + coord.toString());
            return false;
        }

        // Generate biomes
        if (!generateBiomesForChunk(context, chunk))
        {
            Log::error("Failed to generate biomes for chunk " + coord.toString());
            // Continue anyway
        }

        // Generate features
        if (!generateFeaturesForChunk(context, chunk))
        {
            Log::error("Failed to generate features for chunk " + coord.toString());
            // Continue anyway
        }

        return true;
    }

    int ProceduralGenerationSystem::generateRegion(const ChunkCoord& min, const ChunkCoord& max, Grid& grid, const std::string& parametersId)
    {
        if (!m_initialized || m_paused)
        {
            return 0;
        }

        int generatedCount = 0;

        // Iterate through all chunks in the region
        for (int x = min.x; x <= max.x; x++)
        {
            for (int y = min.y; y <= max.y; y++)
            {
                for (int z = min.z; z <= max.z; z++)
                {
                    ChunkCoord coord(x, y, z);

                    // Get or create the chunk
                    auto chunk = grid.getChunk(coord);
                    if (!chunk)
                    {
                        chunk = grid.createChunk(coord);
                    }

                    if (!chunk)
                    {
                        Log::error("Failed to create chunk at " + coord.toString());
                        continue;
                    }

                    // Generate the chunk
                    if (generateChunk(coord, *chunk, parametersId))
                    {
                        generatedCount++;

                        // Update neighbor chunks
                        updateNeighborChunks(coord, grid);
                    }
                }
            }
        }

        return generatedCount;
    }

    int ProceduralGenerationSystem::generateAroundPoint(const glm::vec3& center, float radius, Grid& grid, const std::string& parametersId)
    {
        if (!m_initialized || m_paused)
        {
            return 0;
        }

        int chunkSize = grid.getChunkSize();
        if (chunkSize <= 0)
        {
            Log::error("Invalid chunk size");
            return 0;
        }

        // Calculate the chunk coordinates range
        ChunkCoord centerChunk = ChunkCoord::fromWorldPosition(center, chunkSize);
        int chunkRadius = static_cast<int>(std::ceil(radius / chunkSize)) + 1;

        ChunkCoord min(
            centerChunk.x - chunkRadius,
            centerChunk.y - chunkRadius,
            centerChunk.z - chunkRadius
        );

        ChunkCoord max(
            centerChunk.x + chunkRadius,
            centerChunk.y + chunkRadius,
            centerChunk.z + chunkRadius
        );

        int generatedCount = 0;

        // Iterate through all chunks in the range
        for (int x = min.x; x <= max.x; x++)
        {
            for (int y = min.y; y <= max.y; y++)
            {
                for (int z = min.z; z <= max.z; z++)
                {
                    ChunkCoord coord(x, y, z);

                    // Calculate chunk center in world space
                    glm::vec3 chunkCenter = coord.toWorldPosition(chunkSize) + glm::vec3(chunkSize / 2.0f);

                    // Check if chunk is within the radius
                    float distanceSquared = glm::distance2(center, chunkCenter);
                    if (distanceSquared > (radius + chunkSize) * (radius + chunkSize))
                    {
                        continue;
                    }

                    // Get or create the chunk
                    auto chunk = grid.getChunk(coord);
                    if (!chunk)
                    {
                        chunk = grid.createChunk(coord);
                    }

                    if (!chunk)
                    {
                        Log::error("Failed to create chunk at " + coord.toString());
                        continue;
                    }

                    // Generate the chunk
                    if (generateChunk(coord, *chunk, parametersId))
                    {
                        generatedCount++;

                        // Update neighbor chunks
                        updateNeighborChunks(coord, grid);
                    }
                }
            }
        }

        return generatedCount;
    }

    std::shared_ptr<GenerationParameters> ProceduralGenerationSystem::createParameters(const std::string& id)
    {
        if (id.empty())
        {
            Log::error("Parameter ID cannot be empty");
            return nullptr;
        }

        // Check if parameters already exist
        auto it = m_parameters.find(id);
        if (it != m_parameters.end())
        {
            Log::warn("Generation parameters '" + id + "' already exist");
            return it->second;
        }

        // Create new parameters
        auto params = std::make_shared<GenerationParameters>();
        m_parameters[id] = params;

        // Set default values
        params->setTerrainNoiseLayer("terrain");
        params->setCaveNoiseLayer("caves");

        return params;
    }

    std::shared_ptr<GenerationParameters> ProceduralGenerationSystem::getParameters(const std::string& id) const
    {
        auto it = m_parameters.find(id);
        if (it != m_parameters.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool ProceduralGenerationSystem::setActiveParameters(const std::string& id)
    {
        auto params = getParameters(id);
        if (!params)
        {
            Log::error("Generation parameters '" + id + "' not found");
            return false;
        }

        m_activeParametersId = id;
        return true;
    }

    std::shared_ptr<GenerationParameters> ProceduralGenerationSystem::getActiveParameters() const
    {
        return getParameters(m_activeParametersId);
    }

    std::shared_ptr<NoiseGenerator> ProceduralGenerationSystem::createNoiseGenerator(const std::string& id)
    {
        if (id.empty())
        {
            Log::error("Noise generator ID cannot be empty");
            return nullptr;
        }

        // Check if generator already exists
        auto it = m_noiseGenerators.find(id);
        if (it != m_noiseGenerators.end())
        {
            Log::warn("Noise generator '" + id + "' already exists");
            return it->second;
        }

        // Create new generator
        auto generator = std::make_shared<NoiseGenerator>();
        m_noiseGenerators[id] = generator;

        return generator;
    }

    std::shared_ptr<NoiseGenerator> ProceduralGenerationSystem::getNoiseGenerator(const std::string& id) const
    {
        auto it = m_noiseGenerators.find(id);
        if (it != m_noiseGenerators.end())
        {
            return it->second;
        }
        return nullptr;
    }

    std::shared_ptr<FeaturePlacement> ProceduralGenerationSystem::createFeaturePlacement(const std::string& id)
    {
        if (id.empty())
        {
            Log::error("Feature placement ID cannot be empty");
            return nullptr;
        }

        // Check if placement already exists
        auto it = m_featurePlacements.find(id);
        if (it != m_featurePlacements.end())
        {
            Log::warn("Feature placement '" + id + "' already exists");
            return it->second;
        }

        // Create new placement
        auto placement = std::make_shared<FeaturePlacement>();
        m_featurePlacements[id] = placement;

        return placement;
    }

    std::shared_ptr<FeaturePlacement> ProceduralGenerationSystem::getFeaturePlacement(const std::string& id) const
    {
        auto it = m_featurePlacements.find(id);
        if (it != m_featurePlacements.end())
        {
            return it->second;
        }
        return nullptr;
    }

    std::shared_ptr<DistributionControl> ProceduralGenerationSystem::createDistributionControl(const std::string& id)
    {
        if (id.empty())
        {
            Log::error("Distribution control ID cannot be empty");
            return nullptr;
        }

        // Check if control already exists
        auto it = m_distributionControls.find(id);
        if (it != m_distributionControls.end())
        {
            Log::warn("Distribution control '" + id + "' already exists");
            return it->second;
        }

        // Create new control
        auto control = std::make_shared<DistributionControl>();
        m_distributionControls[id] = control;

        return control;
    }

    std::shared_ptr<DistributionControl> ProceduralGenerationSystem::getDistributionControl(const std::string& id) const
    {
        auto it = m_distributionControls.find(id);
        if (it != m_distributionControls.end())
        {
            return it->second;
        }
        return nullptr;
    }

    std::shared_ptr<GenerationParameters> ProceduralGenerationSystem::loadParameters(const std::string& path, const std::string& id)
    {
        if (path.empty() || id.empty())
        {
            Log::error("Path or ID cannot be empty");
            return nullptr;
        }

        // Create parameters object
        auto params = createParameters(id);
        if (!params)
        {
            return nullptr;
        }

        // Ensure file exists
        if (!Utility::FileSystem::fileExists(path))
        {
            Log::debug("Chunk file does not exist: " + path);
            return false;
        }

        // Load from file
        try
        {
            // Open file for reading
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open())
            {
                Log::error("Failed to open chunk file for reading: " + path);
                return false;
            }

            // Create a deserializer
            ECS::Deserializer deserializer(file);

            // Deserialize the params
            params->deserialize(deserializer);

            Log::info("Loaded generation parameters from " + path);
            return params;
        }
        catch (const std::exception& e)
        {
            Log::error("Failed to load parameters from " + path + ": " + e.what());
            m_parameters.erase(id);
            return nullptr;
        }
    }

    bool ProceduralGenerationSystem::saveParameters(const std::string& id, const std::string& path)
    {
        if (path.empty() || id.empty())
        {
            Log::error("Path or ID cannot be empty");
            return false;
        }

        // Get parameters
        auto params = getParameters(id);
        if (!params)
        {
            Log::error("Generation parameters '" + id + "' not found");
            return false;
        }

        // Save to file
        try
        {
            // Open the file
            std::ofstream file(path, std::ios::binary);
            if (!file.is_open())
            {
                Log::error("Failed to open file for chunk serialization: " + path);
                return false;
            }

            // Create a serializer
            ECS::Serializer serializer(file);

            // Serialize the params
            params->serialize(serializer);

            Log::info("Saved generation parameters to " + path);
            return true;
        }
        catch (const std::exception& e)
        {
            Log::error("Failed to save parameters to " + path + ": " + e.what());
            return false;
        }
    }

    std::shared_ptr<BiomeManager> ProceduralGenerationSystem::getBiomeManager() const
    {
        return m_biomeManager;
    }

    uint32_t ProceduralGenerationSystem::getSeed() const
    {
        return m_seed;
    }

    void ProceduralGenerationSystem::setSeed(uint32_t seed)
    {
        m_seed = seed;

        // Update seeds in all systems
        for (auto& [id, generator] : m_noiseGenerators)
        {
            generator->setSeed(seed + std::hash<std::string>{}(id));
        }

        for (auto& [id, placement] : m_featurePlacements)
        {
            placement->setSeed(seed + std::hash<std::string>{}(id));
        }

        for (auto& [id, control] : m_distributionControls)
        {
            control->setSeed(seed + std::hash<std::string>{}(id));
        }
    }

    bool ProceduralGenerationSystem::registerFeatureGenerator(const std::string& name,
                                                              std::function<bool(const GenerationContext&, Chunk&)> generator)
    {
        if (name.empty() || !generator)
        {
            Log::error("Feature generator name or function cannot be empty");
            return false;
        }

        // Check if already registered
        if (m_featureGenerators.find(name) != m_featureGenerators.end())
        {
            Log::warn("Feature generator '" + name + "' already registered");
            return false;
        }

        m_featureGenerators[name] = generator;
        return true;
    }

    std::function<bool(const GenerationContext&, Chunk&)> ProceduralGenerationSystem::getFeatureGenerator(const std::string& name) const
    {
        auto it = m_featureGenerators.find(name);
        if (it != m_featureGenerators.end())
        {
            return it->second;
        }
        return nullptr;
    }

    int ProceduralGenerationSystem::preGenerateChunks(const ChunkCoord& min, const ChunkCoord& max, int priority)
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);

        // Create a new task
        GenerationTask task;
        task.id = m_nextTaskId++;
        task.min = min;
        task.max = max;
        task.parametersId = m_activeParametersId;
        task.priority = priority;
        task.complete = false;
        task.canceled = false;

        m_generationTasks.push_back(task);

        return task.id;
    }

    bool ProceduralGenerationSystem::isPreGenerationComplete(int taskId)
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);

        for (const auto& task : m_generationTasks)
        {
            if (task.id == taskId)
            {
                return task.complete;
            }
        }

        // If task not found, assume it's complete
        return true;
    }

    bool ProceduralGenerationSystem::cancelPreGeneration(int taskId)
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);

        for (auto& task : m_generationTasks)
        {
            if (task.id == taskId && !task.complete)
            {
                task.canceled = true;
                return true;
            }
        }

        return false;
    }

    void ProceduralGenerationSystem::setPaused(bool paused)
    {
        m_paused = paused;
    }

    bool ProceduralGenerationSystem::isPaused() const
    {
        return m_paused;
    }

    GenerationContext ProceduralGenerationSystem::createContext(const ChunkCoord& coord, const std::string& parametersId)
    {
        GenerationContext context;
        context.chunkCoord = coord;
        context.seed = m_seed;
        context.chunkSize = 16; // Default size, should be obtained from ChunkManager
        context.worldBounds = Utility::AABB(glm::vec3(-1000000.0f), glm::vec3(1000000.0f)); // Default large bounds

        // Set parameters
        std::string paramsId = parametersId.empty() ? m_activeParametersId : parametersId;
        context.parameters = getParameters(paramsId);

        if (!context.parameters)
        {
            Log::error("Parameters not found: " + paramsId);
            return context;
        }

        // Set noise generator
        std::string noiseId = context.parameters->getTerrainNoiseLayer();
        context.noiseGenerator = getNoiseGenerator(noiseId);

        if (!context.noiseGenerator)
        {
            Log::warn("Noise generator not found: " + noiseId + ", using default");
            context.noiseGenerator = getNoiseGenerator("default");
        }

        // Set feature placement
        std::string placementId = context.parameters->getFeaturePlacementControl();
        if (placementId.empty())
        {
            placementId = "default";
        }
        context.featurePlacement = getFeaturePlacement(placementId);

        if (!context.featurePlacement)
        {
            Log::warn("Feature placement not found: " + placementId + ", using default");
            context.featurePlacement = getFeaturePlacement("default");
        }

        // Set distribution control
        std::string distributionId = context.parameters->getDistributionControl();
        if (distributionId.empty())
        {
            distributionId = "default";
        }
        context.distributionControl = getDistributionControl(distributionId);

        if (!context.distributionControl)
        {
            Log::warn("Distribution control not found: " + distributionId + ", using default");
            context.distributionControl = getDistributionControl("default");
        }

        // Set biome manager
        context.biomeManager = m_biomeManager;

        return context;
    }

    bool ProceduralGenerationSystem::generateTerrainForChunk(const GenerationContext& context, Chunk& chunk)
    {
        if (!context.parameters || !context.noiseGenerator)
        {
            return false;
        }

        int chunkSize = context.chunkSize;
        auto terrainMode = context.parameters->getTerrainMode();

        // Get height range
        float minHeight = context.parameters->getMinHeight();
        float maxHeight = context.parameters->getMaxHeight();
        float heightRange = maxHeight - minHeight;

        // Get water level
        float waterLevel = context.parameters->getWaterLevel();
        bool waterEnabled = context.parameters->isWaterEnabled();

        // Get cave parameters
        bool cavesEnabled = context.parameters->areCavesEnabled();
        float caveDensity = context.parameters->getCaveDensity();
        float caveSize = context.parameters->getCaveSize();

        // Get cave noise generator
        std::shared_ptr<NoiseGenerator> caveNoiseGen = nullptr;
        if (cavesEnabled)
        {
            std::string caveNoiseId = context.parameters->getCaveNoiseLayer();
            if (!caveNoiseId.empty())
            {
                caveNoiseGen = getNoiseGenerator(caveNoiseId);
            }
        }

        // Get chunk world position
        glm::vec3 chunkWorldPos = context.chunkCoord.toWorldPosition(chunkSize);

        // Generate terrain based on mode
        switch (terrainMode)
        {
            case GenerationParameters::TerrainMode::Flat:
            {
                // Flat terrain at a fixed height
                float flatHeight = minHeight + heightRange * 0.5f; // Middle of range

                for (int x = 0; x < chunkSize; x++)
                {
                    for (int z = 0; z < chunkSize; z++)
                    {
                        for (int y = 0; y < chunkSize; y++)
                        {
                            float worldY = chunkWorldPos.y + y;

                            if (worldY <= flatHeight)
                            {
                                // Terrain
                                Voxel voxel;
                                voxel.type = 1; // Solid type

                                // If below water level and water enabled, use different type
                                if (waterEnabled && worldY <= waterLevel)
                                {
                                    voxel.type = 2; // Underwater type
                                }

                                chunk.setVoxel(x, y, z, voxel);
                            }
                            else if (waterEnabled && worldY <= waterLevel)
                            {
                                // Water
                                Voxel voxel;
                                voxel.type = 3; // Water type
                                chunk.setVoxel(x, y, z, voxel);
                            }
                            else
                            {
                                // Air
                                Voxel voxel;
                                voxel.type = 0; // Air type
                                chunk.setVoxel(x, y, z, voxel);
                            }
                        }
                    }
                }
                break;
            }

            case GenerationParameters::TerrainMode::HeightMap:
            {
                // Height map based terrain
                for (int x = 0; x < chunkSize; x++)
                {
                    for (int z = 0; z < chunkSize; z++)
                    {
                        // Calculate world coordinates
                        float worldX = chunkWorldPos.x + x;
                        float worldZ = chunkWorldPos.z + z;

                        // Get terrain height from noise
                        float noise = context.noiseGenerator->generate2D(worldX, worldZ);
                        float terrainHeight = minHeight + (noise * 0.5f + 0.5f) * heightRange;

                        for (int y = 0; y < chunkSize; y++)
                        {
                            float worldY = chunkWorldPos.y + y;

                            // Check if we need to generate caves
                            bool isCave = false;
                            if (cavesEnabled && caveNoiseGen && worldY < terrainHeight)
                            {
                                float caveNoise = caveNoiseGen->generate(worldX, worldY, worldZ);
                                isCave = caveNoise > (1.0f - caveDensity * caveSize);
                            }

                            if (worldY <= terrainHeight && !isCave)
                            {
                                // Terrain
                                Voxel voxel;
                                voxel.type = 1; // Solid type

                                // If below water level and water enabled, use different type
                                if (waterEnabled && worldY <= waterLevel)
                                {
                                    voxel.type = 2; // Underwater type
                                }

                                chunk.setVoxel(x, y, z, voxel);
                            }
                            else if (waterEnabled && worldY <= waterLevel)
                            {
                                // Water
                                Voxel voxel;
                                voxel.type = 3; // Water type
                                chunk.setVoxel(x, y, z, voxel);
                            }
                            else
                            {
                                // Air
                                Voxel voxel;
                                voxel.type = 0; // Air type
                                chunk.setVoxel(x, y, z, voxel);
                            }
                        }
                    }
                }
                break;
            }

            case GenerationParameters::TerrainMode::Volumetric:
            {
                // Full 3D volumetric terrain with caves
                for (int x = 0; x < chunkSize; x++)
                {
                    for (int y = 0; y < chunkSize; y++)
                    {
                        for (int z = 0; z < chunkSize; z++)
                        {
                            // Calculate world coordinates
                            float worldX = chunkWorldPos.x + x;
                            float worldY = chunkWorldPos.y + y;
                            float worldZ = chunkWorldPos.z + z;

                            // Get density from noise
                            float density = context.noiseGenerator->generate(worldX, worldY, worldZ);

                            // Adjust density based on height (gradually less dense with height)
                            float heightFactor = 1.0f - ((worldY - minHeight) / heightRange);
                            heightFactor = glm::clamp(heightFactor, 0.0f, 1.0f);

                            // Apply cave noise if enabled
                            if (cavesEnabled && caveNoiseGen)
                            {
                                float caveNoise = caveNoiseGen->generate(worldX, worldY, worldZ);
                                density -= caveNoise * caveDensity * caveSize;
                            }

                            // Apply height adjustment
                            density = density * heightFactor;

                            Voxel voxel;

                            if (density > 0.0f)
                            {
                                // Solid terrain
                                voxel.type = 1; // Solid type

                                // If below water level and water enabled, use different type
                                if (waterEnabled && worldY <= waterLevel)
                                {
                                    voxel.type = 2; // Underwater type
                                }
                            }
                            else if (waterEnabled && worldY <= waterLevel)
                            {
                                // Water
                                voxel.type = 3; // Water type
                            }
                            else
                            {
                                // Air
                                voxel.type = 0; // Air type
                            }

                            chunk.setVoxel(x, y, z, voxel);
                        }
                    }
                }
                break;
            }

            case GenerationParameters::TerrainMode::Islands:
            {
                // Floating islands
                for (int x = 0; x < chunkSize; x++)
                {
                    for (int y = 0; y < chunkSize; y++)
                    {
                        for (int z = 0; z < chunkSize; z++)
                        {
                            // Calculate world coordinates
                            float worldX = chunkWorldPos.x + x;
                            float worldY = chunkWorldPos.y + y;
                            float worldZ = chunkWorldPos.z + z;

                            // Get 3D noise for islands
                            float islandNoise = context.noiseGenerator->generate(worldX * 0.05f, worldY * 0.05f, worldZ * 0.05f);

                            // Create floating island effect
                            float heightFactor = std::abs(worldY - (minHeight + heightRange * 0.5f)) / (heightRange * 0.5f);
                            heightFactor = glm::clamp(heightFactor, 0.0f, 1.0f);

                            // Adjust noise based on height (islands concentrated in the middle)
                            islandNoise = islandNoise - heightFactor * 1.5f;

                            Voxel voxel;

                            if (islandNoise > 0.1f)
                            {
                                // Solid island terrain
                                voxel.type = 1; // Solid type
                            }
                            else if (waterEnabled && worldY <= waterLevel)
                            {
                                // Water
                                voxel.type = 3; // Water type
                            }
                            else
                            {
                                // Air
                                voxel.type = 0; // Air type
                            }

                            chunk.setVoxel(x, y, z, voxel);
                        }
                    }
                }
                break;
            }

            case GenerationParameters::TerrainMode::Mountains:
            {
                // Mountainous terrain
                for (int x = 0; x < chunkSize; x++)
                {
                    for (int z = 0; z < chunkSize; z++)
                    {
                        // Calculate world coordinates
                        float worldX = chunkWorldPos.x + x;
                        float worldZ = chunkWorldPos.z + z;

                        // Get mountain height from noise
                        float baseNoise = context.noiseGenerator->generate2D(worldX * 0.01f, worldZ * 0.01f);
                        float detailNoise = context.noiseGenerator->generate2D(worldX * 0.05f, worldZ * 0.05f) * 0.3f;

                        // Adjust for more dramatic mountains
                        float mountainFactor = (baseNoise + 1.0f) * 0.5f; // [0, 1]
                        mountainFactor = std::pow(mountainFactor, 3.0f); // Steeper mountains

                        float terrainHeight = minHeight + (mountainFactor + detailNoise) * heightRange;

                        for (int y = 0; y < chunkSize; y++)
                        {
                            float worldY = chunkWorldPos.y + y;

                            // Check if we need to generate caves
                            bool isCave = false;
                            if (cavesEnabled && caveNoiseGen && worldY < terrainHeight)
                            {
                                float caveNoise = caveNoiseGen->generate(worldX, worldY, worldZ);
                                isCave = caveNoise > (1.0f - caveDensity * caveSize);
                            }

                            if (worldY <= terrainHeight && !isCave)
                            {
                                // Terrain
                                Voxel voxel;
                                voxel.type = 1; // Solid type

                                // Different types based on height
                                if (worldY > terrainHeight - 3.0f)
                                {
                                    voxel.type = 4; // Top layer type
                                }
                                else if (worldY > minHeight + heightRange * 0.7f)
                                {
                                    voxel.type = 5; // Mountain type
                                }
                                else if (waterEnabled && worldY <= waterLevel)
                                {
                                    voxel.type = 2; // Underwater type
                                }

                                chunk.setVoxel(x, y, z, voxel);
                            }
                            else if (waterEnabled && worldY <= waterLevel)
                            {
                                // Water
                                Voxel voxel;
                                voxel.type = 3; // Water type
                                chunk.setVoxel(x, y, z, voxel);
                            }
                            else
                            {
                                // Air
                                Voxel voxel;
                                voxel.type = 0; // Air type
                                chunk.setVoxel(x, y, z, voxel);
                            }
                        }
                    }
                }
                break;
            }

            default:
            {
                // Default to height map if not implemented
                Log::warn("Unsupported terrain mode, falling back to height map");
                return generateTerrainForChunk(context, chunk);
            }
        }

        return true;
    }

    bool ProceduralGenerationSystem::generateFeaturesForChunk(const GenerationContext& context, Chunk& chunk)
    {
        if (!context.parameters || !context.featurePlacement || !context.distributionControl)
        {
            return false;
        }

        std::vector<std::string> featureTypes = context.featurePlacement->getFeatureTypes();

        // If no registered feature types, nothing to generate
        if (featureTypes.empty())
        {
            return true;
        }

        // Create a grid reference for feature placement constraints
        // This would be obtained from the application in a real implementation
        Grid* grid = nullptr;
        auto app = Core::Application::getInstance();
        grid = app->getSubsystem<ChunkManager>()->getGrid();

        if (!grid)
        {
            Log::warn("No grid available for feature placement");
            return false;
        }

        // For each feature type
        for (const auto& featureType : featureTypes)
        {
            // Check if feature type is enabled
            if (!context.parameters->isFeatureTypeEnabled(featureType))
            {
                continue;
            }

            // Get feature density
            float density = context.parameters->getFeatureTypeDensity(featureType);

            // Calculate max count based on density
            int maxCount = static_cast<int>(density * context.chunkSize * context.chunkSize / 64.0f);
            maxCount = std::max(1, maxCount); // At least 1

            // Find valid placements
            std::vector<glm::vec3> placements = context.featurePlacement->findPlacementsInChunk(
                context.chunkCoord, featureType, maxCount, context, *grid);

            // Generate features at valid positions
            for (const auto& pos : placements)
            {
                context.featurePlacement->placeFeature(pos, featureType, context, chunk);
            }
        }

        return true;
    }

    bool ProceduralGenerationSystem::generateBiomesForChunk(const GenerationContext& context, Chunk& chunk)
    {
        if (!context.parameters || !context.biomeManager)
        {
            return false;
        }

        // Generate biome data for the chunk
        return context.biomeManager->generateBiomeData(context.chunkCoord, context, chunk);
    }

    void ProceduralGenerationSystem::processGenerationQueue()
    {
        while (!m_shutdownThread)
        {
            // Sleep to avoid high CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            if (m_paused)
            {
                continue;
            }

            GenerationTask currentTask;

            // Get the highest priority task
            {
                std::lock_guard<std::mutex> lock(m_taskMutex);

                if (m_generationTasks.empty())
                {
                    continue;
                }

                // Find the highest priority task
                auto it = std::min_element(m_generationTasks.begin(), m_generationTasks.end(),
                                           [](const GenerationTask& a, const GenerationTask& b) {
                                               if (a.complete || a.canceled) return false;
                                               if (b.complete || b.canceled) return true;
                                               return a.priority < b.priority;
                                           });

                if (it == m_generationTasks.end() || it->complete || it->canceled)
                {
                    continue;
                }

                currentTask = *it;
            }

            // Process the task
            Grid* grid = nullptr;
            auto app = Core::Application::getInstance();
            grid = app->getSubsystem<ChunkManager>()->getGrid();

            if (!grid)
            {
                Log::error("No grid available for pre-generation");

                // Mark task as complete
                std::lock_guard<std::mutex> lock(m_taskMutex);
                for (auto& task : m_generationTasks)
                {
                    if (task.id == currentTask.id)
                    {
                        task.complete = true;
                        break;
                    }
                }

                continue;
            }

            // Generate chunks in the region
            generateRegion(currentTask.min, currentTask.max, *grid, currentTask.parametersId);

            // Mark task as complete
            {
                std::lock_guard<std::mutex> lock(m_taskMutex);
                for (auto& task : m_generationTasks)
                {
                    if (task.id == currentTask.id)
                    {
                        task.complete = true;
                        break;
                    }
                }
            }
        }
    }

    void ProceduralGenerationSystem::updateNeighborChunks(const ChunkCoord& coord, Grid& grid)
    {
        // Check all 6 neighbors and update them if needed
        ChunkCoord neighbors[6] = {
            ChunkCoord(coord.x + 1, coord.y, coord.z),
            ChunkCoord(coord.x - 1, coord.y, coord.z),
            ChunkCoord(coord.x, coord.y + 1, coord.z),
            ChunkCoord(coord.x, coord.y - 1, coord.z),
            ChunkCoord(coord.x, coord.y, coord.z + 1),
            ChunkCoord(coord.x, coord.y, coord.z - 1)
        };

        for (const auto& neighborCoord : neighbors)
        {
            auto chunk = grid.getChunk(neighborCoord);
            if (chunk)
            {
                // Update mesh or notify chunk of neighbor change
                // In a real implementation, this would likely call:
                // chunk->markMeshDirty();
            }
        }
    }

} // namespace PixelCraft::Voxel