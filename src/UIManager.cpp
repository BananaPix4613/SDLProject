#include "UIManager.h"
#include "FileDialog.h"
#include "GridSerializer.h"
#include <iostream>
#include <ctime>

UIManager::UIManager(Application* application)
    : app(application),
    imGui(nullptr),
    showUI(true),
    showMainMenuBar(true),
    showDockSpace(true),
    showControlPanel(true),
    showSettingsPanel(true),
    showGridNavigationPanel(true),
    showProfilerPanel(true),
    isEditing(false),
    windowWidth(1280),
    windowHeight(720),
    selectedCubeX(-1),
    selectedCubeY(-1),
    selectedCubeZ(-1),
    selectedCubeColor(0.5f, 0.5f, 1.0f),
    brushSize(1),
    chunkViewDistance(5),
    maxViewDistance(500.0f),
    useInstanceCache(true),
    perCubeCulling(true),
    batchSize(10000),
    enableAutoSave(false),
    autoSaveInterval(5),
    autoSaveFolder(""),
    currentDialogName(""),
    showConfirmDialog(false),
    useDarkTheme(true)
{
    
}

UIManager::~UIManager()
{
    delete imGui;
}

bool UIManager::initialize(GLFWwindow* window)
{
    // Create ImGui wrapper
    imGui = new ImGuiWrapper();
    if (!imGui->initialize(window))
    {
        std::cerr << "Failed to initialize ImGui wrapper." << std::endl;
        return false;
    }

    // Set up theme
    setupTheme(useDarkTheme);

    return true;
}

void UIManager::shutdown()
{
    if (imGui)
    {
        imGui->shutdown();
    }
}

void UIManager::beginFrame()
{
    if (imGui && showUI)
    {
        imGui->newFrame();

        // If we want docking, set it up
        if (showDockSpace)
        {
            renderDockSpace();
        }

        // Render main menu bar if enabled
        if (showMainMenuBar)
        {
            renderMainMenuBar();
        }

        // Render all panels
        renderPanels();

        // Render notifications
        renderNotifications();

        // Render current dialog if any
        if (!currentDialogName.empty())
        {
            if (currentDialogName == "SaveFile")
            {
                renderSaveFileDialog();
            }
            else if (currentDialogName == "LoadFile")
            {
                renderLoadFileDialog();
            }
            else if (currentDialogName == "Settings")
            {
                renderSettingsDialog();
            }
        }
        
        // Render confirmation dialog if needed
        if (showConfirmDialog)
        {
            ImGui::OpenPopup("Confirm##dialog");
            showConfirmDialog = false;
        }

        if (ImGui::BeginPopupModal("Confirm##dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%s", confirmMessage.c_str());
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                // Execute callback if provided
                if (confirmCallback)
                {
                    confirmCallback();
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

void UIManager::render()
{
    if (imGui && showUI)
    {
        imGui->render();
    }
}

void UIManager::renderMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        // File menu
        if (ImGui::BeginMenu("File"))
        {
            renderFileMenu();
            ImGui::EndMenu();
        }

        // Edit menu
        if (ImGui::BeginMenu("Edit"))
        {
            renderEditMenu();
            ImGui::EndMenu();
        }

        // View menu
        if (ImGui::BeginMenu("View"))
        {
            renderViewMenu();
            ImGui::EndMenu();
        }

        // Tools menu
        if (ImGui::BeginMenu("Tools"))
        {
            renderToolsMenu();
            ImGui::EndMenu();
        }

        // Help menu
        if (ImGui::BeginMenu("Help"))
        {
            renderHelpMenu();
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void UIManager::renderDockSpace()
{
    // Get viewport size
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    // Window flags
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // Push styles for dockspace window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    // Create the dockspace window
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    }

    // End the dockspace window
    ImGui::End();
}

void UIManager::renderPanels()
{
    // Render main panels if they are visible
    if (showControlPanel)
    {
        renderControlPanel();
    }

    if (showSettingsPanel)
    {
        renderSettingsPanel();
    }

    if (showGridNavigationPanel)
    {
        renderGridNavigationPanel();
    }

    if (showProfilerPanel)
    {
        app->getProfiler().drawImGuiWindow();
    }
}

void UIManager::renderNotifications()
{
    if (notifications.empty()) return;

    float PADDING = 10.0f;
    int windowWidth, windowHeight;
    app->getWindowSize(windowWidth, windowHeight);

    ImVec2 windowPos = ImVec2(windowWidth - 300.0f - PADDING, PADDING);
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(ImVec2(300, 0));
    ImGui::SetNextWindowBgAlpha(0.8f);

    if (ImGui::Begin("##Notifications", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))
    {
        for (auto& notification : notifications)
        {
            if (notification.isError)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
            }

            ImGui::TextWrapped("%s", notification.message.c_str());
            ImGui::PopStyleColor();
        }
    }
    ImGui::End();
}

void UIManager::renderFileMenu()
{
    // New World
    if (ImGui::MenuItem("New World", "Ctrl+N"))
    {
        showConfirmationDialog("Create a new world? All unsaved changes will be lost.", [this]()
            {
                app->clearGrid(true);
                addNotification("New world created");
            });
    }

    ImGui::Separator();

    // Save World
    if (ImGui::MenuItem("Save World", "Ctrl+S"))
    {
        HWND hwnd = glfwGetWin32Window(app->getWindow());
        if (GridSerializer::saveGridToFile(app->getGrid(), hwnd))
        {
            addNotification("World saved successfully");
        }
        else if (GetLastError() != 0)
        {
            addNotification("Failed to save world", true);
        }
    }

    // Save World As...
    if (ImGui::MenuItem("Save World As...", "Ctrl+Shift+S"))
    {
        setCurrentDialog("SaveFile");
    }

    // Load World
    if (ImGui::MenuItem("Load World", "Ctrl+O"))
    {
        HWND hwnd = glfwGetWin32Window(app->getWindow());
        if (GridSerializer::loadGridFromFile(app->getGrid(), hwnd))
        {
            addNotification("World loaded successfully");
            app->updateRenderer();
        }
        else if (GetLastError() != 0)
        {
            addNotification("Failed to load world", true);
        }
    }

    ImGui::Separator();

    // Auto-Save options submenu
    if (ImGui::BeginMenu("Auto-Save Settings"))
    {
        ImGui::Checkbox("Enable Auto-Save", &enableAutoSave);

        if (ImGui::InputInt("Interval (minutes)", &autoSaveInterval, 1, 5))
        {
            if (autoSaveInterval < 1) autoSaveInterval = 1;
            if (autoSaveInterval > 60) autoSaveInterval = 60;
        }

        if (ImGui::Button("Set Auto-Save Folder..."))
        {
            std::string folder = FileDialog::selectFolder("Select Auto-Save Folder", glfwGetWin32Window(app->getWindow()));
            if (!folder.empty())
            {
                autoSaveFolder = folder;
                app->setAutoSaveSettings(enableAutoSave, autoSaveInterval, autoSaveFolder);
            }
        }

        ImGui::SameLine();
        ImGui::Text("%s", autoSaveFolder.empty() ? "No folder selected" : autoSaveFolder.c_str());

        ImGui::EndMenu();
    }

    ImGui::Separator();

    // Export and import options
    if (ImGui::BeginMenu("Export/Import"))
    {
        if (ImGui::MenuItem("Export to OBJ..."))
        {
            // Add OBJ export implementation here
            addNotification("OBJ export not implemented yet");
        }

        if (ImGui::MenuItem("Import from OBJ..."))
        {
            // Add OBJ import implementation here
            addNotification("OBJ import not implemented yet");
        }

        ImGui::EndMenu();
    }

    ImGui::Separator();

    // Exit
    if (ImGui::MenuItem("Exit", "Alt+F4"))
    {
        showConfirmationDialog("Exit application? All unsaved changes will be lost.", [this]()
            {
                glfwSetWindowShouldClose(app->getWindow(), GLFW_TRUE);
            });
    }
}

void UIManager::renderEditMenu()
{
    // Toggle edit mode
    if (ImGui::MenuItem("Edit Mode", nullptr, &isEditing))
    {
        app->setEditingMode(isEditing);
    }

    ImGui::Separator();

    // Brush size
    if (ImGui::BeginMenu("Brush Size"))
    {
        for (int i = 1; i <= 5; i++)
        {
            char label[32];
            sprintf_s(label, "%dx%dx$d", i, i, i);
            if (ImGui::MenuItem(label, nullptr, brushSize == i))
            {
                brushSize = i;
                app->setBrushSize(brushSize);
            }
        }
        ImGui::EndMenu();
    }

    // Cube color
    if (ImGui::MenuItem("Set Cube Color..."))
    {
        ImGui::OpenPopup("ColorPicker");
    }

    // Color picker popup
    if (ImGui::BeginPopup("ColorPicker"))
    {
        ImGui::Text("Cube Color");
        if (ImGui::ColorEdit3("##color", &selectedCubeColor[0]))
        {
            app->setSelectedCubeColor(selectedCubeColor);
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    // Undo/Redo (placeholder for future implementation)
    ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
    ImGui::MenuItem("Redo", "Ctrl+Y", false, false);

    ImGui::Separator();

    // Grid operations
    if (ImGui::MenuItem("Celar Grid"))
    {
        showConfirmationDialog("Clear all cubes except the floor?", [this]()
            {
                app->clearGrid(false);
                addNotification("Grid cleared");
            });
    }

    if (ImGui::MenuItem("Reset World"))
    {
        showConfirmationDialog("Reset the entire world including the floor?", [this]()
            {
                app->clearGrid(true);
                addNotification("World reset");
            });
    }
}

void UIManager::renderViewMenu()
{
    // Toggle panels
    if (ImGui::MenuItem("Control Panel", nullptr, &showControlPanel)) {}
    if (ImGui::MenuItem("Settings Panel", nullptr, &showSettingsPanel)) {}
    if (ImGui::MenuItem("Grid Navigation", nullptr, &showGridNavigationPanel)) {}
    if (ImGui::MenuItem("Profiler", nullptr, &showProfilerPanel)) {}

    ImGui::Separator();

    // Display options
    RenderSettings& renderSettings = app->getRenderSettings();

    if (ImGui::MenuItem("Frustum Culling", nullptr, &renderSettings.enableFrustumCulling))
    {
        app->updateRenderer();
    }

    if (ImGui::MenuItem("Shadows", nullptr, &renderSettings.enableShadows))
    {
        app->setupShadowMap();
    }

    // Visualization submenu
    if (ImGui::BeginMenu("Visualization"))
    {
        if (ImGui::MenuItem("Show Debug View", nullptr, &renderSettings.showDebugView)) {}
        if (ImGui::MenuItem("Show Frustum Wireframe", nullptr, &renderSettings.showFrustumWireframe)) {}
        if (ImGui::MenuItem("Show Chunk Boundaries", nullptr, &renderSettings.showChunkBoundaries))
        {
            app->getDebugRenderer()->setShowChunkBoundaries(renderSettings.showChunkBoundaries);
        }
        if (ImGui::MenuItem("Show Grid Lines", nullptr, &renderSettings.showGridLines))
        {
            app->getDebugRenderer()->setShowGridLines(renderSettings.showGridLines);
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();

    // Window size presets
    if (ImGui::BeginMenu("Window Size"))
    {
        if (ImGui::MenuItem("720p (1280x720)"))
        {
            resizeWindowPreset(1280, 720);
        }
        if (ImGui::MenuItem("1080p (1920x1080)"))
        {
            resizeWindowPreset(1920, 1080);
        }
        if (ImGui::MenuItem("1440p (2560x1440)"))
        {
            resizeWindowPreset(2560, 1440);
        }
        if (ImGui::MenuItem("4K (3840x2160)"))
        {
            resizeWindowPreset(3840, 2160);
        }
        ImGui::EndMenu();
    }

    // Theme selection
    if (ImGui::BeginMenu("Theme"))
    {
        if (ImGui::MenuItem("Dark Theme", nullptr, useDarkTheme))
        {
            useDarkTheme = true;
            setupTheme(true);
        }
        if (ImGui::MenuItem("Light Theme", nullptr, !useDarkTheme))
        {
            useDarkTheme = false;
            setupTheme(false);
        }
        ImGui::EndMenu();
    }
}

void UIManager::renderToolsMenu()
{
    // Quality presets
    if (ImGui::BeginMenu("Quality Preset"))
    {
        RenderSettings& renderSettings = app->getRenderSettings();

        if (ImGui::MenuItem("Low"))
        {
            renderSettings.applyPreset(RenderSettings::LOW);
            app->setupShadowMap();
        }
        if (ImGui::MenuItem("Medium"))
        {
            renderSettings.applyPreset(RenderSettings::MEDIUM);
            app->setupShadowMap();
        }
        if (ImGui::MenuItem("High"))
        {
            renderSettings.applyPreset(RenderSettings::HIGH);
            app->setupShadowMap();
        }
        if (ImGui::MenuItem("Ultra"))
        {
            renderSettings.applyPreset(RenderSettings::ULTRA);
            app->setupShadowMap();
        }

        ImGui::EndMenu();
    }

    // Performance options
    if (ImGui::BeginMenu("Performance"))
    {
        RenderSettings& renderSettings = app->getRenderSettings();

        if (ImGui::MenuItem("VSync", nullptr, &renderSettings.enableVSync))
        {
            glfwSwapInterval(renderSettings.enableVSync ? 1 : 0);
        }

        if (ImGui::MenuItem("Use Instancing", nullptr, &renderSettings.useInstancing))
        {
            app->updateRenderer();
        }

        ImGui::EndMenu();
    }

    ImGui::Separator();

    // Navigation tools
    if (ImGui::MenuItem("Go to Origin"))
    {
        app->getCamera()->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    }

    if (ImGui::MenuItem("Find Selected Cube"))
    {
        if (selectedCubeX != -1)
        {
            glm::vec3 worldPos = app->getGrid()->calculatePosition(selectedCubeX, selectedCubeY, selectedCubeZ);
            app->getCamera()->setTarget(worldPos);
        }
        else
        {
            addNotification("No cube selected", true);
        }
    }

    ImGui::Separator();

    // Debug tools
    if (ImGui::MenuItem("Refresh Instance Cache"))
    {
        app->getRenderer()->markCacheForUpdate();
        app->getRenderer()->updateChunkInstanceCache();
        addNotification("Instance cache refreshed");
    }
}

void UIManager::renderHelpMenu()
{
    if (ImGui::MenuItem("Controls"))
    {
        ImGui::OpenPopup("Controls Help");
    }

    if (ImGui::MenuItem("About"))
    {
        ImGui::OpenPopup("About");
    }

    // Controls popup
    if (ImGui::BeginPopupModal("Controls Help", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Navigation Controls:");
        ImGui::BulletText("WASD - Pan camera");
        ImGui::BulletText("Q/E - Rotate camera");
        ImGui::BulletText("Mouse Wheel - Zoom in/out");

        ImGui::Separator();

        ImGui::Text("Editor Controls:");
        ImGui::BulletText("Left Click - Place Cube");
        ImGui::BulletText("Right Click - Remove cube");
        ImGui::BulletText("Ctrl+Z - Undo (not implemented yet)");
        ImGui::BulletText("Ctrl+Y - Redo (not implemented yet)");

        ImGui::Separator();

        ImGui::Text("UI Controls:");
        ImGui::BulletText("F1 - Toggle UI visibility");
        ImGui::BulletText("Tab - Toggle debug view");
        ImGui::BulletText("Ctrl+ / Ctrl- - Resize window");

        if (ImGui::Button("Close", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // About popup
    if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Isometric Voxel Editor");
        ImGui::Text("Version 1.0");
        ImGui::Separator();
        ImGui::Text("An OpenGL-based voxel editor with chunk-based rendering");
        ImGui::Text("and frustum fulling for optimal performance.");

        ImGui::Separator();

        ImGui::Text("Technologies used:");
        ImGui::BulletText("OpenGl 3.3");
        ImGui::BulletText("GLFW");
        ImGui::BulletText("GLM");
        ImGui::BulletText("Dear ImGui");

        if (ImGui::Button("Close", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void UIManager::renderControlPanel()
{
    if (ImGui::Begin("Control Panel", &showControlPanel))
    {
        if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float zoom = app->getCamera()->getZoom();
            if (ImGui::SliderFloat("Zoom", &zoom, 0.1f, 5.0f))
            {
                app->getCamera()->setZoom();
            }

            // Camera position display
            glm::vec3 camPos = app->getCamera()->getPosition();
            ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", camPos.x, camPos.y, camPos.z);

            // Camera target display
            glm::vec3 camTarget = app->getCamera()->getTarget();
            ImGui::Text("Camera Target: (%.1f, %.1f, %.1f)", camTarget.x, camTarget.y, camTarget.z);
        }

        if (ImGui::CollapsingHeader("Editing Tools", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Checkbox("Editing Mode", &isEditing))
            {
                app->setEditingMode(isEditing);
            }

            ImGui::ColorEdit3("Cube Color", &selectedCubeColor[0]);
            if (ImGui::SliderInt("Brush Size", &brushSize, 1, 5))
            {
                app->setBrushSize(brushSize);
            }

            if (selectedCubeX != -1)
            {
                ImGui::Text("Selected Cube: (%d, %d, %d)", selectedCubeX, selectedCubeY, selectedCubeZ);

                if (ImGui::Button("Delete Cube"))
                {
                    app->setCubeAt(selectedCubeX, selectedCubeY, selectedCubeZ, false, glm::vec3(0.0f));
                }

                ImGui::SameLine();

                if (ImGui::Button("Go To Cube"))
                {
                    glm::vec3 cubePos = app->getGrid()->calculatePosition(selectedCubeX, selectedCubeY, selectedCubeZ);
                    app->getCamera()->setTarget(cubePos);
                }
            }
            else
            {
                ImGui::Text("No cube selected");
            }
        }

        if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

            // Cube statistics
            ImGui::Text("Active Chunks: %d", app->getGrid()->getActiveChunkCount());
            ImGui::Text("Total Cubes: %d", app->getGrid()->getTotalActiveCubeCount());
            ImGui::Text("Visible Cubes: %d", app->getVisibleCubeCount());

            // Culling stats
            RenderSettings& renderSettings = app->getRenderSettings();
            if (renderSettings.enableFrustumCulling)
            {
                float cullingPercentage = 100.0f * (1.0f - (float)app->getVisibleCubeCount() /
                    (float)max(1, app->getGrid()->getTotalActiveCubeCount()));

                ImGui::Text("Culling Rate: %.1f%%", cullingPercentage);
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                    "Frustum culling disabled");
            }
        }
    }
    ImGui::End();
}

void UIManager::renderSettingsPanel()
{
    if (ImGui::Begin("Settings", &showSettingsPanel))
    {
        renderRenderSettingsSection();
    }
    ImGui::End();
}

void UIManager::renderRenderSettingsSection()
{
}

void UIManager::renderGridNavigationPanel()
{
    if (ImGui::Begin("Grid Navigation", &showGridNavigationPanel))
    {
        // Show current bounds
        glm::ivec3 minBounds = app->getGrid()->getMinBounds();
        glm::ivec3 maxBounds = app->getGrid()->getMaxBounds();

        ImGui::Text("Grid Bounds: (%d, %d, %d) to (%d, %d, %d)",
            minBounds.x, minBounds.y, minBounds.z,
            maxBounds.x, maxBounds.y, maxBounds.z);

        ImGui::Separator();

        // Navigation to specific coordinates
        static int targetX = 0, targetY = 0, targetZ = 0;
        ImGui::InputInt("Target X", &targetX);
        ImGui::InputInt("Target Y", &targetY);
        ImGui::InputInt("Target Z", &targetZ);

        if (ImGui::Button("Go To Target"))
        {
            // Calculate world coordinates
            glm::vec3 worldPos = app->getGrid()->calculatePosition(targetX, targetY, targetZ);
            
            // Set camera target to that location
            app->getCamera()->setTarget(worldPos);

            // Check if we're in edit mode
            if (isEditing)
            {
                // Select cube at that location (or nearby empty spot)
                if (app->getGrid()->isCubeActive(targetX, targetY, targetZ))
                {
                    selectedCubeX = targetX;
                    selectedCubeY = targetY;
                    selectedCubeZ = targetZ;
                    app->setSelectedCubeCoords(selectedCubeX, selectedCubeY, selectedCubeZ);
                }
            }
        }

        ImGui::Separator();

        // Chunk loading settings
        ImGui::SliderInt("View Distance (chunks)", &chunkViewDistance, 2, 16);

        if (ImGui::Button("Update Loaded Chunks"))
        {
            // Calculate camera position in grid coordinates
            glm::vec3 camWorldPos = app->getCamera()->getPosition();
            glm::ivec3 camGridPos = app->getGrid()->worldToGridCoordinates(camWorldPos);

            // Update which chunks are loaded
            app->getGrid()->updateLoadedChunks(camGridPos, chunkViewDistance);
        }

        ImGui::Separator();

        // Statistics
        ImGui::Text("Active Chunks: %d", app->getGrid()->getActiveChunkCount());
        ImGui::Text("Total Cubes: %d" app->getGrid()->getTotalActiveCubeCount());
    }
    ImGui::End();
}

void UIManager::renderSaveFileDialog()
{
    ImGui::OpenPopup("Save World##dialog");

    if (ImGui::BeginPopupModal("Save World##dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Save world to a file");
        ImGui::Separator();

        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            HWND hwnd = glfwGetWin32Window(app->getWindow());
            if (GridSerializer::saveGridToFile(app->getGrid(), hwnd))
            {
                addNotification("World saved successfully");
            }
            else if (GetLastError() != 0)
            {
                addNotification("Failed to save world", true);
            }

            setCurrentDialog("");
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            setCurrentDialog("");
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void UIManager::renderLoadFileDialog()
{
    ImGui::OpenPopup("Load World##dialog");

    if (ImGui::BeginPopupModal("Load World##dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Load world from a file");
        ImGui::Separator();

        if (ImGui::Button("Load", ImVec2(120, 0)))
        {
            HWND hwnd = glfwGetWin32Window(app->getWindow());
            if (GridSerializer::loadGridFromFile(app->getGrid(), hwnd))
            {
                addNotification("World loaded successfully");
                app->updateRenderer();
            }
            else if (GetLastError() != 0)
            {
                addNotification("Failed to load world", true);
            }

            setCurrentDialog("");
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            setCurrentDialog("");
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void UIManager::renderSettingsDialog()
{
    ImGui::OpenPopup("Settings##dialog");

    if (ImGui::BeginPopupModal("Settings##dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // Add custom settings dialog here
        // This is just a placeholder - you can expand it with additional settings

        // Theme selection
        if (ImGui::Checkbox("Use Dark Theme", &useDarkTheme))
        {
            setupTheme(useDarkTheme);
        }

        ImGui::Separator();

        // UI-specific settings
        static bool showMainMenu = true;
        if (ImGui::Checkbox("Show Main Menu Bar", &showMainMenu))
        {
            showMainMenuBar = showMainMenu;
        }

        static bool enableDocking = true;
        if (ImGui::Checkbox("Enable Docking", &enableDocking))
        {
            showDockSpace = enableDocking;
        }

        ImGui::Separator();

        // Close button
        if (ImGui::Button("Close", ImVec2(120, 0)))
        {
            setCurrentDialog("");
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void UIManager::renderConfirmationDialog()
{
    // This is handled in beginFrame()
}
