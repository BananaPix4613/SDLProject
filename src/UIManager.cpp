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
    showViewportPanel(true),
    showControlPanel(true),
    showSettingsPanel(true),
    showGridNavigationPanel(true),
    showProfilerPanel(true),
    shouldOpenControlsPopup(false),
    shouldOpenAboutPopup(false),
    isEditing(false),
    windowWidth(1280),
    windowHeight(720),
    dpiScale(1.0f),
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

        // Set up docking
        if (showDockSpace)
        {
            renderDockSpace();
        }

        // Main menu bar rendering
        if (showMainMenuBar)
        {
            renderMainMenuBar();
        }

        // Handle popup flags
        if (shouldOpenControlsPopup)
        {
            ImGui::OpenPopup("Controls Help");
            shouldOpenControlsPopup = false;
        }

        if (shouldOpenAboutPopup)
        {
            ImGui::OpenPopup("About");
            shouldOpenAboutPopup = false;
        }

        // Render all modal dialogs
        renderPopupModals();

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

        // Only set up the default layout if it hasn't been done yet
        static bool first_time = true;
        if (first_time && ImGui::DockBuilderGetNode(dockspace_id) == NULL)
        {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

            // Split the dockspace into sections
            ImGuiID dock_main = dockspace_id;
            ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.25f, NULL, &dock_main);
            ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.2f, NULL, &dock_main);
            ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.25f, NULL, &dock_main);

            // Dock windows to specific areas
            ImGui::DockBuilderDockWindow("Viewport", dock_main);
            ImGui::DockBuilderDockWindow("Control Panel", dock_right);
            ImGui::DockBuilderDockWindow("Settings", dock_right);
            ImGui::DockBuilderDockWindow("Grid Navigation", dock_left);
            ImGui::DockBuilderDockWindow("Profiler", dock_bottom);

            ImGui::DockBuilderFinish(dockspace_id);
            first_time = false;
        }

        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    }

    // End the dockspace window
    ImGui::End();
}

void UIManager::renderPopupModals()
{
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

        ImGui::Text("Window Controls:");
        ImGui::BulletText("F11 - Toggle Fullscreen");
        ImGui::BulletText("Ctrl+ / Ctrl- - Resize window");

        ImGui::Separator();

        ImGui::Text("UI Controls:");
        ImGui::BulletText("F1 - Toggle UI visibility");
        ImGui::BulletText("Tab - Toggle debug view");

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

void UIManager::renderPanels()
{
    // Render main panels if they are visible
    if (showViewportPanel)
    {
        renderViewportPanel();
    }

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
            sprintf_s(label, "%dx%dx%d", i, i, i);
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
    if (ImGui::MenuItem("Clear Grid"))
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
            app->getRenderSettings().showChunkBoundaries = renderSettings.showChunkBoundaries;
            app->updateRenderSettings();
        }
        if (ImGui::MenuItem("Show Grid Lines", nullptr, &renderSettings.showGridLines))
        {
            app->getRenderSettings().showGridLines = renderSettings.showGridLines;
            app->updateRenderSettings();
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();

    // Toggle fullscreen option
    if (ImGui::MenuItem("Toggle Fullscreen", "F11", app->getIsFullscreen()))
    {
        app->toggleFullscreen();
    }

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
}

void UIManager::renderHelpMenu()
{
    if (ImGui::MenuItem("Controls"))
    {
        shouldOpenControlsPopup = true;
    }

    if (ImGui::MenuItem("About"))
    {
        shouldOpenAboutPopup = true;
    }
}

void UIManager::renderViewportPanel()
{
    if (ImGui::Begin("Viewport", &showViewportPanel, ImGuiWindowFlags_NoScrollbar))
    {
        // Get current content region available for the image
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        // Update application viewport size if window size changed
        if (viewportSize.x > 0 && viewportSize.y > 0)
        {
            app->setViewportSize(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
        }

        // Display the rendered scene texture (sceneTexture should be populated by RenderIntegration)
        if (sceneTexture > 0)
        {
            ImGui::Image((ImTextureID)(intptr_t)sceneTexture,
                         viewportSize,
                         ImVec2(0, 1), ImVec2(1, 0)); // Flip UV coordinates to match OpenGL texture coords

            // Store viewport position in screen space for input handling
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 contentPos = ImGui::GetCursorStartPos();
            app->setViewportPos(
                static_cast<int>(windowPos.x + contentPos.x),
                static_cast<int>(windowPos.y + contentPos.y)
            );
        }
        else
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Scene texture not available!");
        }
    }
    ImGui::End();
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
                app->getCamera()->setZoom(zoom);
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
    RenderSettings& renderSettings = app->getRenderSettings();

    if (ImGui::CollapsingHeader("Graphics Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Quality presets
        const char* presets[] = { "Low", "Medium", "High", "Ultra" };
        static int currentPreset = 2; // High by default

        if (ImGui::Combo("Quality Preset", &currentPreset, presets, 4))
        {
            renderSettings.applyPreset(static_cast<RenderSettings::QualityPreset>(currentPreset));
            // Recreate shadow map if resolution changed
            app->setupShadowMap();
        }

        // Individual settings
        if (ImGui::TreeNode("Shadow Settings"))
        {
            bool shadowsChanged = false;

            shadowsChanged |= ImGui::Checkbox("Enable Shadows", &renderSettings.enableShadows);

            const char* resolutions[] = { "512x512", "1024x1024", "2048x2048", "4096x4096" };
            int currentRes = 0;

            if (renderSettings.shadowMapResolution == 512) currentRes = 0;
            else if (renderSettings.shadowMapResolution == 1024) currentRes = 1;
            else if (renderSettings.shadowMapResolution == 2048) currentRes = 2;
            else if (renderSettings.shadowMapResolution == 4096) currentRes = 3;

            if (ImGui::Combo("Shadow Resolution", &currentRes, resolutions, 4))
            {
                switch (currentRes)
                {
                case 0: renderSettings.shadowMapResolution = 512; break;
                case 1: renderSettings.shadowMapResolution = 1024; break;
                case 2: renderSettings.shadowMapResolution = 2048; break;
                case 3: renderSettings.shadowMapResolution = 4096; break;
                }
                shadowsChanged = true;
            }

            shadowsChanged |= ImGui::Checkbox("Use PCF Filtering", &renderSettings.usePCF);

            // Make the bias slider more precise for debugging
            ImGui::SliderFloat("Shadow Bias", &renderSettings.shadowBias, 0.0001f, 0.01f, "%.5f");

            if (shadowsChanged)
            {
                // Recreate shadow map if settings changed
                app->setupShadowMap();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Lighting Settings"))
        {
            ImGui::SliderFloat("Ambient Strength", &renderSettings.ambientStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("Specular Strength", &renderSettings.specularStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("Shininess", &renderSettings.shininess, 1.0f, 128.0f);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Performance Settings"))
        {
            ImGui::Checkbox("Enable Frustum Culling", &renderSettings.enableFrustumCulling);
            ImGui::Checkbox("Use Instanced Rendering", &renderSettings.useInstancing);

            bool vsyncChanged = ImGui::Checkbox("VSync", &renderSettings.enableVSync);
            if (vsyncChanged)
            {
                glfwSwapInterval(renderSettings.enableVSync ? 1 : 0);
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Advanced Visualization"))
        {
            bool settingsChanged = false;

            // Chunk visualization
            if (ImGui::TreeNode("Chunk System"))
            {
                ImGui::Text("Active Chunks: %d", app->getGrid()->getActiveChunkCount());
                ImGui::Text("Total Cubes: %d", app->getGrid()->getTotalActiveCubeCount());

                if (ImGui::Checkbox("Show Chunk Boundaries", &renderSettings.showChunkBoundaries))
                {
                    settingsChanged = true;
                }

                if (ImGui::SliderInt("Chunk View Distance", &chunkViewDistance, 1, 16))
                {
                    settingsChanged = true;
                    app->getGrid()->updateLoadedChunks(
                        app->getGrid()->worldToGridCoordinates(app->getCamera()->getPosition()),
                        chunkViewDistance);
                }

                if (ImGui::SliderFloat("Max View Distance", &maxViewDistance, 100.0f, 2000.0f))
                {
                    settingsChanged = true;
                }

                if (ImGui::Button("Update Loaded Chunks"))
                {
                    app->getGrid()->updateLoadedChunks(
                        app->getGrid()->worldToGridCoordinates(app->getCamera()->getPosition()),
                        chunkViewDistance);
                }

                ImGui::TreePop();
            }

            // Rendering optimization
            if (ImGui::TreeNode("Rendering Optimization"))
            {
                if (ImGui::Checkbox("Use Instance Cache", &useInstanceCache))
                {
                    settingsChanged = true;
                }

                if (ImGui::Checkbox("Per-Cube Culling", &perCubeCulling))
                {
                    settingsChanged = true;
                }

                if (ImGui::SliderInt("Batch Size", &batchSize, 1000, 50000))
                {
                    settingsChanged = true;
                }

                ImGui::TreePop();
            }

            // Apply settings changes
            if (settingsChanged)
            {
                // Get values from UI widgets
                float viewDist = maxViewDistance;
                bool useCache = useInstanceCache;
                bool cubeCulling = perCubeCulling;
                int batch = batchSize;

                // Update renderer settings
                app->updateRenderSettings();
            }

            ImGui::TreePop();
        }

        // Debug visualization
        if (ImGui::TreeNode("Debug Settings"))
        {
            ImGui::Checkbox("Show Debug View", &renderSettings.showDebugView);
            ImGui::Checkbox("Show Frustum Wireframe", &renderSettings.showFrustumWireframe);
            ImGui::Checkbox("Show Performance Overlay", &renderSettings.showPerformanceOverlay);

            const char* debugViews[] = { "Shadow Map Corner", "Full Shadow Map", "Linearized Depth" };
            ImGui::Combo("Debug View Mode", &renderSettings.currentDebugView, debugViews, 3);

            ImGui::TreePop();
        }
    }
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
        ImGui::Text("Total Cubes: %d", app->getGrid()->getTotalActiveCubeCount());
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

void UIManager::addNotification(const std::string& message, bool isError)
{
    notifications.emplace_back(message, isError);
}

void UIManager::updateNotifications(float deltaTime)
{
    // Update notification timers and remove expired ones
    for (auto it = notifications.begin(); it != notifications.end();)
    {
        it->timeRemaining -= deltaTime;
        if (it->timeRemaining <= 0.0f)
        {
            it = notifications.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void UIManager::setupTheme(bool darkTheme)
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Apply DPI scaling
    style.ScaleAllSizes(dpiScale);
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = dpiScale;

    if (darkTheme)
    {
        ImGui::StyleColorsDark();

        // Customize dark theme
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.13f, 1.0f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.16f, 0.16f, 0.21f, 1.0f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.27f, 1.0f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.37f, 1.0f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.35f, 0.42f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.32f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.3f, 0.37f, 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.35f, 0.42f, 1.0f);
    }
    else
    {
        ImGui::StyleColorsLight();

        // Customize light theme
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.74f, 0.74f, 0.87f, 0.76f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.84f, 0.84f, 0.87f, 0.76f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.88f, 0.88f, 0.92f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.70f, 0.70f, 0.78f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.80f, 0.80f, 0.85f, 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.73f, 0.73f, 0.82f, 1.0f);
    }

    // General style settings regardless of theme
    style.WindowRounding = 6.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(5, 3);
    style.ItemSpacing = ImVec2(6, 4);
    style.ItemInnerSpacing = ImVec2(4, 4);

    style.ScrollbarSize = 16.0f;
    style.GrabMinSize = 8.0f;
}

void UIManager::showConfirmationDialog(const std::string& message, std::function<void()> onConfirm)
{
    confirmMessage = message;
    confirmCallback = onConfirm;
    showConfirmDialog = true;
}

void UIManager::resizeWindowPreset(int width, int height)
{
    app->resizeWindow(width, height);

    // Update local window size variables
    windowWidth = width;
    windowHeight = height;
}
