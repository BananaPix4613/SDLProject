#pragma once

#include "RenderSystem.h"
#include "VoxelObject.h"
#include "RenderStages.h"
#include "PostProcessors.h"
#include "UIManager.h"
#include "Camera.h"

// Forward declarations
class Application;

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

    bool initialize();
    void update();
    void render();
    void resizeViewport(int width, int height);
    void updateRenderSettings();

    // Access to render system components
    RenderSystem* getRenderSystem() const { return renderSystem; }
    VoxelObject* getVoxelObject() const { return voxelObject; }

private:
    Application* application;
    RenderSystem* renderSystem;
    VoxelObject* voxelObject;

    // Internal cached debug renderer
    std::shared_ptr<DebugStage> debugStage;
};