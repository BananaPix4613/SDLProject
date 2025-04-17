#include "ClusteredRenderSystem.h"
#include "RenderSystem.h"
#include "Camera.h"
#include "Shader.h"
#include "Light.h"
#include "Decal.h"
#include "Material.h"
#include "Texture.h"
#include "PaletteManager.h"
#include <algorithm>
#include <iostream>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

// Constructor
ClusteredRenderSystem::ClusteredRenderSystem() :
    m_clusterBuildShader(nullptr),
    m_lightAssignShader(nullptr),
    m_decalAssignShader(nullptr),
    m_pixelationShader(nullptr),
    m_debugClusterShader(nullptr),
    m_lightBuffer(0),
    m_decalBuffer(0),
    m_quadVAO(0),
    m_quadVBO(0),
    m_timeOfDay(12.0f), // Noon default
    m_deltaTime(0.0f),
    m_pixelSize(2),
    m_snapToGrid(true),
    m_paletteEnabled(false),
    m_paletteSize(16),
    m_paletteTexture(nullptr),
    m_paletteManager(nullptr),
    m_ditheringEnabled(false),
    m_ditherStrength(0.5f),
    m_ditherPatternTexture(nullptr),
    m_postProcessingEnabled(true),
    m_width(1280),
    m_height(720)
{
    // Initialize default weather parameters
    m_weatherParams.rainIntensity = 0.0f;
    m_weatherParams.snowIntensity = 0.0f;
    m_weatherParams.fogDensity = 0.05f;
    m_weatherParams.fogColor = glm::vec3(0.8f, 0.9f, 1.0f);
    m_weatherParams.windSpeed = 1.0f;
    m_weatherParams.windDirection = glm::vec3(1.0f, 0.0f, 0.0f);

    // Initialize default atmosphere settings
    m_atmosphereSettings.rayleighScattering = 1.0f;
    m_atmosphereSettings.mieScattering = 1.0f;
    m_atmosphereSettings.exposure = 1.0f;
    m_atmosphereSettings.skyTint = glm::vec3(1.0f);
    m_atmosphereSettings.enableAtmosphere = true;

    // Initialize texture manager
    m_textureManager = std::make_unique<TextureManager>();

    // Create palette manager
    m_paletteManager = new PaletteManager(m_textureManager.get());
}

// Destructor
ClusteredRenderSystem::~ClusteredRenderSystem()
{
    // Clean up GPU resources
    if (m_clusterData.clusterBuffer)
        glDeleteBuffers(1, &m_clusterData.clusterBuffer);

    if (m_clusterData.lightGrid)
        glDeleteTextures(1, &m_clusterData.lightGrid);

    if (m_clusterData.lightIndexList)
        glDeleteBuffers(1, &m_clusterData.lightIndexList);

    if (m_clusterData.clusterAABBs)
        glDeleteBuffers(1, &m_clusterData.clusterAABBs);

    if (m_clusterData.decalGrid)
        glDeleteBuffers(1, &m_clusterData.decalGrid);

    if (m_clusterData.decalIndexList)
        glDeleteBuffers(1, &m_clusterData.decalIndexList);

    if (m_lightBuffer)
        glDeleteBuffers(1, &m_lightBuffer);

    if (m_decalBuffer)
        glDeleteBuffers(1, &m_decalBuffer);

    if (m_quadVAO)
        glDeleteVertexArrays(1, &m_quadVAO);

    if (m_quadVBO)
        glDeleteBuffers(1, &m_quadVBO);

    // TextureManager will handle texture cleanup
    m_textureManager.reset();

    // Clean up palette manager
    delete m_paletteManager;

    // Render targets are cleaned up automatically via smart pointers
}

// Initialize the clustered render system
bool ClusteredRenderSystem::initialize()
{
    // Initialize shaders
    initializeShaders();

    // Create fullscreen quad for post-processing
    createFullscreenQuad();

    // Create render targets with initial dimensions
    createRenderTargets(m_width, m_height);

    // Configure initial clustering
    configureClustering(16, 8, 24);

    // Create GPU buffers for cluster data
    glGenBuffers(1, &m_clusterData.clusterBuffer);
    glGenTextures(1, &m_clusterData.lightGrid);
    glGenBuffers(1, &m_clusterData.lightIndexList);
    glGenBuffers(1, &m_clusterData.clusterAABBs);
    glGenBuffers(1, &m_clusterData.decalGrid);
    glGenBuffers(1, &m_clusterData.decalIndexList);

    // Create light and decal buffers
    glGenBuffers(1, &m_lightBuffer);
    glGenBuffers(1, &m_decalBuffer);

    // Initialize texture manager default textures
    m_textureManager->initializeDefaultTextures();

    // Set up default render pipeline
    setupDefaultRenderPipeline();

    std::cout << "ClusteredRenderSystem initialized with TextureManager" << std::endl;

    return true;
}

// Create and compile shaders used by the clustered renderer
void ClusteredRenderSystem::initializeShaders()
{
    // Create shader for building clusters
    m_clusterBuildShader = ShaderCache::getComputeShader("shaders/ClusterBuildCompute.glsl");

    // Create shader for assigning lights to clusters
    m_lightAssignShader = ShaderCache::getComputeShader("shaders/LightAssignCompute.glsl");

    // Create shader for assigning decals to clusters
    m_decalAssignShader = ShaderCache::getComputeShader("shaders/DecalAssignCompute.glsl");

    // Create shader for pixelation post-processing
    m_pixelationShader = ShaderCache::getShader("shaders/PixelArtVert.glsl", "shaders/PixelArtFrag.glsl");

    // Create shader for debug visualization of clusters
    m_debugClusterShader = ShaderCache::getShader("shaders/ClusterDebugVert.glsl", "shaders/ClusterDebugFrag.glsl");
}

// Create render targets for the rendering pipeline
void ClusteredRenderSystem::createRenderTargets(int width, int height)
{
    // Main render target for scene rendering
    m_mainRenderTarget = std::make_unique<RenderTarget>(width, height);

    // Intermediate target for post-processing
    m_intermediateTarget = std::make_unique<RenderTarget>(width, height);

    // Final target for display
    // For pixel art, the final target size is based on the pixel size
    int finalWidth = width / m_pixelSize;
    int finalHeight = height / m_pixelSize;
    m_finalRenderTarget = std::make_unique<RenderTarget>(finalWidth, finalHeight);
}

// Create a fullscreen quad for post-processing
void ClusteredRenderSystem::createFullscreenQuad()
{
    float quadVertices[] = {
        // positions        // texture coords
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };

    // Create VAO and VBO
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);

    glBindVertexArray(m_quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    // Texture coordinate attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void ClusteredRenderSystem::renderFullscreenQuad()
{
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// Configure the clustering dimensions
void ClusteredRenderSystem::configureClustering(uint32_t dimX, uint32_t dimY, uint32_t dimZ)
{
    m_clusterData.clusterDimX = dimX;
    m_clusterData.clusterDimY = dimY;
    m_clusterData.clusterDimZ = dimZ;
    m_clusterData.totalClusters = dimX * dimY * dimZ;

    // Resize data containers
    m_clusterData.lightAssignmentCount.resize(m_clusterData.totalClusters, 0);
    m_clusterData.clusterBounds.resize(m_clusterData.totalClusters * 2); // min and max point for each cluster

    std::cout << "Configured clustering: " << dimX << "x" << dimY << "x" << dimZ
              << " (" << m_clusterData.totalClusters << " clusters)" << std::endl;
}

// Resize the render system when the viewport changes
void ClusteredRenderSystem::resize(int width, int height)
{
    m_width = width;
    m_height = height;

    // Recreate render targets
    createRenderTargets(width, height);

    std::cout << "ClusteredRenderSystem resized to " << width << "x" << height << std::endl;
}

// Main rendering function
void ClusteredRenderSystem::render(Camera* camera)
{
    if (!camera) return;

    // Set up view and projection matrices
    glm::mat4 viewMatrix = camera->getViewMatrix();
    glm::mat4 projMatrix = camera->getProjectionMatrix();

    // Create render context
    RenderContext context;
    context.camera = camera;
    context.viewMatrix = viewMatrix;
    context.projectionMatrix = projMatrix;

    // Add clustered rendering specific context data
    context.clusterDimX = m_clusterData.clusterDimX;
    context.clusterDimY = m_clusterData.clusterDimY;
    context.clusterDimZ = m_clusterData.clusterDimZ;
    context.lightGrid = m_clusterData.lightGrid;
    context.lightIndexList = m_clusterData.lightIndexList;
    context.decalGrid = m_clusterData.decalGrid;
    context.decalIndexList = m_clusterData.decalIndexList;
    context.nearClip = m_clusterData.nearClip;
    context.farClip = m_clusterData.farClip;

    // Update cluster grid based on camera parameters
    updateClusterGrid(camera);

    // Update light and decal buffers
    updateLightBuffer();
    updateDecalBuffer();

    // Update palette manager with current time of day
    if (m_paletteManager)
    {
        m_paletteManager->update(m_timeOfDay, m_deltaTime);
    }

    // Assign lights and decals to clusters
    assignLightsToClusters();
    assignDecalsToClusters();

    // Bind main render target
    m_mainRenderTarget->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Execute render stages
    for (auto& stage : m_renderStages)
    {
        if (stage->getActive())
        {
            stage->execute(context);
        }
    }

    // Apply post-processing
    if (m_postProcessingEnabled && !m_postProcessors.empty())
    {
        // Use main target as input, final as output
        RenderTarget* input = m_mainRenderTarget.get();
        RenderTarget* output = m_intermediateTarget.get();

        // Chain post-processors
        for (size_t i = 0; i < m_postProcessors.size(); i++)
        {
            // For final processor, output to final render target
            // For intermediate processors, ping-pong between targets
            RenderTarget* currentOutput = (i == m_postProcessors.size() - 1) ?
                output : m_intermediateTarget.get();

            // Apply the post-processor
            m_postProcessors[i]->apply(input, currentOutput, context);

            // Next input is current output
            input = currentOutput;
        }
    }
    else if (m_pixelSize > 1)
    {
        // If no post-processors but pixel art is enabled, apply pixelation directly
        applyPixelArtPass(m_finalRenderTarget.get(), m_finalRenderTarget.get());
    }

    // Debug visualization (if enabled)
    if (context.showWireframe)
    {
        renderDebugClusters(context);
    }

    // Unbind framebuffer to return to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ClusteredRenderSystem::update(float deltaTime)
{
    m_deltaTime = deltaTime;
}

// Update the cluster grid based on camera parameters
void ClusteredRenderSystem::updateClusterGrid(Camera* camera)
{
    if (!camera || !m_clusterBuildShader) return;

    // Extract camera parameters
    glm::mat4 viewMatrix = camera->getViewMatrix();
    glm::mat4 projMatrix = camera->getProjectionMatrix();
    glm::mat4 invViewProj = glm::inverse(projMatrix * viewMatrix);

    // Calculate near and far planes
    m_clusterData.nearClip = 0.1f; // Could extract from camera if available
    m_clusterData.farClip = 100.0f; // Could extract from camera if available

    // Bind the cluster build shader
    m_clusterBuildShader->use();

    // Set shader uniforms
    m_clusterBuildShader->setMat4("viewProj", projMatrix * viewMatrix);
    m_clusterBuildShader->setMat4("invViewProj", invViewProj);
    m_clusterBuildShader->setFloat("nearClip", m_clusterData.nearClip);
    m_clusterBuildShader->setFloat("farClip", m_clusterData.farClip);
    m_clusterBuildShader->setUint("clusterDimX", m_clusterData.clusterDimX);
    m_clusterBuildShader->setUint("clusterDimY", m_clusterData.clusterDimY);
    m_clusterBuildShader->setUint("clusterDimZ", m_clusterData.clusterDimZ);

    // Bind the cluster AABB storage buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_clusterData.clusterAABBs);
    // Allocate buffer if not already done (2 vec4s per cluster - min and max)
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 m_clusterData.totalClusters * 2 * sizeof(glm::vec4),
                 nullptr, GL_DYNAMIC_DRAW);

    m_clusterBuildShader->bindShaderStorageBuffer(0, m_clusterData.clusterAABBs);

    // Dispatch compute shader
    uint32_t workGroupsX, workGroupsY, workGroupsZ;
    calculateComputeWorkGroups(workGroupsX, workGroupsY, workGroupsZ);

    m_clusterBuildShader->dispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader has finished
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Reset cluster light counts
    std::fill(m_clusterData.lightAssignmentCount.begin(),
              m_clusterData.lightAssignmentCount.end(), 0);
}

// Assign lights to clusters
void ClusteredRenderSystem::assignLightsToClusters()
{
    // Skip if no lights or clusters
    if (m_lights.empty() || m_clusterData.totalClusters == 0)
    {
        return;
    }

    // Set up uniforms for the compute shader
    m_lightAssignShader->use();
    m_lightAssignShader->setUint("lightCount", static_cast<uint32_t>(m_lights.size()));
    m_lightAssignShader->setUint("clusterCount", m_clusterData.totalClusters);
    m_lightAssignShader->setUint("maxLightsPerCluster", m_clusterData.maxLightsPerCluster);

    // Bind required buffers
    m_lightAssignShader->bindShaderStorageBuffer(0, m_clusterData.clusterAABBs);
    m_lightAssignShader->bindShaderStorageBuffer(1, m_clusterData.lightGrid);
    m_lightAssignShader->bindShaderStorageBuffer(2, m_clusterData.lightIndexList);
    m_lightAssignShader->bindShaderStorageBuffer(3, m_lightBuffer);

    // Reset light counter (first uint in lightIndexCounter)
    uint32_t zero = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_clusterData.lightGrid);
    glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, 0, sizeof(uint32_t), GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    // Calculate dispatch size
    uint32_t workGroupsX, workGroupsY, workGroupsZ;
    calculateComputeWorkGroups(workGroupsX, workGroupsY, workGroupsZ);

    // Dispatch compute shader
    m_lightAssignShader->dispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader finishes before proceeding
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Read back active cluster count for statistics (optional)
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_clusterData.lightGrid);
    uint32_t* counterPtr = static_cast<uint32_t*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
                                                                   0, sizeof(uint32_t),
                                                                   GL_MAP_READ_BIT));

    if (counterPtr)
    {
        m_clusterData.totalLightIndices = *counterPtr;
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ClusteredRenderSystem::assignDecalsToClusters()
{
    // Very similar to light assignment
    if (m_decals.empty() || m_clusterData.totalClusters == 0)
    {
        return;
    }

    // Set up uniforms for the compute shader
    m_decalAssignShader->use();
    m_decalAssignShader->setUint("decalCount", static_cast<uint32_t>(m_decals.size()));
    m_decalAssignShader->setUint("clusterCount", m_clusterData.totalClusters);
    m_decalAssignShader->setUint("maxDecalsPerCluster", m_clusterData.maxDecalsPerCluster);

    // Bind required buffers
    m_decalAssignShader->bindShaderStorageBuffer(0, m_clusterData.clusterAABBs);
    m_decalAssignShader->bindShaderStorageBuffer(1, m_clusterData.decalGrid);
    m_decalAssignShader->bindShaderStorageBuffer(2, m_clusterData.decalIndexList);
    m_decalAssignShader->bindShaderStorageBuffer(3, m_decalBuffer);

    // Reset counter
    uint32_t zero = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_clusterData.decalGrid);
    glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, 0, sizeof(uint32_t), GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    // Calculate dispatch size
    uint32_t workGroupsX, workGroupsY, workGroupsZ;
    calculateComputeWorkGroups(workGroupsX, workGroupsY, workGroupsZ);

    // Dispatch compute shader
    m_decalAssignShader->dispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader finishes before proceeding
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

// Render debug visualization of clusters
void ClusteredRenderSystem::renderDebugClusters(RenderContext& context)
{
    if (!m_debugClusterShader) return;

    m_debugClusterShader->use();

    m_debugClusterShader->setMat4("view", context.viewMatrix);
    m_debugClusterShader->setMat4("projection", context.projectionMatrix);

    // Bind cluster AABBs
    m_debugClusterShader->bindShaderStorageBuffer(0, m_clusterData.clusterAABBs);

    // Render wireframe
    GLint polygonMode;
    glGetIntegerv(GL_POLYGON_MODE, &polygonMode);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Draw instanced cubes for each cluster
    // This assumes we have a cube VAO bound and ready to render
    // In practice, you would need to create a cube VAO elsewhere and bind it here
    glDrawArraysInstanced(GL_TRIANGLES, 0, 36, m_clusterData.totalClusters);

    // Restore polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
}

void ClusteredRenderSystem::updateLightBuffer()
{
    // Skip if no lights
    if (m_lights.empty())
    {
        return;
    }

    // Calculate total size needed for light data
    const uint32_t floatsPerLight = 16; // Based on our shader packing (4 vec4s per light)
    const uint32_t totalFloats = m_lights.size() * floatsPerLight;

    // Prepare buffer
    std::vector<float> lightData(totalFloats);
    uint32_t offset = 0;

    // Update each light before packing
    for (Light* light : m_lights)
    {
        light->update(m_deltaTime);
        light->packLightData(lightData, offset);
    }

    // Upload to GPU
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_lightBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, lightData.size() * sizeof(float), lightData.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Store visible light count
    m_clusterData.visibleLightCount = m_lights.size();
}

void ClusteredRenderSystem::updateDecalBuffer()
{
    // Skip if no decals
    if (m_decals.empty())
    {
        return;
    }

    // Calculate total size needed for decal data
    const uint32_t floatsPerDecal = 28; // Based on our shader packing
    const uint32_t totalFloats = m_decals.size() * floatsPerDecal;

    // Prepare buffer
    std::vector<float> decalData(totalFloats);
    uint32_t offset = 0;

    // Pack each decal
    for (Decal* decal : m_decals)
    {
        decal->packDecalData(decalData, offset);
    }

    // Upload to GPU
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_decalBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, decalData.size() * sizeof(float), decalData.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

// Apply pixel art post-processing pass
void ClusteredRenderSystem::applyPixelArtPass(RenderTarget* source, RenderTarget* destination)
{
    if (!source || !destination || !m_pixelationShader) return;

    // Bind destination framebuffer
    destination->bind();

    // Set up pixelation shader
    m_pixelationShader->use();
    m_pixelationShader->setInt("sourceTexture", 0);
    m_pixelationShader->setInt("pixelSize", m_pixelSize);
    m_pixelationShader->setInt("snapToGrid", m_snapToGrid);

    // Set palette constraints if using a color palette
    if (m_paletteEnabled && m_paletteTexture)
    {
        m_pixelationShader->setBool("usePalette", true);
        m_pixelationShader->setInt("palette", 1);
        m_pixelationShader->setInt("paletteSize", m_paletteSize);

        // Bind palette texture
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, m_paletteTexture->getID());
    }
    else
    {
        m_pixelationShader->setBool("usePalette", false);
    }

    // Set dithering options
    if (m_ditheringEnabled && m_ditherPatternTexture)
    {
        m_pixelationShader->setBool("useDithering", true);
        m_pixelationShader->setInt("ditherPattern", 2);
        m_pixelationShader->setFloat("ditherStrength", m_ditherStrength);

        // Bind dither pattern texture
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_ditherPatternTexture->getID());
    }
    else
    {
        m_pixelationShader->setBool("useDithering", false);
    }

    // Bind source texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source->getColorTexture());

    // Render fullscreen quad
    renderFullscreenQuad();

    // Unbind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (m_paletteEnabled && m_paletteTexture)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, 0);
    }

    if (m_ditheringEnabled && m_ditherPatternTexture)
    {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

//Set up material for clustered rendering
void ClusteredRenderSystem::setupMaterialForClustering(Material* material, Shader* shader)
{
    // Bind the shader to ensure uniforms are set
    shader->use();

    // Set cluster data uniforms
    shader->setUint("clusterDimX", m_clusterData.clusterDimX);
    shader->setUint("clusterDimY", m_clusterData.clusterDimY);
    shader->setUint("clusterDimZ", m_clusterData.clusterDimZ);
    shader->setFloat("nearClip", m_clusterData.nearClip);
    shader->setFloat("farClip", m_clusterData.farClip);

    // Bind light grid and light index list
    shader->setInt("lightGrid", 10); // Use high texture unit for system textures
    shader->setInt("lightIndexList", 11);

    // Set environment parameters
    shader->setFloat("timeOfDay", m_timeOfDay);
    shader->setVec3("fogColor", m_weatherParams.fogColor);
    shader->setFloat("fogDensity", m_weatherParams.fogDensity);

    // Set atmosphere parameters
    shader->setVec3("skyTint", m_atmosphereSettings.skyTint);
    shader->setFloat("rayleighScattering", m_atmosphereSettings.rayleighScattering);
    shader->setFloat("mieScattering", m_atmosphereSettings.mieScattering);

    // Custom parameters can be set on the material directly
    material->setParameter("_ClusterEnabled", 1.0f);
}

// Calculate optimal workgroup sizes for compute shaders
void ClusteredRenderSystem::calculateComputeWorkGroups(uint32_t& workGroupsX, uint32_t& workGroupsY, uint32_t& workGroupsZ) const
{
    // Assuming 8x8x1 threads per work group - adjust based on your shaders
    workGroupsX = (m_clusterData.clusterDimX + 7) / 8;
    workGroupsY = (m_clusterData.clusterDimY + 7) / 8;
    workGroupsZ = m_clusterData.clusterDimZ; // One workgroup per Z-slice
}

// Set up default render pipeline
void ClusteredRenderSystem::setupDefaultRenderPipeline()
{
    // Add default render stages
    // These would be created elsewhere and added here

    // Add default post-processors
    // These would be created elsewhere and added here
}

// Add a light to the system
void ClusteredRenderSystem::addLight(Light* light)
{
    if (light && std::find(m_lights.begin(), m_lights.end(), light) == m_lights.end())
    {
        m_lights.push_back(light);
    }
}

// Remove a light from the system
void ClusteredRenderSystem::removeLight(Light* light)
{
    auto it = std::find(m_lights.begin(), m_lights.end(), light);
    if (it != m_lights.end())
    {
        m_lights.erase(it);
    }
}

// Add a decal to the system
void ClusteredRenderSystem::addDecal(Decal* decal)
{
    if (decal && std::find(m_decals.begin(), m_decals.end(), decal) == m_decals.end())
    {
        m_decals.push_back(decal);
    }
}

// Remove a decal from the system
void ClusteredRenderSystem::removeDecal(Decal* decal)
{
    auto it = std::find(m_decals.begin(), m_decals.end(), decal);
    if (it != m_decals.end())
    {
        m_decals.erase(it);
    }
}

// Set the palette manager
void ClusteredRenderSystem::setPaletteManager(PaletteManager* paletteManager)
{
    m_paletteManager = paletteManager;
}

// Get the palette manager
PaletteManager* ClusteredRenderSystem::getPaletteManager() const
{
    return m_paletteManager;
}

// Set time of day
void ClusteredRenderSystem::setTimeOfDay(float timeOfDay)
{
    m_timeOfDay = timeOfDay;
    // Update lighting based on time of day
    updateLightBuffer();
}

// Set weather conditions
void ClusteredRenderSystem::setWeatherConditions(const WeatherParameters& params)
{
    m_weatherParams = params;

    // Update relevant rendering parameters based on weather
    updateLightBuffer();
}

// Set atmosphere settings
void ClusteredRenderSystem::setAtmosphereSettings(const AtmosphereSettings& settings)
{
    m_atmosphereSettings = settings;

    // Update relevant rendering parameters based on atmosphere settings
    updateLightBuffer();
}

// Get texture manager
TextureManager* ClusteredRenderSystem::getTextureManager()
{
    return m_textureManager.get();
}

// Load a texture through the texture manager
Texture* ClusteredRenderSystem::loadTexture(const std::string& path, bool generateMipmaps, bool sRGB)
{
    if (!m_textureManager)
    {
        std::cerr << "Texture manager not initialized!" << std::endl;
        return nullptr;
    }

    Texture* texture = m_textureManager->getTexture(path, generateMipmaps, sRGB);

    // For pixel art, set appropriate filtering by default
    if (texture)
    {
        texture->setFilterMode(TextureFilterMode::NEAREST);
        texture->setPixelGridAlignment(m_snapToGrid);
    }

    return texture;
}

// Create an empty texture through the texture manager
Texture* ClusteredRenderSystem::createEmptyTexture(int width, int height, int channels, bool generateMipmaps, bool sRGB)
{
    if (!m_textureManager)
    {
        std::cerr << "Texture manager not initialized!" << std::endl;
        return nullptr;
    }

    Texture* texture = m_textureManager->createTexture(width, height, channels, generateMipmaps, sRGB);

    // For pixel art, set appropriate filtering by default
    if (texture)
    {
        texture->setFilterMode(TextureFilterMode::NEAREST);
        texture->setPixelGridAlignment(m_snapToGrid);
    }

    return texture;
}

// Create a material
Material* ClusteredRenderSystem::createMaterial(Shader* shader)
{
    if (!shader)
    {
        std::cerr << "Cannot create material with null shader!" << std::endl;
        return nullptr;
    }

    Material* material = new Material(shader);

    // Set up defaults for pixel art aesthetic
    material->setPixelSnapAmount(m_snapToGrid ? 0.8f : 0.0f);

    // Set up palette manager if available
    if (m_paletteManager)
    {
        material->setPaletteManager(m_paletteManager);
        material->setPaletteConstraint(m_paletteEnabled);
    }

    return material;
}

// Release a material
void ClusteredRenderSystem::releaseMaterial(Material* material)
{
    // Materials aren't tracked by the system, just delete it
    delete material;
}

// Set up material for clustered rendering
void ClusteredRenderSystem::bindMaterialToClusterSystem(Material* material)
{
    if (!material)
    {
        return;
    }

    Shader* shader = material->getShader();
    if (!shader)
    {
        return;
    }

    setupMaterialForClustering(material, shader);
}

// Add a renderable object
void ClusteredRenderSystem::addRenderableObject(RenderableObject* object)
{
    if (object && std::find(m_renderableObjects.begin(), m_renderableObjects.end(), object) == m_renderableObjects.end())
    {
        m_renderableObjects.push_back(object);
    }
}

// Remove a renderable object
void ClusteredRenderSystem::removeRenderableObject(RenderableObject* object)
{
    auto it = std::find(m_renderableObjects.begin(), m_renderableObjects.end(), object);
    if (it != m_renderableObjects.end())
    {
        m_renderableObjects.erase(it);
    }
}

// Reload all shaders
void ClusteredRenderSystem::reloadShaders()
{
    ShaderCache::reloadAll();
}

// Add a render stage
void ClusteredRenderSystem::addRenderStage(std::shared_ptr<RenderStage> stage)
{
    if (stage)
    {
        m_renderStages.push_back(stage);
    }
}

// Add a post-processor
void ClusteredRenderSystem::addPostProcessor(std::shared_ptr<PostProcessor> processor)
{
    if (processor)
    {
        m_postProcessors.push_back(processor);
    }
}

// Configure pixel art integration
void ClusteredRenderSystem::configureForPixelArt(int pixelSize, bool snapToGrid)
{
    m_pixelSize = std::max(1, pixelSize);
    m_snapToGrid = snapToGrid;

    // Recreate the final render target with the appropriate size
    int finalWidth = m_width / m_pixelSize;
    int finalHeight = m_height / m_pixelSize;
    m_finalRenderTarget = std::make_unique<RenderTarget>(finalWidth, finalHeight);

    std::cout << "Configured for pixel art: pixel size = " << m_pixelSize
              << ", snap to grid = " << (m_snapToGrid ? "yes" : "no") << std::endl;
}

// Set palette options
void ClusteredRenderSystem::setPaletteOptions(bool enabled, int paletteSize, Texture* paletteTexture)
{
    m_paletteEnabled = enabled;
    m_paletteSize = paletteSize;
    m_paletteTexture = paletteTexture;
}

// Set dithering options
void ClusteredRenderSystem::setDitheringOptions(bool enabled, float strength, Texture* patternTexture)
{
    m_ditheringEnabled = enabled;
    m_ditherStrength = strength;
    m_ditherPatternTexture = patternTexture;
}

// Enable/disable post-processing
void ClusteredRenderSystem::setPostProcessingEnabled(bool enabled)
{
    m_postProcessingEnabled = enabled;
}

// Get the main render target
RenderTarget* ClusteredRenderSystem::getMainRenderTarget()
{
    return m_mainRenderTarget.get();
}

// Get the final render target
RenderTarget* ClusteredRenderSystem::getFinalRenderTarget()
{
    return m_finalRenderTarget.get();
}

// Get the active cluster count
uint32_t ClusteredRenderSystem::getActiveClusterCount() const
{
    return m_clusterData.activeClusterCount;
}

// Get the visible light count
uint32_t ClusteredRenderSystem::getVisibleLightCount() const
{
    return m_clusterData.visibleLightCount;
}

// Get the current weather parameters
const WeatherParameters& ClusteredRenderSystem::getWeatherParameters() const
{
    return m_weatherParams;
}

// Get the current atmosphere settings
const AtmosphereSettings& ClusteredRenderSystem::getAtmosphereSettings() const
{
    return m_atmosphereSettings;
}

// Get the current time of day
float ClusteredRenderSystem::getTimeOfDay() const
{
    return m_timeOfDay;
}

// Calculate the z-plane equation for a specific cluster
glm::vec4 ClusteredRenderSystem::calculateZPlaneEquation(float zNear, float zFar, int clusterIndex) const
{
    // For orthographic projection, z-planes are linearly distributed from near to far
    float zStep = (zFar - zNear) / m_clusterData.clusterDimZ;
    float z = zNear + zStep * clusterIndex;

    // Return plane equation in the form (normal.x, normal.y, normal.z, distance)
    return glm::vec4(0.0f, 0.0f, 1.0f, -z);
}

// Calculate the cluster index from x, y, z coordinates
uint32_t ClusteredRenderSystem::calculateClusterIndex(uint32_t x, uint32_t y, uint32_t z) const
{
    return x + y * m_clusterData.clusterDimX + z * m_clusterData.clusterDimX * m_clusterData.clusterDimY;
}
