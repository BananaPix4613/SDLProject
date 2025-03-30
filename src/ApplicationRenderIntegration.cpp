#include "ApplicationRenderIntegration.h"
#include "Application.h"

bool ApplicationRenderIntegration::initialize()
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
    const RenderSettings& settings = application->getRenderSettings();

    debugStage->setShowChunkBoundaries(settings.showChunkBoundaries);
    debugStage->setShowGrid(settings.showGridLines);
    debugStage->setShowFrustumWireframe(settings.showFrustumWireframe);
    renderSystem->addRenderStage(debugStage);

    // Add post-processors
    if (settings.enableBloom)
    {
        renderSystem->addPostProcessor(std::make_shared<BloomPostProcessor>());
    }

    if (settings.enableSSAO)
    {
        renderSystem->addPostProcessor(std::make_shared<SSAOPostProcessor>());
    }

    // Always add tonemap as the final post-processor
    renderSystem->addPostProcessor(std::make_shared<TonemapPostProcessor>());

    return true;
}

void ApplicationRenderIntegration::update()
{
    if (!renderSystem || !application) return;

    // Update voxel object when grid changes
    if (voxelObject)
    {
        // Set render distance from settings
        const RenderSettings& settings = application->getRenderSettings();
        voxelObject->setRenderDistance(500.0f); // default maxViewDistance value

        // Update the voxel object
        RenderContext context;
        context.camera = application->getCamera();
        context.viewMatrix = application->getCamera()->getViewMatrix();
        context.projectionMatrix = application->getCamera()->getProjectionMatrix();
        voxelObject->prepare(context);
    }

    // Update render settings
    updateRenderSettings();
}

void ApplicationRenderIntegration::render()
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

void ApplicationRenderIntegration::resizeViewport(int width, int height)
{
    if (renderSystem)
    {
        renderSystem->resize(width, height);
    }
}

void ApplicationRenderIntegration::updateRenderSettings()
{
    if (!renderSystem) return;

    const RenderSettings& settings = application->getRenderSettings();

    // Update shadow settings
    renderSystem->setEnableShadows(settings.enableShadows);

    // Update post-processing settings
    renderSystem->setEnablePostProcessing(settings.enablePostProcessing);

    if (debugStage)
    {
        debugStage->setShowChunkBoundaries(settings.showChunkBoundaries);
        debugStage->setShowGrid(settings.showGridLines);
        debugStage->setShowFrustumWireframe(settings.showFrustumWireframe);

        // Update grid parameters
        CubeGrid* grid = application->getGrid();
        debugStage->setGridParameters(
            grid->getMinBounds(),
            grid->getMaxBounds(),
            grid->getSpacing()
        );
    }
}
