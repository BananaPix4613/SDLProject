// -------------------------------------------------------------------------
// ProceduralGenerationSystem.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Subsystem.h"
#include "Core/Resource.h"
#include "Voxel/Grid.h"
#include "Voxel/Chunk.h"
#include "Voxel/ChunkCoord.h"
#include "Utility/AABB.h"

namespace PixelCraft::Voxel
{

    // Forward declarations
    class NoiseGenerator;
    class GenerationParameters;
    class FeaturePlacement;
    class DistributionControl;
    class BiomeManager;
    struct GenerationContext;

    /**
     * @brief Procedural Generation System for voxel-based world creation
     * 
     * This subsystem manages the procedural generation process for voxel worlds,
     * including terrain formation, biome distribution, and feature placement.
     * It supports both runtime generation and offline pre-generation.
     */
    class ProceduralGenerationSystem : public Core::Subsystem
    {
    public:
        /**
         * @brief Constructor
         */
        ProceduralGenerationSystem();

        /**
         * @brief Destructor
         */
        virtual ~ProceduralGenerationSystem();

        /**
         * @brief Initialize the subsystem
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the subsystem
         * @param deltaTime Time since last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Render debug visualization (if applicable)
         */
        void render() override;

        /**
         * @brief Shut down the subsystem and release resources
         */
        void shutdown() override;

        /**
         * @brief Get the name of the subsystem
         * @return The subsystem name
         */
        std::string getName() const override
        {
            return "ProceduralGenerationSystem";
        }

        /**
         * @brief Get the module's dependencies
         * @return Vector of dependency subsystem names
         */
        std::vector<std::string> getDependencies() const override;

        /**
         * @brief Generate a single chunk
         * @param coord The chunk coordinate
         * @param chunk Reference to the chunk to populate
         * @param parametersId ID of the generation parameters to use (defaults to active)
         * @return True if generation was successful
         */
        bool generateChunk(const ChunkCoord& coord, Chunk& chunk, const std::string& parametersId = "");

        /**
         * @brief Generate multiple chunks within a region
         * @param min Minimum chunk coordinate
         * @param max Maximum chunk coordinate
         * @param grid Reference to the grid to populate
         * @param parametersId ID of the generation parameters to use (defaults to active)
         * @return Number of chunks successfully generated
         */
        int generateRegion(const ChunkCoord& min, const ChunkCoord& max, Grid& grid, const std::string& parametersId = "");

        /**
         * @brief Generate chunks around a point within a radius
         * @param center World-space center point
         * @param radius Radius in world units
         * @param grid Reference to the grid to populate
         * @param parametersId ID of the generation parameters to use (defaults to active)
         * @return Number of chunks successfully generated
         */
        int generateAroundPoint(const glm::vec3& center, float radius, Grid& grid, const std::string& parametersId = "");

        /**
         * @brief Create new generation parameters
         * @param id Unique identifier for the parameters
         * @return Shared pointer to the created parameters, nullptr if already exists
         */
        std::shared_ptr<GenerationParameters> createParameters(const std::string& id);

        /**
         * @brief Get generation parameters by ID
         * @param id Identifier of the parameters
         * @return Shared pointer to the parameters, nullptr if not found
         */
        std::shared_ptr<GenerationParameters> getParameters(const std::string& id) const;

        /**
         * @brief Set the active generation parameters
         * @param id Identifier of the parameters to set as active
         * @return True if parameters were found and set as active
         */
        bool setActiveParameters(const std::string& id);

        /**
         * @brief Get the active generation parameters
         * @return Shared pointer to the active parameters, nullptr if none
         */
        std::shared_ptr<GenerationParameters> getActiveParameters() const;

        /**
         * @brief Create a noise generator
         * @param id Unique identifier for the generator
         * @return Shared pointer to the created generator, nullptr if already exists
         */
        std::shared_ptr<NoiseGenerator> createNoiseGenerator(const std::string& id);

        /**
         * @brief Get a noise generator by ID
         * @param id Identifier of the generator
         * @return Shared pointer to the generator, nullptr if not found
         */
        std::shared_ptr<NoiseGenerator> getNoiseGenerator(const std::string& id) const;

        /**
         * @brief Create a feature placement controller
         * @param id Unique identifier for the placement controller
         * @return Shared pointer to the created placement controller, nullptr if already exists
         */
        std::shared_ptr<FeaturePlacement> createFeaturePlacement(const std::string& id);

        /**
         * @brief Get a feature placement controller by ID
         * @param id Identifier of the placement controller
         * @return Shared pointer to the placement controller, nullptr if not found
         */
        std::shared_ptr<FeaturePlacement> getFeaturePlacement(const std::string& id) const;

        /**
         * @brief Create a distribution control system
         * @param id Unique identifier for the distribution control
         * @return Shared pointer to the created distribution control, nullptr if already exists
         */
        std::shared_ptr<DistributionControl> createDistributionControl(const std::string& id);

        /**
         * @brief Get a distribution control system by ID
         * @param id Identifier of the distribution control
         * @return Shared pointer to the distribution control, nullptr if not found
         */
        std::shared_ptr<DistributionControl> getDistributionControl(const std::string& id) const;

        /**
         * @brief Load generation parameters from file
         * @param path Path to the parameters file
         * @param id Identifier to assign to the loaded parameters
         * @return Shared pointer to the loaded parameters, nullptr on failure
         */
        std::shared_ptr<GenerationParameters> loadParameters(const std::string& path, const std::string& id);

        /**
         * @brief Save generation parameters to file
         * @param id Identifier of the parameters to save
         * @param path Path to save the parameters to
         * @return True if successfully saved
         */
        bool saveParameters(const std::string& id, const std::string& path);

        /**
         * @brief Get the biome manager
         * @return Shared pointer to the biome manager
         */
        std::shared_ptr<BiomeManager> getBiomeManager() const;

        /**
         * @brief Get the generation seed
         * @return The current generation seed
         */
        uint32_t getSeed() const;

        /**
         * @brief Set the generation seed
         * @param seed The new seed value
         */
        void setSeed(uint32_t seed);

        /**
         * @brief Register a custom feature generator
         * @param name Unique name for the feature
         * @param generator Function to generate the feature
         * @return True if successfully registered
         */
        bool registerFeatureGenerator(const std::string& name,
                                      std::function<bool(const GenerationContext&, Chunk&)> generator);

        /**
         * @brief Get a registered feature generator
         * @param name Name of the feature generator
         * @return The generator function, nullptr if not found
         */
        std::function<bool(const GenerationContext&, Chunk&)> getFeatureGenerator(const std::string& name) const;

        /**
         * @brief Pre-generate and cache chunks in a separate thread
         * @param min Minimum chunk coordinate
         * @param max Maximum chunk coordinate
         * @param priority Generation priority (higher values = higher priority)
         * @return ID of the generation task for tracking
         */
        int preGenerateChunks(const ChunkCoord& min, const ChunkCoord& max, int priority = 0);

        /**
         * @brief Check if a pre-generation task is complete
         * @param taskId ID of the task to check
         * @return True if the task is complete
         */
        bool isPreGenerationComplete(int taskId);

        /**
         * @brief Cancel a pre-generation task
         * @param taskId ID of the task to cancel
         * @return True if the task was found and canceled
         */
        bool cancelPreGeneration(int taskId);

        /**
         * @brief Set whether generation is paused
         * @param paused True to pause generation, false to resume
         */
        void setPaused(bool paused);

        /**
         * @brief Check if generation is paused
         * @return True if generation is paused
         */
        bool isPaused() const;

        /**
         * @brief Create a generation context for a given chunk
         * @param coord The chunk coordinate
         * @param parametersId ID of the generation parameters to use
         * @return The generation context
         */
        GenerationContext createContext(const ChunkCoord& coord, const std::string& parametersId = "");

    private:
        /**
         * @brief Generate terrain for the chunk
         * @param context Generation context for terrain
         * @param chunk Chunk to generate terrain in
         * @return True if generated successfully
         */
        bool generateTerrainForChunk(const GenerationContext& context, Chunk& chunk);

        /**
         * @brief Generate features for the chunk
         * @param context Generation context for features
         * @param chunk Chunk to generate features in
         * @return True if generated successfully
         */
        bool generateFeaturesForChunk(const GenerationContext& context, Chunk& chunk);

        /**
         * @brief Generate biomes for the chunk
         * @param context Generation context for biomes
         * @param chunk Chunk to generate biomes in
         * @return True if generated successfully
         */
        bool generateBiomesForChunk(const GenerationContext& context, Chunk& chunk);

        /**
         * @brief Process the generation queue
         */
        void processGenerationQueue();

        /**
         * @brief Update the neighboring chunks
         * @param coord Chunk to update neighbors of
         * @param grid Grid that chunk exists in
         */
        void updateNeighborChunks(const ChunkCoord& coord, Grid& grid);

        // Member variables
        bool m_initialized = false;
        bool m_paused = false;
        uint32_t m_seed = 12345;
        std::string m_activeParametersId;
        std::unordered_map<std::string, std::shared_ptr<GenerationParameters>> m_parameters;
        std::unordered_map<std::string, std::shared_ptr<NoiseGenerator>> m_noiseGenerators;
        std::unordered_map<std::string, std::shared_ptr<FeaturePlacement>> m_featurePlacements;
        std::unordered_map<std::string, std::shared_ptr<DistributionControl>> m_distributionControls;
        std::shared_ptr<BiomeManager> m_biomeManager;
        std::unordered_map<std::string, std::function<bool(const GenerationContext&, Chunk&)>> m_featureGenerators;

        // Pre-generation task tracking
        struct GenerationTask
        {
            int id;
            ChunkCoord min;
            ChunkCoord max;
            std::string parametersId;
            int priority;
            bool complete;
            bool canceled;
        };
        std::vector<GenerationTask> m_generationTasks;
        int m_nextTaskId = 0;
        std::mutex m_taskMutex;
        std::thread m_generationThread;
        std::atomic<bool> m_shutdownThread;
    };

    /**
     * @brief Context for chunk generation
     * 
     * Contains all necessary information for generating a chunk, including
     * coordinate, parameters, and references to related systems.
     */
    struct GenerationContext
    {
        ChunkCoord chunkCoord;
        std::shared_ptr<GenerationParameters> parameters;
        std::shared_ptr<NoiseGenerator> noiseGenerator;
        std::shared_ptr<FeaturePlacement> featurePlacement;
        std::shared_ptr<DistributionControl> distributionControl;
        std::shared_ptr<BiomeManager> biomeManager;
        uint32_t seed;
        Utility::AABB worldBounds;
        int chunkSize;
    };

} // namespace PixelCraft::Voxel