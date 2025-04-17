#pragma once

#include <string>
#include <memory>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "ClusteredRenderSystem.h"
#include "CubeGrid.h"
#include "UIManager.h"
#include "EventSystem.h"
#include "MemoryManager.h"

// Forward declarations
class InputManager;
class EntityManager;
class PhysicsSystem;
class AudioSystem;
class Editor;

// Render settings for configuration
struct RenderSettings
{
    int windowWidth = 1280;
    int windowHeight = 720;
    bool vsync = true;
    int pixelSize = 2;            // For pixel art aesthetic
    bool enablePaletteConstraint = true;
    int paletteSize = 64;
    bool enableDithering = true;
    float ditherStrength = 0.5f;
    bool enablePostProcessing = true;
    int clusterDimX = 16;
    int clusterDimY = 8;
    int clusterDimZ = 24;
    float timeOfDay = 12.0f;      // Noon by default
    bool debugClusters = false;
    bool debugChunks = false;
};

class Application
{
public:
    // Singleton access
    static Application& getInstance();

    // Core application
    bool initialize();
    void run();
    void shutdown();

    // Time management
    float getDeltaTime() const;
    float getTotalTime() const;

    // Camera access
    Camera* getCamera() const;
    void setCamera(Camera* camera);

    // Configuration access
    RenderSettings& getRenderSettings();
    const RenderSettings& getRenderSettings() const;

    // Subsystem access
    ClusteredRenderSystem* getRenderer() const;
    CubeGrid* getGrid() const;
    UIManager* getUIManager() const;
    InputManager* getInputManager() const;
    EntityManager* getEntityManager() const;
    PhysicsSystem* getPhysicsSystem() const;
    AudioSystem* getAudioSystem() const;
    EventSystem* getEventSystem() const;
    MemoryManager* getMemoryManager() const;
    Editor* getEditor() const;

    // Window management
    GLFWwindow* getWindow() const;
    void setWindowTitle(const std::string& title);
    void setWindowSize(int width, int height);
    bool isWindowMinimized() const;

    // Application status
    bool isRunning() const;
    void quit();

private:
    Application();  // Private constructor for singleton
    ~Application(); // Private destructor for singleton

    // Singleton instance
    static Application* s_instance;

    // Core systems
    GLFWwindow* m_window;
    std::unique_ptr<ClusteredRenderSystem> m_renderer;
    std::unique_ptr<CubeGrid> m_grid;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<UIManager> m_uiManager;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<EntityManager> m_entityManager;
    std::unique_ptr<PhysicsSystem> m_physicsSystem;
    std::unique_ptr<AudioSystem> m_audioSystem;
    std::unique_ptr<EventSystem> m_eventSystem;
    std::unique_ptr<MemoryManager> m_memoryManager;
    std::unique_ptr<Editor> m_editor;

    // Camera (owned by scene, not application)
    Camera* m_activeCamera;

    // Timing
    float m_deltaTime;
    float m_lastFrameTime;
    float m_totalTime;

    // Settings
    RenderSettings m_renderSettings;
    bool m_isRunning;
    bool m_isWindowMinimized;

    // Private initialization helpers
    bool initializeWindow();
    bool initializeRenderer();
    bool initializeSubsystems();
    void setupDefaultScene();

    // Event callbacks
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void windowMinimizeCallback(GLFWwindow* window, int minimized);

    // Update and render
    void update();
    void render();
    void processInput();
};