#pragma once

#include "Application.h"
#include "RenderSystem.h"
#include "VoxelObject.h"
#include "RenderStages.h"
#include "PostProcessors.h"
#include "Camera.h"

// Helper class to integrate the new rendering system with the existing Application class
class ApplicationRenderIntegration
{
public:
    ApplicationRenderIntegration(Application* app)
        : application(app), renderSystem(nullptr), voxelObject(nullptr)
    {

    }

    ~ApplicationRenderIntegration()
    {
        // Clean up render system
        if (renderSystem)
        {
            renderSystem->shutdown();
            delete renderSystem;
            renderSystem = nullptr;
        }

        // VoxelObject is owned by renderSystem, no need to delete it separately
        voxelObject = nullptr;
    }

    bool initialize()
    {
        if (!application) return false;

        // Create render system
        renderSystem = new RenderSystem();

        // Get application window size
        int width, height;
        application->getWindowSize(width, height);

        // Initialize render system with app's window size
        renderSystem->resize(width, height);
        renderSystem->initialize();

        // Create shaders
        Shader* mainShader = renderSystem->createShader("Main", "shaders/VertexShader.glsl", "shaders/FragmentShader.glsl");
        Shader* shadowShader = renderSystem->createShader("Shadow", "shaders/ShadowMappingVertexShader.glsl", "shaders/ShadowMappingFragmentShader.glsl");
        Shader* instancedShader = renderSystem->createShader("Instanced", "shaders/InstancedVertexShader.glsl", "shaders/InstancedFragmentShader.glsl");
        Shader* instancedShadowShader = renderSystem->createShader("InstancedShadow", "shaders/InstancedShadowVertexShader.glsl", "shaders/InstancedShadowFragmentShader.glsl");
        
        // Create voxel object for the grid
        voxelObject = new VoxelObject(application->getGrid());

        // Create materials
        Material* mainMaterial = new Material(instancedShader);
        Material* shadowMaterial = new Material(instancedShadowShader);

        // Set materials for voxel object
        voxelObject->setMaterial(mainMaterial);
        voxelObject->setShadowMaterial(shadowMaterial);

        // Add voxel object to render system
        renderSystem->addRenderableObject(voxelObject);

        // Add render stages
        renderSystem->addRenderStage(std::make_shared<ShadowStage>());
        renderSystem->addRenderStage(std::make_shared<SkyboxStage>());
        renderSystem->addRenderStage(std::make_shared<GeometryStage>());

        // Create debug stage
        auto debugStage = std::make_shared<DebugStage>();
        debugStage->setShowChunkBoundaries(application->getRenderSettings().showChunkBoundaries);
        debugStage->setShowGrid(application->getRenderSettings().showGridLines);
        debugStage->setShowFrustumWireframe(application->getRenderSettings().showFrustumWireframe);
        renderSystem->addRenderStage(debugStage);

        // Add post-processors
        if (application->getRenderSettings().enableBloom)
        {
            renderSystem->addPostProcessor(std::make_shared<BloomPostProcessor>());
        }

        if (application->getRenderSettings().enableSSAO)
        {
            renderSystem->addPostProcessor(std::make_shared<SSAOPostProcessor>());
        }

        // Always add tonemap as the final post-processor
        renderSystem->addPostProcessor(std::make_shared<TonemapPostProcessor>());

        return true;
    }

    void update()
    {
        if (!renderSystem || !application) return;

        // Update voxel object when grid changes
        if (voxelObject)
        {
            voxelObject->onGridChanged();

            // Set render distance
            voxelObject->setRenderDistance(application->getRenderSettings().maxViewDistance);
        }

        // Update render settings
        updateRenderSettings();
    }

    void render()
    {
        if (!renderSystem || !application) return;

        // Perform rendering
        renderSystem->render(application->getCamera());

        // Get render statistics
        if (voxelObject)
        {
            // Update visible cube count in application
            application->setVisibleCubeCount(voxelObject->getVisibleCubes());
        }

        // Get the final rendered texture for UI display
        unsigned int finalTexture = renderSystem->getFinalRenderTarget()->getColorTexture();
        application->getUIManager()->setSceneTexture(finalTexture);
    }

    void resizeViewport(int width, int height)
    {
        if (renderSystem)
        {
            renderSystem->resize(width, height);
        }
    }

    void updateRenderSettings()
    {
        if (!renderSystem) return;

        const RenderSettings& settings = application->getRenderSettings();

        // Update shadow settings
        renderSystem->setEnableShadows(settings.enableShadows);

        // Update post-processing settings
        renderSystem->setEnablePostProcessing(settings.enablePostProcessing);

        // Update debug stage settings
        std::shared_ptr<DebugStage> debugStage =
            std::dynamic_pointer_cast<DebugStage>(renderSystem->getRenderStage("Debug"));

        if (debugStage)
        {
            debugStage->setShowChunkBoundaries(settings.showChunkBoundaries);
            debugStage->setShowGrid(settings.showGridLines);
            debugStage->setShowFrustumWireframe(settings.showFrustumWireframe);

            // Update grid parameters
            debugStage->setGridParameters(
                application->getGrid()->getMinBounds(),
                application->getGrid()->getMaxBounds(),
                application->getGrid()->getSpacing()
            );

            // Update chunk data
            debugStage->setChunkData(
                application->getGrid()->getChunks(),
                application->getGrid()->getSpacing()
            );
        }
    }

    // Access to render system components
    RenderSystem* getRenderSystem() const { return renderSystem; }
    VoxelObject* getVoxelObject() const { return voxelObject; }

private:
    Application* application;
    RenderSystem* renderSystem;
    VoxelObject* voxelObject;
};