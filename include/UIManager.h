#pragma once

#include "ImGuiWrapper.h"
#include "Application.h"
#include "CubeGrid.h"
#include "Profiler.h"
#include "RenderSettings.h"
#include <string>
#include <vector>
#include <functional>

// Forward declaration
class Application;

// Struct to hold notification data
struct Notification
{
    std::string message;
    float timeRemaining;
    bool isError;

    Notification(const std::string& msg, bool error, float duration = 3.0f)
        : message(msg), timeRemaining(duration), isError(error) {}
};

class UIManager
{
public:
    UIManager(Application* application);
    ~UIManager();

    bool initialize(GLFWwindow* window);
    void shutdown();

    void beginFrame();
    void render();

    // Main UI rendering methods
    void renderMainMenuBar();
    void renderDockSpace();
    void renderPanels();
    void renderNotifications();

    // Methods to show/hide specific panels
    void toggleControlPanel() { showControlPanel = !showControlPanel; }
    void toggleSettingsPanel() { showSettingsPanel = !showSettingsPanel; }
    void toggleGridNavigationPanel() { showGridNavigationPanel = !showGridNavigationPanel; }
    void toggleProfilerPanel() { showProfilerPanel = !showProfilerPanel; }

    // Menu-specific rendering methods
    void renderFileMenu();
    void renderEditMenu();
    void renderViewMenu();
    void renderToolsMenu();
    void renderHelpMenu();

    // Panel-specific rendering methods
    void renderControlPanel();
    void renderSettingsPanel();
    void renderRenderSettingsSection();
    void renderGridNavigationPanel();

    // Dialog rendering methods
    void renderSaveFileDialog();
    void renderLoadFileDialog();
    void renderSettingsDialog();
    void renderConfirmationDialog();

    // Notification system
    void addNotification(const std::string& message, bool isError = false);
    void updateNotifications(float deltaTime);

    // Getters/setters for UI state
    void setShowUI(bool show) { showUI = show; }
    bool getShowUI() const { return showUI; }

    // Input state for editing
    void setSelectedCubeCoords(int x, int y, int z)
    {
        selectedCubeX = x;
        selectedCubeY = y;
        selectedCubeZ = z;
    }

    void getSelectedCubeCoords(int& x, int& y, int& z) const
    {
        x = selectedCubeX;
        y = selectedCubeY;
        z = selectedCubeZ;
    }

    // Set current dialog to show (empty string for no dialog)
    void setCurrentDialog(const std::string& dialogName) { currentDialogName = dialogName; }
    std::string getCurrentDialog() const { return currentDialogName; }

private:
    // Application reference for interacting with the game systems
    Application* app;

    // ImGui wrapper
    ImGuiWrapper* imGui;

    // UI state variables
    bool showUI;
    bool showMainMenuBar;
    bool showDockSpace;
    bool showControlPanel;
    bool showSettingsPanel;
    bool showGridNavigationPanel;
    bool showProfilerPanel;
    bool isEditing;

    // Window size
    int windowWidth, windowHeight;

    // Selected cube
    int selectedCubeX, selectedCubeY, selectedCubeZ;
    glm::vec3 selectedCubeColor;
    int brushSize;

    // View distance for chunks
    int chunkViewDistance;
    float maxViewDistance;
    bool useInstanceCache;
    bool perCubeCulling;
    int batchSize;

    // Auto-save settings
    bool enableAutoSave;
    int autoSaveInterval; // In minutes
    std::string autoSaveFolder;

    // Dialog system
    std::string currentDialogName; // Current dialog being shown
    bool showConfirmDialog;
    std::string confirmMessage;
    std::function<void()> confirmCallback;

    // Notification system
    std::vector<Notification> notifications;

    // UI customization
    ImVec4 themeColors[ImGuiCol_COUNT];
    bool useDarkTheme;

    // Helper methods
    void setupTheme(bool darkTheme);
    void showConfirmationDialog(const std::string& message, std::function<void()> onConfirm);
    void resizeWindowPreset(int width, int height);
};