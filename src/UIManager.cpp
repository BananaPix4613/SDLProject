#include "UIManager.h"
#include "Application.h"
#include "RenderSystem.h"
#include "Texture.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_internal.h"

#include <GL/gl3w.h>
#include <iostream>
#include <algorithm>

// Initialize static instance to null
UIManager* UIManager::s_instance = nullptr;

UIManager& UIManager::getInstance()
{
    if (s_instance == nullptr)
    {
        s_instance = new UIManager();
    }
    return *s_instance;
}

UIManager::UIManager() :
    m_window(nullptr),
    m_application(nullptr),
    m_dpiScale(1.0f),
    m_notificationDuration(5.0f),
    m_showConfirmDialog(false),
    m_showInputDialog(false)
{
}

UIManager::~UIManager()
{
    shutdown();
}

bool UIManager::initialize(GLFWwindow* window)
{
    if (window == nullptr)
    {
        std::cerr << "UIManager::initialize - Window is null" << std::endl;
        return false;
    }

    m_window = window;

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Enable docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Enable multi-viewport / platform windows
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Setup initial style
    setTheme("Dark");

    // Add default font
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    m_dpiScale = xscale;

    // Configure default font size with DPI awareness
    float fontSize = 13.0f * m_dpiScale;
    io.Fonts->AddFontDefault();
    io.FontGlobalScale = 1.0f;

    return true;
}

void UIManager::shutdown()
{
    if (ImGui::GetCurrentContext() != nullptr)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}

void UIManager::beginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::render()
{
    // Render notifications
    renderNotifications();

    // Render modal dialogs
    if (m_showConfirmDialog)
    {
        renderConfirmDialog();
    }

    if (m_showInputDialog)
    {
        renderInputDialog();
    }

    // Render main ImGui content
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

void UIManager::setApplication(Application* application)
{
    m_application = application;
}

void UIManager::setTexture(const std::string& name, unsigned int textureId, int width, int height)
{
    TextureInfo info;
    info.textureId = reinterpret_cast<void*>(static_cast<intptr_t>(textureId));
    info.width = width;
    info.height = height;
    m_textures[name] = info;
}

void UIManager::setRenderTarget(const std::string& name, RenderTarget* target)
{
    if (target)
    {
        setTexture(name, target->getColorTexture(), target->getWidth(), target->getHeight());
    }
}

void UIManager::setTexture(const std::string& name, Texture* texture)
{
    if (texture)
    {
        setTexture(name, texture->getID(), texture->getWidth(), texture->getHeight());
    }
}

void UIManager::setDpiScale(float scale)
{
    m_dpiScale = scale;
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = scale;
}

//==== Window and Panel Management ====

bool UIManager::beginDockspace(const std::string& id)
{
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
    {
        window_flags |= ImGuiWindowFlags_NoBackground;
    }

    // Important: note that we proceed even if Begin() return false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    bool open = true;
    bool result = ImGui::Begin(id.c_str(), &open, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID(id.c_str());
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    return result;
}

void UIManager::endDockspace()
{
    ImGui::End();
}

bool UIManager::beginWindow(const std::string& name, bool* open, int flags)
{
    return ImGui::Begin(name.c_str(), open, flags);
}

void UIManager::endWindow()
{
    ImGui::End();
}

bool UIManager::beginPanel(const std::string& name, int flags)
{
    return ImGui::CollapsingHeader(name.c_str(), flags);
}

void UIManager::endPanel()
{
    // ImGui's CollapsingHeader doesn't need an explicit end call
}

bool UIManager::beginPopup(const std::string& name)
{
    return ImGui::BeginPopupModal(name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
}

void UIManager::openPopup(const std::string& name)
{
    ImGui::OpenPopup(name.c_str());
}

void UIManager::endPopup()
{
    ImGui::EndPopup();
}

bool UIManager::beginMenuBar()
{
    return ImGui::BeginMenuBar();
}

void UIManager::endMenuBar()
{
    ImGui::EndMenuBar();
}

bool UIManager::beginMenu(const std::string& label, bool enabled)
{
    return ImGui::BeginMenu(label.c_str(), enabled);
}

void UIManager::endMenu()
{
    ImGui::EndMenu();
}

bool UIManager::menuItem(const std::string& label, const std::string& shortcut, bool selected, bool enabled)
{
    return ImGui::MenuItem(label.c_str(), shortcut.c_str(), selected, enabled);
}

bool UIManager::beginTable(const std::string& id, int columnCount, int flags, const glm::vec2& outerSize, float innerWidth)
{
    return ImGui::BeginTable(id.c_str(), columnCount, flags, ImVec2(outerSize.x, outerSize.y), innerWidth);
}

void UIManager::endTable()
{
    ImGui::EndTable();
}

void UIManager::tableSetupColumn(const std::string& label, int flags, float width)
{
    ImGui::TableSetupColumn(label.c_str(), flags, width);
}

void UIManager::tableHeadersRow()
{
    ImGui::TableHeadersRow();
}

void UIManager::tableNextRow()
{
    ImGui::TableNextRow();
}

bool UIManager::tableNextColumn()
{
    return ImGui::TableNextColumn();
}

bool UIManager::beginChild(const std::string& id, const glm::vec2& size, bool border, int flags)
{
    return ImGui::BeginChild(id.c_str(), ImVec2(size.x, size.y), border, flags);
}

void UIManager::endChild()
{
    ImGui::EndChild();
}

void UIManager::beginGroup()
{
    ImGui::BeginGroup();
}

void UIManager::endGroup()
{
    ImGui::EndGroup();
}

void UIManager::beginTooltip()
{
    ImGui::BeginTooltip();
}

void UIManager::endTooltip()
{
    ImGui::EndTooltip();
}

bool UIManager::beginCombo(const std::string& label, const std::string& previewValue, int flags)
{
    return ImGui::BeginCombo(label.c_str(), previewValue.c_str(), flags);
}

void UIManager::endCombo()
{
    ImGui::EndCombo();
}

bool UIManager::beginListBox(const std::string& label, const glm::vec2& size)
{
    return ImGui::BeginListBox(label.c_str(), ImVec2(size.x, size.y));
}

void UIManager::endListBox()
{
    ImGui::EndListBox();
}

bool UIManager::treeNode(const std::string& label, int flags)
{
    return ImGui::TreeNodeEx(label.c_str(), flags);
}

void UIManager::treePop()
{
    ImGui::TreePop();
}

//==== Layout Controls ====

void UIManager::spacing(float height)
{
    if (height <= 0.0f)
    {
        ImGui::Spacing();
    }
    else
    {
        ImGui::Dummy(ImVec2(0.0f, height));
    }
}

void UIManager::newLine()
{
    ImGui::NewLine();
}

void UIManager::separator()
{
    ImGui::Separator();
}

void UIManager::setNextItemWidth(float width)
{
    ImGui::SetNextItemWidth(width);
}

void UIManager::indent(float indent)
{
    ImGui::Indent(indent);
}

void UIManager::unindent(float indent)
{
    ImGui::Unindent(indent);
}

void UIManager::pushId(const std::string& id)
{
    ImGui::PushID(id.c_str());
}

void UIManager::pushId(int id)
{
    ImGui::PushID(id);
}

void UIManager::popId()
{
    ImGui::PopID();
}

void UIManager::sameLine(float spacing)
{
    ImGui::SameLine(0.0f, spacing);
}

void UIManager::pushStyleColor(int colorId, const glm::vec4& color)
{
    ImGui::PushStyleColor(colorId, ImVec4(color.r, color.g, color.b, color.a));
}

void UIManager::popStyleColor(int count)
{
    ImGui::PopStyleColor(count);
}

void UIManager::pushStyleVar(int styleId, float value)
{
    ImGui::PushStyleVar(styleId, value);
}

void UIManager::pushStyleVar(int styleId, const glm::vec2& value)
{
    ImGui::PushStyleVar(styleId, ImVec2(value.x, value.y));
}

void UIManager::popStyleVar(int count)
{
    ImGui::PopStyleVar(count);
}

//==== Basic Controls ====

void UIManager::text(const std::string& text)
{
    ImGui::Text("%s", text.c_str());
}

void UIManager::textColored(const std::string& text, const glm::vec4& color)
{
    ImGui::TextColored(ImVec4(color.r, color.g, color.b, color.a), "%s", text.c_str());
}

void UIManager::textWrapped(const std::string& text)
{
    ImGui::TextWrapped("%s", text.c_str());
}

void UIManager::textDisabled(const std::string& text)
{
    ImGui::TextDisabled("%s", text.c_str());
}

void UIManager::bulletText(const std::string& text)
{
    ImGui::BulletText("%s", text.c_str());
}

void UIManager::labelText(const std::string& label, const std::string& text)
{
    ImGui::LabelText(label.c_str(), "%s", text.c_str());
}

bool UIManager::button(const std::string& label, const glm::vec2& size)
{
    return ImGui::Button(label.c_str(), ImVec2(size.x, size.y));
}

bool UIManager::smallButton(const std::string& label)
{
    return ImGui::SmallButton(label.c_str());
}

bool UIManager::invisibleButton(const std::string& id, const glm::vec2& size)
{
    return ImGui::InvisibleButton(id.c_str(), ImVec2(size.x, size.y));
}

bool UIManager::imageButton(const std::string& textureName, const glm::vec2& size,
                            const glm::vec2& uvMin, const glm::vec2& uvMax,
                            const glm::vec4& bgColor,
                            const glm::vec4& tintColor)
{
    auto it = m_textures.find(textureName);
    if (it != m_textures.end())
    {
        // Create a unique ID based on texture name
        std::string buttonId = "##ImgBtn_" + textureName;

        // Properly cast the textureId to ImTextureID
        ImTextureID texId = reinterpret_cast<ImTextureID>(it->second.textureId);

        return ImGui::ImageButton(
            buttonId.c_str(),           // Unique ID string (required)
            texId,                      // Texture ID (with proper cast)
            ImVec2(size.x, size.y),     // Size
            ImVec2(uvMin.x, uvMin.y),   // UV0
            ImVec2(uvMax.x, uvMax.y),   // UV1
            ImVec4(bgColor.r, bgColor.g, bgColor.b, bgColor.a),      // Background color
            ImVec4(tintColor.r, tintColor.g, tintColor.b, tintColor.a) // Tint color
        );
    }
    return false;
}

bool UIManager::checkbox(const std::string& label, bool* checked)
{
    return ImGui::Checkbox(label.c_str(), checked);
}

bool UIManager::radioButton(const std::string& label, bool active)
{
    return ImGui::RadioButton(label.c_str(), active);
}

bool UIManager::radioButton(const std::string& label, int* v, int vButton)
{
    return ImGui::RadioButton(label.c_str(), v, vButton);
}

void UIManager::progressBar(float fraction, const glm::vec2& size, const std::string& overlay)
{
    ImGui::ProgressBar(fraction, ImVec2(size.x, size.y), overlay.empty() ? nullptr : overlay.c_str());
}

void UIManager::bullet()
{
    ImGui::Bullet();
}

//==== Drag Controls ====

bool UIManager::dragFloat(const std::string& label, float* value, float speed, float min, float max, const std::string& format, int flags)
{
    return ImGui::DragFloat(label.c_str(), value, speed, min, max, format.c_str(), flags);
}

bool UIManager::dragFloat2(const std::string& label, glm::vec2* value, float speed, float min, float max, const std::string& format, int flags)
{
    return ImGui::DragFloat2(label.c_str(), &value->x, speed, min, max, format.c_str(), flags);
}

bool UIManager::dragFloat3(const std::string& label, glm::vec3* value, float speed, float min, float max, const std::string& format, int flags)
{
    return ImGui::DragFloat3(label.c_str(), &value->x, speed, min, max, format.c_str(), flags);
}

bool UIManager::dragFloat4(const std::string& label, glm::vec4* value, float speed, float min, float max, const std::string& format, int flags)
{
    return ImGui::DragFloat4(label.c_str(), &value->x, speed, min, max, format.c_str(), flags);
}

bool UIManager::dragInt(const std::string& label, int* value, float speed, int min, int max, const std::string& format, int flags)
{
    return ImGui::DragInt(label.c_str(), value, speed, min, max, format.c_str(), flags);
}

bool UIManager::dragInt2(const std::string& label, glm::ivec2* value, float speed, int min, int max, const std::string& format, int flags)
{
    return ImGui::DragInt2(label.c_str(), &value->x, speed, min, max, format.c_str(), flags);
}

bool UIManager::dragInt3(const std::string& label, glm::ivec3* value, float speed, int min, int max, const std::string& format, int flags)
{
    return ImGui::DragInt3(label.c_str(), &value->x, speed, min, max, format.c_str(), flags);
}

bool UIManager::dragInt4(const std::string& label, glm::ivec4* value, float speed, int min, int max, const std::string& format, int flags)
{
    return ImGui::DragInt4(label.c_str(), &value->x, speed, min, max, format.c_str(), flags);
}

//==== Slider Controls ====

bool UIManager::sliderFloat(const std::string& label, float* value, float min, float max, const std::string& format, int flags)
{
    return ImGui::SliderFloat(label.c_str(), value, min, max, format.c_str(), flags);
}

bool UIManager::sliderFloat2(const std::string& label, glm::vec2* value, float min, float max, const std::string& format, int flags)
{
    return ImGui::SliderFloat2(label.c_str(), &value->x, min, max, format.c_str(), flags);
}

bool UIManager::sliderFloat3(const std::string& label, glm::vec3* value, float min, float max, const std::string& format, int flags)
{
    return ImGui::SliderFloat3(label.c_str(), &value->x, min, max, format.c_str(), flags);
}

bool UIManager::sliderFloat4(const std::string& label, glm::vec4* value, float min, float max, const std::string& format, int flags)
{
    return ImGui::SliderFloat4(label.c_str(), &value->x, min, max, format.c_str(), flags);
}

bool UIManager::sliderInt(const std::string& label, int* value, int min, int max, const std::string& format, int flags)
{
    return ImGui::SliderInt(label.c_str(), value, min, max, format.c_str(), flags);
}

bool UIManager::sliderInt2(const std::string& label, glm::ivec2* value, int min, int max, const std::string& format, int flags)
{
    return ImGui::SliderInt2(label.c_str(), &value->x, min, max, format.c_str(), flags);
}

bool UIManager::sliderInt3(const std::string& label, glm::ivec3* value, int min, int max, const std::string& format, int flags)
{
    return ImGui::SliderInt3(label.c_str(), &value->x, min, max, format.c_str(), flags);
}

bool UIManager::sliderInt4(const std::string& label, glm::ivec4* value, int min, int max, const std::string& format, int flags)
{
    return ImGui::SliderInt4(label.c_str(), &value->x, min, max, format.c_str(), flags);
}

bool UIManager::sliderAngle(const std::string& label, float* radians, float degreesMin, float degreesMax, const std::string& format, int flags)
{
    return ImGui::SliderAngle(label.c_str(), radians, degreesMin, degreesMax, format.c_str(), flags);
}

//==== Input Controls ====

bool UIManager::inputText(const std::string& label, char* buffer, size_t bufferSize, int flags)
{
    return ImGui::InputText(label.c_str(), buffer, bufferSize, flags);
}

bool UIManager::inputTextMultiline(const std::string& label, char* buffer, size_t bufferSize, const glm::vec2& size, int flags)
{
    return ImGui::InputTextMultiline(label.c_str(), buffer, bufferSize, ImVec2(size.x, size.y), flags);
}

bool UIManager::inputFloat(const std::string& label, float* value, float step, float stepFast, const std::string& format, int flags)
{
    return ImGui::InputFloat(label.c_str(), value, step, stepFast, format.c_str(), flags);
}

bool UIManager::inputFloat2(const std::string& label, glm::vec2* value, const std::string& format, int flags)
{
    return ImGui::InputFloat2(label.c_str(), &value->x, format.c_str(), flags);
}

bool UIManager::inputFloat3(const std::string& label, glm::vec3* value, const std::string& format, int flags)
{
    return ImGui::InputFloat3(label.c_str(), &value->x, format.c_str(), flags);
}

bool UIManager::inputFloat4(const std::string& label, glm::vec4* value, const std::string& format, int flags)
{
    return ImGui::InputFloat4(label.c_str(), &value->x, format.c_str(), flags);
}

bool UIManager::inputInt(const std::string& label, int* value, int step, int stepFast, int flags)
{
    return ImGui::InputInt(label.c_str(), value, step, stepFast, flags);
}

bool UIManager::inputInt2(const std::string& label, glm::ivec2* value, int flags)
{
    return ImGui::InputInt2(label.c_str(), &value->x, flags);
}

bool UIManager::inputInt3(const std::string& label, glm::ivec3* value, int flags)
{
    return ImGui::InputInt3(label.c_str(), &value->x, flags);
}

bool UIManager::inputInt4(const std::string& label, glm::ivec4* value, int flags)
{
    return ImGui::InputInt4(label.c_str(), &value->x, flags);
}

//==== Color Pickers ====

bool UIManager::colorEdit3(const std::string& label, glm::vec3* color, int flags)
{
    return ImGui::ColorEdit3(label.c_str(), &color->x, flags);
}

bool UIManager::colorEdit4(const std::string& label, glm::vec4* color, int flags)
{
    return ImGui::ColorEdit4(label.c_str(), &color->x, flags);
}

bool UIManager::colorButton(const std::string& label, const glm::vec3& color, int flags, const glm::vec2& size)
{
    return ImGui::ColorButton(label.c_str(), ImVec4(color.r, color.g, color.b, 1.0f), flags, ImVec2(size.x, size.y));
}

bool UIManager::colorButton(const std::string& label, const glm::vec4& color, int flags, const glm::vec2& size)
{
    return ImGui::ColorButton(label.c_str(), ImVec4(color.r, color.g, color.b, color.a), flags, ImVec2(size.x, size.y));
}

//==== Selection Controls ====

bool UIManager::combo(const std::string& label, int* currentItem, const std::vector<std::string>& items, int popupMaxHeightInItems)
{
    // Convert items vector to char* array
    std::vector<const char*> itemsArray;
    for (const auto& item : items)
    {
        itemsArray.push_back(item.c_str());
    }

    return ImGui::Combo(label.c_str(), currentItem, itemsArray.data(), static_cast<int>(items.size()), popupMaxHeightInItems);
}

bool UIManager::selectable(const std::string& label, bool* selected, int flags, const glm::vec2& size)
{
    return ImGui::Selectable(label.c_str(), selected, flags, ImVec2(size.x, size.y));
}

bool UIManager::selectable(const std::string& label, bool selected, int flags, const glm::vec2& size)
{
    return ImGui::Selectable(label.c_str(), selected, flags, ImVec2(size.x, size.y));
}

bool UIManager::listBox(const std::string& label, int* currentItem, const std::vector<std::string>& items, int heightInItems)
{
    // Convert items vector to char* array
    std::vector<const char*> itemsArray;
    for (const auto& item : items)
    {
        itemsArray.push_back(item.c_str());
    }

    return ImGui::ListBox(label.c_str(), currentItem, itemsArray.data(), static_cast<int>(items.size()), heightInItems);
}

//==== Tooltip Helpers ====

void UIManager::setTooltip(const std::string& text)
{
    ImGui::SetTooltip("%s", text.c_str());
}

void UIManager::helpMarker(const std::string& description)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
        ImGui::TextUnformatted(description.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

//==== Image Display ====

void UIManager::image(const std::string& textureName, const glm::vec2& size,
                      const glm::vec2& uv0, const glm::vec2& uv1,
                      const glm::vec4& tintColor,
                      const glm::vec4& borderColor)
{
    auto it = m_textures.find(textureName);
    if (it != m_textures.end())
    {
        // Properly cast the textureId to ImTextureID
        ImTextureID texId = reinterpret_cast<ImTextureID>(it->second.textureId);

        ImGui::Image(
            texId,
            ImVec2(size.x, size.y),
            ImVec2(uv0.x, uv0.y),
            ImVec2(uv1.x, uv1.y),
            ImVec4(tintColor.r, tintColor.g, tintColor.b, tintColor.a),
            ImVec4(borderColor.r, borderColor.g, borderColor.b, borderColor.a)
        );
    }
}

//==== Notification System ====

void UIManager::showNotification(const std::string& message, bool isError, float duration)
{
    if (duration <= 0.0f)
    {
        duration = m_notificationDuration;
    }
    m_notifications.emplace_back(message, isError, duration);
}

void UIManager::showError(const std::string& message, float duration)
{
    showNotification(message, true, duration);
}

void UIManager::renderNotifications()
{
    if (m_notifications.empty())
        return;

    // Update notification timers and remove expired ones
    float deltaTime = ImGui::GetIO().DeltaTime;
    updateNotifications(deltaTime);

    // Get viewport size and calculate notification position
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    float windowWidth = viewport->Size.x;
    float windowPadding = 20.0f;
    float notificationWidth = std::min(400.0f, windowWidth - 2 * windowPadding);

    ImVec2 windowPos = ImVec2(
        viewport->Pos.x + viewport->Size.x - notificationWidth - windowPadding,
        viewport->Pos.y + windowPadding
    );

    // Draw notifications from top down
    for (size_t i = 0; i < m_notifications.size(); i++)
    {
        const auto& notification = m_notifications[i];

        // Set notification color based on type
        if (notification.isError)
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.7f, 0.0f, 0.0f, 0.8f));
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.8f));
        }

        // Create unique window name for this notification
        std::string windowName = "##Notification" + std::to_string(i);

        // Set window position and size
        ImGui::SetNextWindowPos(windowPos);
        ImGui::SetNextWindowSize(ImVec2(notificationWidth, 0));
        ImGui::SetNextWindowBgAlpha(0.8f);

        // Draw notification window
        ImGui::Begin(windowName.c_str(), nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoFocusOnAppearing);

        // Display notification text
        ImGui::Text("%s", notification.message.c_str());

        // Display progress bar
        float progress = 1.0f - (notification.timeRemaining / notification.totalTime);
        ImGui::ProgressBar(progress, ImVec2(-1, 5), "");

        ImGui::End();
        ImGui::PopStyleColor();

        // Update position for the next notification
        windowPos.y += ImGui::GetItemRectSize().y + 10.0f;
    }
}

void UIManager::updateNotifications(float deltaTime)
{
    for (auto it = m_notifications.begin(); it != m_notifications.end(); )
    {
        it->timeRemaining -= deltaTime;
        if (it->timeRemaining <= 0.0f)
        {
            it = m_notifications.erase(it);
        }
        else
        {
            it++;
        }
    }
}

//==== Modal Dialog Helpers ====

void UIManager::showConfirmDialog(const std::string& title, const std::string& message,
                                  std::function<void()> onConfirm,
                                  std::function<void()> onCancel)
{
    m_showConfirmDialog = true;
    m_dialogTitle = title;
    m_dialogMessage = message;
    m_confirmCallback = onConfirm;
    m_cancelCallback = onCancel;

    ImGui::OpenPopup(title.c_str());
}

void UIManager::showInputDialog(const std::string& title, const std::string& message,
                                const std::string& defaultValue,
                                std::function<void(const std::string&)> onConfirm,
                                std::function<void()> onCancel)
{
    m_showInputDialog = true;
    m_dialogTitle = title;
    m_dialogMessage = message;
    m_inputBuffer = defaultValue;
    m_inputCallback = onConfirm;
    m_cancelCallback = onCancel;

    ImGui::OpenPopup(title.c_str());
}

void UIManager::renderConfirmDialog()
{
    if (!m_showConfirmDialog)
        return;

    // Center the modal in the viewport
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(m_dialogTitle.c_str(), &m_showConfirmDialog, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("%s", m_dialogMessage.c_str());
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_showConfirmDialog = false;
            if (m_confirmCallback)
            {
                m_confirmCallback();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_showConfirmDialog = false;
            if (m_cancelCallback)
            {
                m_cancelCallback();
            }
        }

        ImGui::EndPopup();
    }
}

void UIManager::renderInputDialog()
{
    if (!m_showInputDialog)
        return;

    // Center the modal in the viewport
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(m_dialogTitle.c_str(), &m_showInputDialog, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("%s", m_dialogMessage.c_str());
        ImGui::Separator();

        // Calculate dialog width based on content
        const float dialogWidth = 300.0f;
        ImGui::SetNextItemWidth(dialogWidth);

        // Create a character buffer from the string
        char inputBuffer[256] = {0};
        strncpy(inputBuffer, m_inputBuffer.c_str(), sizeof(inputBuffer) - 1);

        // Input field
        bool enterPressed = ImGui::InputText("##input", inputBuffer, sizeof(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue);

        // Update the string if input changed
        if (ImGui::IsItemDeactivatedAfterEdit() || enterPressed)
        {
            m_inputBuffer = inputBuffer;
        }

        if (ImGui::Button("OK", ImVec2(120, 0)) || enterPressed)
        {
            ImGui::CloseCurrentPopup();
            m_showInputDialog = false;
            if (m_inputCallback)
            {
                m_inputCallback(m_inputBuffer);
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_showInputDialog = false;
            if (m_cancelCallback)
            {
                m_cancelCallback();
            }
        }

        ImGui::EndPopup();
    }
}

//==== Drag and Drop ====

bool UIManager::beginDragDropSource(int flags)
{
    return ImGui::BeginDragDropSource(flags);
}

bool UIManager::setDragDropPayload(const std::string& type, const void* data, size_t size, int cond)
{
    return ImGui::SetDragDropPayload(type.c_str(), data, size, cond);
}

void UIManager::endDragDropSource()
{
    ImGui::EndDragDropSource();
}

bool UIManager::beginDragDropTarget()
{
    return ImGui::BeginDragDropTarget();
}

const void* UIManager::acceptDragDropPayload(const std::string& type, int flags)
{
    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(type.c_str(), flags);
    return payload ? payload->Data : nullptr;
}

void UIManager::endDragDropTarget()
{
    ImGui::EndDragDropTarget();
}

//==== Utility Functions ====

bool UIManager::isAnyItemHovered() const
{
    return ImGui::IsAnyItemHovered();
}

bool UIManager::isAnyItemActive() const
{
    return ImGui::IsAnyItemActive();
}

bool UIManager::isAnyItemFocused() const
{
    return ImGui::IsAnyItemFocused();
}

glm::vec2 UIManager::getViewportSize() const
{
    ImVec2 size = ImGui::GetMainViewport()->Size;
    return glm::vec2(size.x, size.y);
}

float UIManager::getDisplayScale() const
{
    return m_dpiScale;
}

void UIManager::setTheme(const std::string& themeName)
{
    ImGuiStyle& style = ImGui::GetStyle();

    if (themeName == "Dark")
    {
        ImGui::StyleColorsDark();
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.33f, 0.33f, 0.35f, 1.00f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.25, 0.25f, 0.27f, 1.00f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.33f, 0.33f, 0.35f, 1.00f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    }
    else if (themeName == "Light")
    {
        ImGui::StyleColorsLight();
    }
    else if (themeName == "Classic")
    {
        ImGui::StyleColorsClassic();
    }

    // Set modern style elements
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 16.0f;
    style.GrabMinSize = 8.0f;
}

void UIManager::setFontScale(float scale)
{
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = scale;
}

bool UIManager::setFont(const std::string& fontPath, float fontSize, bool merge)
{
    ImGuiIO& io = ImGui::GetIO();

    // Apply DPI scaling to font size
    float scaledFontSize = fontSize * m_dpiScale;

    ImFont* font = nullptr;

    if (merge)
    {
        // Add font while preserving existing
        font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), scaledFontSize);
    }
    else
    {
        // Clear existing fonts
        io.Fonts->Clear();

        // Add new font as default
        font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), scaledFontSize);

        // If font loading failed, add the default font as fallback
        if (font == nullptr)
        {
            io.Fonts->AddFontDefault();
        }
    }

    // Rebuild font atlas
    io.Fonts->Build();

    // Update font texture in the backend
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_CreateFontsTexture();

    return font != nullptr;
}