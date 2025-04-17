#define GLM_ENABLE_EXPERIMENTAL

#include "Application.h"
#include "InputManager.h"
#include "EntityManager.h"
#include "PhysicsSystem.h"
#include "AudioSystem.h"
#include "Editor.h"
#include "IsometricCamera.h"
#include "PaletteManager.h"
#include <iostream>

// Initialize static instance
Application* Application::s_instance = nullptr;

// Singleton access
Application& Application::getInstance()
{
    if (!s_instance)
    {
        s_instance = new Application();
    }
    return *s_instance;
}

Application::Application()
    : m_window(nullptr),
    m_activeCamera(nullptr),
    m_deltaTime(0.0f),
    m_lastFrameTime(0.0f),
    m_totalTime(0.0f),
    m_isRunning(false),
    m_isWindowMinimized(false)
{
}

Application::~Application()
{
    shutdown();
}

bool Application::initialize()
{
    std::cout << "Initializing application..." << std::endl;

    // Initialize window first
    if (!initializeWindow())
    {
        std::cerr << "Failed to initialize window" << std::endl;
        return false;
    }

    // Initialize memory manager
    m_memoryManager = std::make_unique<MemoryManager>();

    // Initialize with a reasonable pool size (adjust as needed)
    if (!m_memoryManager->initialize(64 * 1024 * 1024))
    { // 64MB initial pool
        std::cerr << "Failed to initialize memory manager" << std::endl;
        return false;
    }

    // Register common type pools
    m_memoryManager->registerTypePool<Entity>(100, 20);
    m_memoryManager->registerTypePool<Component>(200, 50);
    m_memoryManager->registerTypePool<Light>(50, 10);
    m_memoryManager->registerTypePool<Decal>(50, 10);

    // Initialize event system
    m_eventSystem = std::make_unique<EventSystem>();
    m_eventSystem->initialize();

    // Initialize renderer
    if (!initializeRenderer())
    {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }

    // Initialize remaining subsystems
    if (!initializeSubsystems())
    {
        std::cerr << "Failed to initialize subsystems" << std::endl;
        return false;
    }

    // Set up default scene
    setupDefaultScene();

    // Initialize timing
    m_lastFrameTime = static_cast<float>(glfwGetTime());
    m_isRunning = true;

    std::cout << "Application initialization complete" << std::endl;
    return true;
}

bool Application::initializeWindow()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    m_window = glfwCreateWindow(
        m_renderSettings.windowWidth,
        m_renderSettings.windowHeight,
        "3D Pixel Art Engine",
        nullptr,
        nullptr
    );

    if (!m_window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    // Make context current
    glfwMakeContextCurrent(m_window);

    // Load OpenGL functions with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // Set up callbacks
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
    glfwSetWindowIconifyCallback(m_window, windowMinimizeCallback);

    // Set VSync
    glfwSwapInterval(m_renderSettings.vsync ? 1 : 0);

    return true;
}

bool Application::initializeRenderer()
{
    // Create the clustered render pipeline
    m_renderer = std::make_unique<ClusteredRenderSystem>();

    // Initialize the renderer
    if (!m_renderer->initialize())
    {
        std::cerr << "Failed to initialize ClusteredRenderSystem" << std::endl;
        return false;
    }

    // Configure clustering based on render settings
    m_renderer->configureClustering(
        m_renderSettings.clusterDimX,
        m_renderSettings.clusterDimY,
        m_renderSettings.clusterDimZ
    );

    // Configure pixel art settings
    m_renderer->configureForPixelArt(
        m_renderSettings.pixelSize,
        true // Always snap to grid for pixel art
    );

    // Set up initial atmosphere and time of day
    m_renderer->setTimeOfDay(m_renderSettings.timeOfDay);

    // Configure window size for the renderer
    m_renderer->resize(m_renderSettings.windowWidth, m_renderSettings.windowHeight);

    // Create and configure PaletteManager
    auto textureManager = m_renderer->getTextureManager();
    auto paletteManager = new PaletteManager(textureManager);

    // Generate default palette
    paletteManager->generatePalette(PaletteGenerationMethod::HSV_DISTRIBUTED, "Default", m_renderSettings.paletteSize);
    paletteManager->setActivePalette("Default");
    paletteManager->enablePaletteConstraint(m_renderSettings.enablePaletteConstraint);
    paletteManager->setDitheringPattern(DitheringPattern::ORDERED_4X4);
    paletteManager->setDitheringStrength(m_renderSettings.ditherStrength);

    // Set palette manager in renderer
    m_renderer->setPaletteManager(paletteManager);

    // Set dithering options based on render settings
    m_renderer->setDitheringOptions(
        m_renderSettings.enableDithering,
        m_renderSettings.ditherStrength,
        paletteManager->getDitherPatternTexture()
    );

    // Enable/disable post-processing
    m_renderer->setPostProcessingEnabled(m_renderSettings.enablePostProcessing);

    return true;
}

bool Application::initializeSubsystems()
{
    // Create the voxel grid
    m_grid = std::make_unique<CubeGrid>(32, 1.0f); // 32x32x32 grid with 1.0 spacing

    // Create a scene
    m_scene = std::make_unique<Scene>("MainScene", m_eventSystem.get());
    if (!m_scene->initialize())
    {
        std::cerr << "Failed to initialize scene" << std::endl;
        return false;
    }

    // Initialize entity manager with scene and event system
    m_entityManager = std::make_unique<EntityManager>(m_scene.get(), m_eventSystem.get());
    if (!m_entityManager->initialize())
    {
        std::cerr << "Failed to initialize entity manager" << std::endl;
        return false;
    }

    // Initialize physics system with scene and grid
    m_physicsSystem = std::make_unique<PhysicsSystem>();
    if (!m_physicsSystem->initialize(m_scene.get()))
    {
        std::cerr << "Failed to initialize physics system" << std::endl;
        return false;
    }
    m_physicsSystem->setCubeGrid(m_grid.get());

    // Initialize audio system with scene and event system
    m_audioSystem = std::make_unique<AudioSystem>();
    if (!m_audioSystem->initialize(m_scene.get(), m_eventSystem.get()))
    {
        std::cerr << "Failed to initialize audio system" << std::endl;
        return false;
    }
    m_audioSystem->setCubeGrid(m_grid.get());

    // Initialize UI manager
    m_uiManager = std::make_unique<UIManager>(this);
    if (!m_uiManager->initialize(m_window))
    {
        std::cerr << "Failed to initialize UI manager" << std::endl;
        return false;
    }

    // Set scene texture to the final render target for UI
    GLuint finalTexture = m_renderer->getFinalRenderTarget()->getColorTexture();
    m_uiManager->setSceneTexture(finalTexture);

    // Initialize input manager
    m_inputManager = std::make_unique<InputManager>(this);
    if (!m_inputManager->initialize())
    {
        std::cerr << "Failed to initialize input manager" << std::endl;
        return false;
    }

    // Initialize editor with all required systems
    m_editor = std::make_unique<Editor>();
    if (!m_editor->initialize())
    {
        std::cerr << "Failed to initialize editor" << std::endl;
        return false;
    }

    // Set active scene in editor
    m_editor->setActiveScene(m_scene.get());
    m_editor->setUIManager(m_uiManager.get());

    // Setup performance monitoring
    //m_performanceMetrics.frameTimeHistory.resize(60, 0.0f); // Track last 60 frames
    //m_performanceMetrics.systemTimings.resize(SystemTimingType::COUNT, 0.0f);

    return true;
}

void Application::setupDefaultScene()
{
    // Create an isometric camera
    m_activeCamera = new IsometricCamera(
        static_cast<float>(m_renderSettings.windowWidth) /
        static_cast<float>(m_renderSettings.windowHeight)
    );

    // Configure camera
    IsometricCamera* isoCamera = static_cast<IsometricCamera*>(m_activeCamera);
    isoCamera->setPosition(glm::vec3(30.0f, 30.0f, 30.0f));
    isoCamera->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    isoCamera->setZoom(1.0f);
    isoCamera->setOrthoSize(20.0f);

    // Add some default lighting
    Light* sunLight = new Light(Light::DIRECTIONAL);
    sunLight->setDirection(glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f)));
    sunLight->setColor(glm::vec3(1.0f, 0.95f, 0.8f));
    sunLight->setIntensity(3.0f);
    sunLight->setCastShadows(true);
    m_renderer->addLight(sunLight);

    // Add ambient fill light
    Light* fillLight = new Light(Light::DIRECTIONAL);
    fillLight->setDirection(glm::normalize(glm::vec3(1.0f, 0.2f, 0.5f)));
    fillLight->setColor(glm::vec3(0.5f, 0.6f, 0.8f));
    fillLight->setIntensity(0.5f);
    fillLight->setCastShadows(false);
    m_renderer->addLight(fillLight);

    // Create the default voxel object
    VoxelObject* voxelObject = new VoxelObject(m_grid.get());

    // Configure voxel rendering
    voxelObject->setAmbientOcclusion(true, 0.5f);
    voxelObject->setEdgeHighlighting(true, 1.0f, glm::vec3(0.1f, 0.1f, 0.2f));

    // Add to renderer
    m_renderer->addRenderableObject(voxelObject);
}

void Application::run()
{
    if (!m_isRunning)
    {
        std::cerr << "Cannot run application, not initialized properly" << std::endl;
        return;
    }

    std::cout << "Starting main loop" << std::endl;

    // Main loop
    while (m_isRunning && !glfwWindowShouldClose(m_window))
    {
        // Calculate delta time
        float currentTime = static_cast<float>(glfwGetTime());
        m_deltaTime = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;
        m_totalTime += m_deltaTime;

        // Skip update if window is minimized
        if (!m_isWindowMinimized)
        {
            processInput();
            update();
            render();
        }

        // Poll events and swap buffers
        glfwPollEvents();
        glfwSwapBuffers(m_window);
    }
}

void Application::update()
{
    // Update renderer state
    m_renderer->update(m_deltaTime);

    // Update physics
    m_physicsSystem->update(m_deltaTime);

    // Update entities
    m_entityManager->update(m_deltaTime);

    // Update audio
    m_audioSystem->update(m_deltaTime);

    // Update UI manager
    m_uiManager->updateNotifications(m_deltaTime);

    // Update editor if active
    m_editor->update(m_deltaTime);
}

void Application::render()
{
    // Begin UI frame
    m_uiManager->beginFrame();

    // Render the scene using clustered rendering
    if (m_activeCamera)
    {
        m_renderer->render(m_activeCamera);
    }

    // Render UI
    m_editor->render();
    m_uiManager->render();
}

void Application::processInput()
{
    m_inputManager->update();

    // Example of basic input handling
    if (m_inputManager->isKeyPressed(GLFW_KEY_ESCAPE))
    {
        quit();
    }

    // Camera controls would be processed here
    // or via a separate camera controller component
}

void Application::shutdown()
{
    std::cout << "Shutting down application..." << std::endl;

    // Clean up in reverse order of initialization

    // Delete camera (not owned by a unique_ptr)
    if (m_activeCamera)
    {
        delete m_activeCamera;
        m_activeCamera = nullptr;
    }

    // Clean up subsystems
    m_editor.reset();
    m_audioSystem.reset();
    m_physicsSystem.reset();
    m_entityManager.reset();
    m_inputManager.reset();
    m_uiManager.reset();

    // Clean up renderer and related systems
    if (m_renderer)
    {
        // Delete palette manager (created directly, not with unique_ptr)
        PaletteManager* paletteManager = m_renderer->getPaletteManager();
        if (paletteManager)
        {
            delete paletteManager;
        }

        // Reset renderer
        m_renderer.reset();
    }

    // Clean up grid
    m_grid.reset();

    // Clean up core systems
    m_eventSystem.reset();
    m_memoryManager.reset();

    // Destroy window and terminate GLFW
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();

    m_isRunning = false;
    std::cout << "Application shutdown complete" << std::endl;
}

float Application::getDeltaTime() const
{
    return m_deltaTime;
}

float Application::getTotalTime() const
{
    return m_totalTime;
}

Camera* Application::getCamera() const
{
    return m_activeCamera;
}

void Application::setCamera(Camera* camera)
{
    // Clean up old camera if owned by application
    if (m_activeCamera)
    {
        delete m_activeCamera;
    }

    m_activeCamera = camera;
}

RenderSettings& Application::getRenderSettings()
{
    return m_renderSettings;
}

const RenderSettings& Application::getRenderSettings() const
{
    return m_renderSettings;
}

ClusteredRenderSystem* Application::getRenderer() const
{
    return m_renderer.get();
}

CubeGrid* Application::getGrid() const
{
    return m_grid.get();
}

UIManager* Application::getUIManager() const
{
    return m_uiManager.get();
}

InputManager* Application::getInputManager() const
{
    return m_inputManager.get();
}

EntityManager* Application::getEntityManager() const
{
    return m_entityManager.get();
}

PhysicsSystem* Application::getPhysicsSystem() const
{
    return m_physicsSystem.get();
}

AudioSystem* Application::getAudioSystem() const
{
    return m_audioSystem.get();
}

EventSystem* Application::getEventSystem() const
{
    return m_eventSystem.get();
}

MemoryManager* Application::getMemoryManager() const
{
    return m_memoryManager.get();
}

Editor* Application::getEditor() const
{
    return m_editor.get();
}

GLFWwindow* Application::getWindow() const
{
    return m_window;
}

void Application::setWindowTitle(const std::string& title)
{
    if (m_window)
    {
        glfwSetWindowTitle(m_window, title.c_str());
    }
}

void Application::setWindowSize(int width, int height)
{
    if (m_window)
    {
        // Update render settings
        m_renderSettings.windowWidth = width;
        m_renderSettings.windowHeight = height;

        // Set window size
        glfwSetWindowSize(m_window, width, height);

        // Explicitly resize GL viewport
        glViewport(0, 0, width, height);

        // Update renderer
        if (m_renderer)
        {
            m_renderer->resize(width, height);
        }

        // Update camera aspect ratio
        if (m_activeCamera)
        {
            m_activeCamera->setAspectRatio(
                static_cast<float>(width) / static_cast<float>(height)
            );
        }
    }
}

bool Application::isWindowMinimized() const
{
    return m_isWindowMinimized;
}

bool Application::isRunning() const
{
    return m_isRunning;
}

void Application::quit()
{
    m_isRunning = false;
}

void Application::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    // Get application instance from window user pointer
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->setWindowSize(width, height);
    }
}

void Application::windowMinimizeCallback(GLFWwindow* window, int minimized)
{
    // Get application instance from window user pointer
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->m_isWindowMinimized = minimized != 0;
    }
}