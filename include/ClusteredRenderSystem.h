#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <glad/glad.h>
#include "RenderSystem.h"
#include "light.h"
#include "Decal.h"
#include "Texture.h"

// Forward declarations
class RenderableObject;
class RenderStage;
class PostProcessor;
class RenderTarget;
class RenderContext;
class Camera;
class Material;
class Shader;
class Light;
class Decal;
class Texture;
class TextureManager;
class PaletteManager;

// For weather and atmosphere parameters
struct WeatherParameters
{
    float rainIntensity = 0.0f;
    float snowIntensity = 0.0f;
    float fogDensity = 0.0f;
    glm::vec3 fogColor = glm::vec3(0.8f, 0.9f, 1.0f);
    float windSpeed = 0.0f;
    glm::vec3 windDirection = glm::vec3(1.0f, 0.0f, 0.0f);
};

struct AtmosphereSettings
{
    float rayleighScattering = 1.0f;
    float mieScattering = 1.0f;
    float exposure = 1.0f;
    glm::vec3 skyTint = glm::vec3(1.0f);
    bool enableAtmosphere = true;
};

// Light cluster data structure
struct ClusterData
{
    GLuint clusterBuffer;         // SSBO for cluster information
    GLuint lightGrid;             // 3D texture for light indices per cluster
    GLuint lightIndexList;        // SSBO for light indices
    GLuint clusterAABBs;          // SSBO for cluster bounding box
    GLuint decalGrid;             // SSBO for decal grid
    GLuint decalIndexList;        // SSBO for decal indices

    std::vector<uint32_t> lightAssignmentCount;
    std::vector<glm::vec4> clusterBounds;

    uint32_t clusterDimX;
    uint32_t clusterDimY;
    uint32_t clusterDimZ;
    uint32_t totalClusters;

    uint32_t maxLightsPerCluster;
    uint32_t maxDecalsPerCluster;
    uint32_t totalLightIndices;
    uint32_t activeClusterCount;
    uint32_t visibleLightCount;

    float nearClip;
    float farClip;

    ClusterData() :
        clusterBuffer(0),
        lightGrid(0),
        lightIndexList(0),
        clusterAABBs(0),
        decalGrid(0),
        decalIndexList(0),
        clusterDimX(16),
        clusterDimY(8),
        clusterDimZ(24),
        totalClusters(0),
        maxLightsPerCluster(64),
        maxDecalsPerCluster(32),
        totalLightIndices(0),
        activeClusterCount(0),
        visibleLightCount(0),
        nearClip(0.1f),
        farClip(100.0f)
    {
        totalClusters = clusterDimX * clusterDimY * clusterDimZ;
    }
};

class ClusteredRenderSystem
{
public:
    // Constructor and destructor
    ClusteredRenderSystem();
    ~ClusteredRenderSystem();

    // Core methods from API
    bool initialize();
    void render(Camera* camera);
    void update(float deltaTime);
    void resize(int width, int height);

    // Texture management
    TextureManager* getTextureManager();
    Texture* loadTexture(const std::string& path, bool generateMipmaps = false, bool sRGB = false);
    Texture* createEmptyTexture(int width, int height, int channels = 4, bool generateMipmaps = false, bool sRGB = false);

    // Material management
    Material* createMaterial(Shader* shader);
    void releaseMaterial(Material* material);
    void bindMaterialToClusterSystem(Material* material);

    // Object management
    void addRenderableObject(RenderableObject* object);
    void removeRenderableObject(RenderableObject* object);

    // Shader management
    void reloadShaders();

    // Pipeline configuration
    void addRenderStage(std::shared_ptr<RenderStage> stage);
    void addPostProcessor(std::shared_ptr<PostProcessor> processor);

    // Cluster configuration
    void configureClustering(uint32_t dimX, uint32_t dimY, uint32_t dimZ);

    // Scene elements
    void addLight(Light* light);
    void removeLight(Light* light);
    void addDecal(Decal* decal);
    void removeDecal(Decal* decal);

    // Palette management
    void setPaletteManager(PaletteManager* paletteManager);
    PaletteManager* getPaletteManager() const;

    // Environment settings
    void setTimeOfDay(float timeOfDay);
    void setWeatherConditions(const WeatherParameters& params);
    void setAtmosphereSettings(const AtmosphereSettings& settings);

    // Accessors
    RenderTarget* getMainRenderTarget();
    RenderTarget* getFinalRenderTarget();
    uint32_t getActiveClusterCount() const;
    uint32_t getVisibleLightCount() const;
    const WeatherParameters& getWeatherParameters() const;
    const AtmosphereSettings& getAtmosphereSettings() const;
    float getTimeOfDay() const;

    // Pixel art integration
    void configureForPixelArt(int pixelSize, bool snapToGrid);
    void setPaletteOptions(bool enabled, int paletteSize, Texture* paletteTexture);
    void setDitheringOptions(bool enabled, float strength, Texture* patternTexture);
    void setPostProcessingEnabled(bool enabled);


private:
    // Rendering resources
    std::vector<RenderableObject*> m_renderableObjects;
    std::vector<std::shared_ptr<RenderStage>> m_renderStages;
    std::vector<std::shared_ptr<PostProcessor>> m_postProcessors;

    // Texture management
    std::unique_ptr<TextureManager> m_textureManager;
    PaletteManager* m_paletteManager;

    // Lighting and environment
    std::vector<Light*> m_lights;
    std::vector<Decal*> m_decals;
    float m_timeOfDay;
    float m_deltaTime;
    WeatherParameters m_weatherParams;
    AtmosphereSettings m_atmosphereSettings;

    // Cluster data
    ClusterData m_clusterData;

    // GPU resources
    GLuint m_lightBuffer;
    GLuint m_decalBuffer;
    GLuint m_quadVAO;
    GLuint m_quadVBO;

    // Shaders
    Shader* m_clusterBuildShader;
    Shader* m_lightAssignShader;
    Shader* m_decalAssignShader;
    Shader* m_pixelationShader;
    Shader* m_debugClusterShader;

    // Render targets
    std::unique_ptr<RenderTarget> m_mainRenderTarget;
    std::unique_ptr<RenderTarget> m_intermediateTarget;
    std::unique_ptr<RenderTarget> m_finalRenderTarget;

    // Pixel art settings
    int m_pixelSize;
    bool m_snapToGrid;
    bool m_paletteEnabled;
    int m_paletteSize;
    Texture* m_paletteTexture;
    bool m_ditheringEnabled;
    float m_ditherStrength;
    Texture* m_ditherPatternTexture;
    bool m_postProcessingEnabled;

    // Viewport dimensions
    int m_width;
    int m_height;

    // Helper methods
    void initializeShaders();
    void createRenderTargets(int width, int height);
    void createFullscreenQuad();
    void renderFullscreenQuad();
    void updateClusterGrid(Camera* camera);
    void assignLightsToClusters();
    void assignDecalsToClusters();
    void setupDefaultRenderPipeline();
    void renderDebugClusters(RenderContext& context);
    void updateLightBuffer();
    void updateDecalBuffer();
    void applyPixelArtPass(RenderTarget* source, RenderTarget* destination);

    // Material binding helpers
    void setupMaterialForClustering(Material* material, Shader* shader);

    // Compute shader dispatch calculation
    void calculateComputeWorkGroups(uint32_t& workGroupsX, uint32_t& workGroupsY, uint32_t& workGroupsZ) const;

    // Utility methods
    glm::vec4 calculateZPlaneEquation(float zNear, float zFar, int clusterIndex) const;
    uint32_t calculateClusterIndex(uint32_t x, uint32_t y, uint32_t z) const;
};