#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include <glm.hpp>
#include <GLFW/glfw3.h>

// Forward declarations
class Application;
class RenderTarget;
class Texture;

/**
 * @class UIManager
 * @brief Core UI system for the engine, providing a direct wrapper around ImGui
 * 
 * The UIManager class handles initialization, rendering, and shutdown of the ImGui
 * library, while providing a clean interface for creating UI elements that match
 * the engine's architecture and style. It supports dockable windows, panels, modal
 * popups, and various input controls required by the editor tools.
 */
class UIManager
{
public:
    /**
     * @brief Get singleton instance
     * @return Reference to UIManager instance
     */
    static UIManager& getInstance();

    /**
     * @brief Initialize the UI system
     * @param window GLFW window for ImGui context
     * @return True if initialization succeeded
     */
    bool initialize(GLFWwindow* window);

    /**
     * @brief Shut down the UI system
     */
    void shutdown();

    /**
     * @brief Begin a new frame for UI rendering
     */
    void beginFrame();

    /**
     * @brief Render all UI elements
     */
    void render();

    /**
     * @brief Set application reference
     * @param application Pointer to application instance
     */
    void setApplication(Application* application);

    /**
     * @brief Set a texture for UI use
     * @param name Name to identify the texture
     * @param textureId OpenGL texture ID
     * @param width Texture width
     * @param height Texture height
     */
    void setTexture(const std::string& name, unsigned int textureId, int width, int height);

    /**
     * @brief Set a RenderTarget as a texture for UI use
     * @param name Name to identify the texture
     * @param target RenderTarget to use as texture
     */
    void setRenderTarget(const std::string& name, RenderTarget* target);

    /**
     * @brief Set a Texture for UI use
     * @param name Name to identify the texture
     * @param texture Texture to use
     */
    void setTexture(const std::string& name, Texture* texture);

    /**
     * @brief Set high DPI scaling factor
     * @param scale DPI scale factor
     */
    void setDpiScale(float scale);

    //==== Window and Panel Management ====

    /**
     * @brief Begin a dockspace for window docking
     * @param id Identifier for the dockspace
     * @return True if dockspace was successfully begun
     */
    bool beginDockspace(const std::string& id);

    /**
     * @brief End a dockspace previously begun with beginDockspace
     */
    void endDockspace();

    /**
     * @brief Begin a new window
     * @param name Window title
     * @param open Pointer to boolean controlling window visibility
     * @param flags Window flags
     * @return True if window was successfully begun
     */
    bool beginWindow(const std::string& name, bool* open = nullptr, int flags = 0);

    /**
     * @brief End a window previously begun with beginFrame
     */
    void endWindow();

    /**
     * @brief Begin a collapsible panel
     * @param name Panel title
     * @param flags Panel flags
     * @return True if panel was successfully begun and is open
     */
    bool beginPanel(const std::string& name, int flags = 0);

    /**
     * @brief End a panel previously begun with beginPanel
     */
    void endPanel();

    /**
     * @brief Begin a modal popup
     * @param name Popup name
     * @return True if popup was successfully begun and is open
     */
    bool beginPopup(const std::string& name);

    /**
     * @brief Open a popup to be shown on next frame
     * @param name Name of the popup to open
     */
    void openPopup(const std::string& name);

    /**
     * @brief End a popup previously begun with beginPopup
     */
    void endPopup();

    /**
     * @brief Begin a menu bar
     * @return True if menu bar was successfully begun
     */
    bool beginMenuBar();

    /**
     * @brief End a menu bar previously begun with beginMenuBar
     */
    void endMenuBar();

    /**
     * @brief Begin a menu
     * @param label Menu label
     * @param enabled Whether menu is enabled
     * @return True if menu was successfully begun and is open
     */
    bool beginMenu(const std::string& label, bool enabled = true);

    /**
     * @brief End a menu previously begun with beginMenu
     */
    void endMenu();

    /**
     * @brief Add a menu item
     * @param label Item label
     * @param shortcut Shortcut text (e.g., "Ctrl+S"), can be empty
     * @param selected Whether item is selected
     * @param enabled Whether item is enabled
     * @return True if item was clicked
     */
    bool menuItem(const std::string& label, const std::string& shortcut = "", bool selected = false, bool enabled = true);

    /**
     * @brief Begin a table
     * @param id Table identifier
     * @param columnCount Number of columns
     * @param flags Table flags
     * @param outerSize Outer size, can be zero for auto
     * @param innerWidth Inner width, can be zero for auto
     * @return True if table was successfully begun
     */
    bool beginTable(const std::string& id, int columnCount, int flags = 0, const glm::vec2& outerSize = {0, 0}, float innerWidth = 0.0f);

    /**
     * @brief End a table previously begun with beginTable
     */
    void endTable();

    /**
     * @brief Setup a table column
     * @param label Column label
     * @param flags Column flags
     * @param width Column width (0.0f for auto)
     */
    void tableSetupColumn(const std::string& label, int flags = 0, float width = 0.0f);

    /**
     * @brief Output table headers based on previously set columns
     */
    void tableHeadersRow();

    /**
     * @brief Go to next table row
     */
    void tableNextRow();

    /**
     * @brief Go to next table column
     * @param columnIndex Column index to go to, or -1 for next
     * @return True if the column is visible
     */
    bool tableNextColumn();

    /**
     * @brief Begin a child window
     * @param id Child identifier
     * @param size Child window size
     * @param border Whether to draw border
     * @param flags Child window flags
     * @return True if child window was successfully begun
     */
    bool beginChild(const std::string& id, const glm::vec2& size = {0, 0}, bool border = false, int flags = 0);

    /**
     * @brief End a child window previously begun with beginChild
     */
    void endChild();

    /**
     * @brief Begin a group
     */
    void beginGroup();

    /**
     * @brief End a group previously begun with beginGroup
     */
    void endGroup();

    /**
     * @brief Begin a tooltip
     */
    void beginTooltip();

    /**
     * @brief End a tooltip previously begun with beginTooltip
     */
    void endTooltip();

    /**
     * @brief Begin a combo box
     * @param label Combo box label
     * @param previewValue Current selected value display text
     * @param flags Combo box flags
     * @return True if combo box was successfully begun and is open
     */
    bool beginCombo(const std::string& label, const std::string& previewValue, int flags = 0);

    /**
     * @brief End a combo box previously begun with beginCombo
     */
    void endCombo();

    /**
     * @brief Begin a list box
     * @param label List box label
     * @param size List box size
     * @return True if list box was successfully begun
     */
    bool beginListBox(const std::string& label, const glm::vec2& size = {0, 0});

    /**
     * @brief End a list box previously begun with beginListBox
     */
    void endListBox();

    /**
     * @brief Begin a tree node
     * @param label Tree node label
     * @param flags Tree node flags
     * @return True if tree node was successfully begun and is open
     */
    bool treeNode(const std::string& label, int flags = 0);

    /**
     * @brief End a tree node previously begun with treeNode
     */
    void treePop();

    //==== Layout Controls ====

    /**
     * @brief Add vertical spacing
     * @param height Amount of space to add
     */
    void spacing(float height = 0.0f);

    /**
     * @brief Begin a new line
     */
    void newLine();

    /**
     * @brief Add a separator line
     */
    void separator();

    /**
     * @brief Set the next item width
     * @param width Width to set
     */
    void setNextItemWidth(float width);

    /**
     * @brief Add indentation
     * @param indent Indentation width
     */
    void indent(float indent = 0.0f);

    /**
     * @brief Remove indentation
     * @param indent Indentation width to remove
     */
    void unindent(float indent = 0.0f);

    /**
     * @brief Push an item onto the ID stack (for creating unique identifiers)
     * @param id Item identifier
     */
    void pushId(const std::string& id);

    /**
     * @brief Push an item onto the ID stack using an integer
     * @param id Integer identifier
     */
    void pushId(int id);

    /**
     * @brief Pop an item from the ID stack
     */
    void popId();

    /**
     * @brief Start a group on same line
     * @param spacing Spacing between elements
     */
    void sameLine(float spacing = 0.0f);

    /**
     * @brief Push a style color
     * @param colorId Color identifier
     * @param color Color value
     */
    void pushStyleColor(int colorId, const glm::vec4& color);

    /**
     * @brief Pop a style color
     */
    void popStyleColor(int count = 1);

    /**
     * @brief Push a style variable
     * @param styleId Style identifier
     * @param value Style value
     */
    void pushStyleVar(int styleId, float value);

    /**
     * @brief Push a style variable (vector version)
     * @param styleId Style identifier
     * @param value Style value
     */
    void pushStyleVar(int styleId, const glm::vec2& value);

    /**
     * @brief Pop a style variable
     */
    void popStyleVar(int count = 1);

    //==== Basic Controls ====

    /**
     * @brief Display text
     * @param text Text to display
     */
    void text(const std::string& text);

    /**
     * @brief Display colored text
     * @param text Text to display
     * @param color Text color
     */
    void textColored(const std::string& text, const glm::vec4& color);

    /**
     * @brief Display wrapped text
     * @param text Text to display (will be word-wrapped)
     */
    void textWrapped(const std::string& text);

    /**
     * @brief Display disabled text
     * @param text Text to display
     */
    void textDisabled(const std::string& text);

    /**
     * @brief Display bullet point text
     * @param text Text to display
     */
    void bulletText(const std::string& text);

    /**
     * @brief Display a label text
     * @param label Label to display
     */
    void labelText(const std::string& label, const std::string& text);

    /**
     * @brief Add a simple button
     * @param label Button label
     * @return True if button was clicked
     */
    bool button(const std::string& label, const glm::vec2& size = {0, 0});

    /**
     * @brief Add a small button
     * @param label Button label
     * @return True if button was clicked
     */
    bool smallButton(const std::string& label);

    /**
     * @brief Add an invisible button (for custom widgets)
     * @param id Button identifier
     * @param size Button size
     * @return True if button was clicked
     */
    bool invisibleButton(const std::string& id, const glm::vec2& size);

    /**
     * @brief Add an image button
     * @param textureName Name of texture registered with setTexture
     * @param size Button size
     * @param uvMin UV coordinates for top-left corner
     * @param uvMax UV coordinates for bottom-right corner
     * @param bgColor Background color
     * @param tintColor Tint color
     * @return True if button was clicked
     */
    bool imageButton(const std::string& textureName, const glm::vec2& size,
                     const glm::vec2& uvMin = {0, 0}, const glm::vec2& uvMax = {1, 1},
                     const glm::vec4& bgColor = {0, 0, 0, 0},
                     const glm::vec4& tintColor = {1, 1, 1, 1});

    /**
     * @brief Display a checkbox
     * @param label Checkbox label
     * @param checked Pointer to boolean value
     * @return True if checkbox value changed
     */
    bool checkbox(const std::string& label, bool* checked);

    /**
     * @brief Display a radio button
     * @param lable Radio button label
     * @param active Whether this radio button is active
     * @return True if radio button is clicked
     */
    bool radioButton(const std::string& label, bool active);

    /**
     * @brief Display a radio button for an enum
     * @param label Radio button label
     * @param v Pointer to enum value
     * @param vButton Value to set if clicked
     * @return True if radio button is clicked
     */
    bool radioButton(const std::string& label, int* v, int vButton);

    /**
     * @brief Display a proress bar
     * @param fraction Progress fraction (0.0f to 1.0f)
     * @param size Bar size
     * @param overlay Text to display on the bar (if any)
     */
    void progressBar(float fraction, const glm::vec2& size = {-1, 0}, const std::string& overlay = "");

    /**
     * @brief Display a bullet point
     */
    void bullet();

    //==== Drag Controls ====

    /**
     * @brief Display a draggable float value
     * @param label Control label
     * @param value Pointer to float value
     * @param speed Drag speed
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Drag flags
     * @return True if value changed
     */
    bool dragFloat(const std::string& label, float* value, float speed = 1.0f, float min = 0.0f, float max = 0.0f, const std::string& format = "%.3f", int flags = 0);

    /**
     * @brief Display a draggable 2D float vector
     * @param label Control label
     * @param value Pointer to vec2 value
     * @param speed Drag speed
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Drag flags
     * @return True if value changed
     */
    bool dragFloat2(const std::string& label, glm::vec2* value, float speed = 1.0f, float min = 0.0f, float max = 0.0f, const std::string& format = "%.3f", int flags = 0);

    /**
     * @brief Display a draggable 3D float vector
     * @param label Control label
     * @param value Pointer to vec3 value
     * @param speed Drag speed
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Drag flags
     * @return True if value changed
     */
    bool dragFloat3(const std::string& label, glm::vec3* value, float speed = 1.0f, float min = 0.0f, float max = 0.0f, const std::string& format = "%.3f", int flags = 0);

    /**
     * @brief Display a draggable 4D float vector
     * @param label Control label
     * @param value Pointer to vec4 value
     * @param speed Drag speed
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Drag flags
     * @return True if value changed
     */
    bool dragFloat4(const std::string& label, glm::vec4* value, float speed = 1.0f, float min = 0.0f, float max = 0.0f, const std::string& format = "%.3f", int flags = 0);

    /**
     * @brief Display a draggable int value
     * @param label Control label
     * @param value Pointer to int value
     * @param speed Drag speed
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Drag flags
     * @return True if value changed
     */
    bool dragInt(const std::string& label, int* value, float speed = 1.0f, int min = 0, int max = 0, const std::string& format = "%d", int flags = 0);

    /**
     * @brief Display a draggable 2D int vector
     * @param label Control label
     * @param value Pointer to ivec2 value
     * @param speed Drag speed
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Drag flags
     * @return True if value changed
     */
    bool dragInt2(const std::string& label, glm::ivec2* value, float speed = 1.0f, int min = 0, int max = 0, const std::string& format = "%d", int flags = 0);

    /**
     * @brief Display a draggable 3D int vector
     * @param label Control label
     * @param value Pointer to ivec3 value
     * @param speed Drag speed
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Drag flags
     * @return True if value changed
     */
    bool dragInt3(const std::string& label, glm::ivec3* value, float speed = 1.0f, int min = 0, int max = 0, const std::string& format = "%d", int flags = 0);
    
    /**
     * @brief Display a draggable 4D int vector
     * @param label Control label
     * @param value Pointer to ivec4 value
     * @param speed Drag speed
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Drag flags
     * @return True if value changed
     */
    bool dragInt4(const std::string& label, glm::ivec4* value, float speed = 1.0f, int min = 0, int max = 0, const std::string& format = "%d", int flags = 0);

    //==== Slider Controls ====

    /**
     * @brief Display a slider for float value
     * @param label Control label
     * @param value Pointer to float value
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Slider flags
     * @return True if value changed
     */
    bool sliderFloat(const std::string& label, float* value, float min, float max, const std::string& format = "%.3f", int flags = 0);

    /**
     * @brief Display a slider for 2D float vector
     * @param label Control label
     * @param value Pointer to vec2 value
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Slider flags
     * @return True if value changed
     */
    bool sliderFloat2(const std::string& label, glm::vec2* value, float min, float max, const std::string& format = "%.3f", int flags = 0);

    /**
     * @brief Display a slider for 3D float vector
     * @param label Control label
     * @param value Pointer to vec3 value
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Slider flags
     * @return True if value changed
     */
    bool sliderFloat3(const std::string& label, glm::vec3* value, float min, float max, const std::string& format = "%.3f", int flags = 0);

    /**
     * @brief Display a slider for 4D float vector
     * @param label Control label
     * @param value Pointer to vec4 value
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Slider flags
     * @return True if value changed
     */
    bool sliderFloat4(const std::string& label, glm::vec4* value, float min, float max, const std::string& format = "%.3f", int flags = 0);

    /**
     * @brief Display a slider for int value
     * @param label Control label
     * @param value Pointer to int value
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Slider flags
     * @return True if value changed
     */
    bool sliderInt(const std::string& label, int* value, int min, int max, const std::string& format = "%d", int flags = 0);

    /**
     * @brief Display a slider for 2D int vector
     * @param label Control label
     * @param value Pointer to ivec2 value
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Slider flags
     * @return True if value changed
     */
    bool sliderInt2(const std::string& label, glm::ivec2* value, int min, int max, const std::string& format = "%d", int flags = 0);

    /**
     * @brief Display a slider for 3D int vector
     * @param label Control label
     * @param value Pointer to ivec3 value
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Slider flags
     * @return True if value changed
     */
    bool sliderInt3(const std::string& label, glm::ivec3* value, int min, int max, const std::string& format = "%d", int flags = 0);

    /**
     * @brief Display a slider for 4D int vector
     * @param label Control label
     * @param value Pointer to ivec4 value
     * @param min Minimum value
     * @param max Maximum value
     * @param format Display format
     * @param flags Slider flags
     * @return True if value changed
     */
    bool sliderInt4(const std::string& label, glm::ivec4* value, int min, int max, const std::string& format = "%d", int flags = 0);

    /**
     * @brief Display a slider for an angle value
     * @param label Control value
     * @param radians Pointer to angle in radians
     * @param degreesMin Minimum value in degrees
     * @param degreesMax Maximum value in degrees
     * @param format Display format
     * @param flags Slider flags
     * @return True if value changed
     */
    bool sliderAngle(const std::string& label, float* radians, float degreesMin = -360.0f, float degreesMax = 360.0f, const std::string& format = "%.0f deg", int flags = 0);

    //==== Input Controls ====

    /**
     * @brief Display an input field for text
     * @param label Control label
     * @param buffer Pointer to string buffer
     * @param bufferSize Size of buffer
     * @param flags Input text flags
     * @return True if value changed
     */
    bool inputText(const std::string& label, char* buffer, size_t bufferSize, int flags = 0);

    /**
     * @brief Display a multiline text input field
     * @param label Control label
     * @param buffer Pointer to string buffer
     * @param bufferSize Size of buffer
     * @param size Field size
     * @param flags Input text flags
     * @return True if value changed
     */
    bool inputTextMultiline(const std::string& label, char* buffer, size_t bufferSize, const glm::vec2& size = {0, 0}, int flags = 0);

    /**
     * @brief Display an input field for a float
     * @param label Control label
     * @param value Pointer to float value
     * @param step Step size
     * @param stepFast Fast step size
     * @param format Display format
     * @param flags Input flags
     * @return True if value changed
     */
    bool inputFloat(const std::string& label, float* value, float step = 0.0f, float stepFast = 0.0f, const std::string& format = "%.3f", int flags = 0);

    /**
     * @brief Display an input field for a 2D float vector
     * @param label Control label
     * @param value Pointer to vec2 value
     * @param format Display format
     * @param flags Input flags
     * @return True if value changed
     */
    bool inputFloat2(const std::string& label, glm::vec2* value, const std::string& format = "%.3f", int flags = 0);

    /**
     * @brief Display an input field for a 3D float vector
     * @param label Control label
     * @param value Pointer to vec3 value
     * @param format Display format
     * @param flags Input flags
     * @return True if value changed
     */
    bool inputFloat3(const std::string& label, glm::vec3* value, const std::string& format = "%.3f", int flags = 0);
    
    /**
     * @brief Display an input field for a 4D float vector
     * @param label Control label
     * @param value Pointer to vec4 value
     * @param format Display format
     * @param flags Input flags
     * @return True if value changed
     */
    bool inputFloat4(const std::string& label, glm::vec4* value, const std::string& format = "%.3f", int flags = 0);

    /**
     * @brief Display an input field for an int
     * @param label Control label
     * @param value Pointer to int value
     * @param step Step size
     * @param stepFast Fast step size
     * @param flags Input flags
     * @return True if value changed
     */
    bool inputInt(const std::string& label, int* value, int step = 1, int stepFast = 100, int flags = 0);

    /**
     * @brief Display an input field for a 2D int vector
     * @param label Control label
     * @param value Pointer to ivec2 value
     * @param flags Input flags
     * @return True if value changed
     */
    bool inputInt2(const std::string& label, glm::ivec2* value, int flags = 0);

    /**
     * @brief Display an input field for a 3D int vector
     * @param label Control label
     * @param value Pointer to ivec3 value
     * @param flags Input flags
     * @return True if value changed
     */
    bool inputInt3(const std::string& label, glm::ivec3* value, int flags = 0);

    /**
     * @brief Display an input field for a 4D int vector
     * @param label Control label
     * @param value Pointer to ivec4 value
     * @param flags Input flags
     * @return True if value changed
     */
    bool inputInt4(const std::string& label, glm::ivec4* value, int flags = 0);

    //==== Color Pickers ====

    /**
     * @brief Display a color picker for RGB
     * @param label Control label
     * @param color Pointer to RGB color
     * @param flags Color picker flags
     * @return True if value changed
     */
    bool colorEdit3(const std::string& label, glm::vec3* color, int flags = 0);

    /**
     * @brief Display a color picker for RGBA
     * @param label Control label
     * @param color Pointer to RGBA color
     * @param flags Color picker flags
     * @return True if value changed
     */
    bool colorEdit4(const std::string& label, glm::vec4* color, int flags = 0);

    /**
     * @brief Display a color picker button for RGB
     * @param label Control label
     * @param color RGB color
     * @param flags Color picker flags
     * @param size Button size
     * @return True if picker was opened
     */
    bool colorButton(const std::string& label, const glm::vec3& color, int flags = 0, const glm::vec2& size = {0, 0});

    /**
     * @brief Display a color picker button for RGBA
     * @param label Control label
     * @param color RGBA color
     * @param flags Color picker flags
     * @param size Button size
     * @return True if picker was opened
     */
    bool colorButton(const std::string& label, const glm::vec4& color, int flags = 0, const glm::vec2& size = {0, 0});

    //==== Selection Controls ====

    /**
     * @brief Display a combo box for selection
     * @param label Control label
     * @param currentItem Pointer to current item index
     * @param items Vector of items
     * @param popupMaxHeightInItems Maximum height of popup in items
     * @return True if selection changed
     */
    bool combo(const std::string& label, int* currentItem, const std::vector<std::string>& items, int popupMaxHeightInItems = -1);

    /**
     * @brief Display a selectable item
     * @param label Item label
     * @param selected Pointer to selection state
     * @param flags Selectable flags
     * @param size Selection box size
     * @return True if item was clicked
     */
    bool selectable(const std::string& label, bool* selected, int flags = 0, const glm::vec2& size = {0, 0});

    /**
     * @brief Display a selectable item (read-only version)
     * @param label Item label
     * @param selected Whether the item is selected
     * @param flags Selectable flags
     * @param size Selection box size
     * @return True if item was clicked
     */
    bool selectable(const std::string& label, bool selected, int flags = 0, const glm::vec2& size = {0, 0});

    /**
     * @brief Display a list box with items
     * @param label Control label
     * @param currentItem Pointer to current item index
     * @param items Vector of items
     * @return True if selection changed
     */
    bool listBox(const std::string& label, int* currentItem, const std::vector<std::string>& items, int heightInItems = -1);

    //==== Tooltip Helpers ====

    /**
     * @brief Set a tooltip for the previous item
     * @param text Tooltip text
     */
    void setTooltip(const std::string& text);

    /**
     * @brief Display help marker (?) with tooltip on hover
     * @param description Text to show in tooltip
     */
    void helpMarker(const std::string& description);

    //==== Image Display ====

    /**
     * @brief Display an image
     * @param textureName Name of texture registered with setTexture
     * @param size Image size
     * @param uv0 UV coordinates for top-left corner
     * @param uv1 UV coordinates for bottom-right corner
     * @param tintColor Tint color
     * @param borderColor Border color
     */
    void image(const std::string& textureName, const glm::vec2& size,
               const glm::vec2& uv0 = {0, 0}, const glm::vec2& uv1 = {1, 1},
               const glm::vec4& tintColor = {1, 1, 1, 1},
               const glm::vec4& borderColor = {0, 0, 0, 0});

    //==== Notification System ====

    /**
     * @brief Show a notification message
     * @param message Message text
     * @param isError Whether this is an error notification
     * @param duration Duration in seconds (0 for default)
     */
    void showNotification(const std::string& message, bool isError = false, float duration = 0.0f);

    /**
     * @brief Show an error notification
     * @param message Error message text
     * @param duration Duration in seconds (0 for default)
     */
    void showError(const std::string& message, float duration = 0.0f);

    /**
     * @brief Render all pending notifications
     */
    void renderNotifications();

    //==== Modal Dialog Helpers ====

    /**
     * @brief Show a confirmation dialog
     * @param title Dialog title
     * @param message Dialog message
     * @param onConfirm Callback to execute on confirm
     * @param onCancel Callback to execute on cancel (optional)
     */
    void showConfirmDialog(const std::string& title, const std::string& message,
                           std::function<void()> onConfirm,
                           std::function<void()> onCancel = nullptr);

    /**
     * @brief Show an input dialog
     * @param title Dialog title
     * @param message Dialog message
     * @param defaultValue Default input value
     * @param onConfirm Callback to execute on confirm, receives input value
     * @param onCancel Callback to execute on cancel (optional)
     */
    void showInputDialog(const std::string& title, const std::string& message,
                         const std::string& defaultValue,
                         std::function<void(const std::string&)> onConfirm,
                         std::function<void()> onCancel = nullptr);

    //==== Drag and Drop ====

    /**
     * @brief Begin drag drop source
     * @param flags Drag drop flags
     * @return True if drag and drop started
     */
    bool beginDragDropSource(int flags = 0);

    /**
     * @brief Set a drag drop payload
     * @param type Payload type identifier
     * @param data Pointer to payload data
     * @param size Size of payload data
     * @param cond Condition flags
     * @return True if accepted
     */
    bool setDragDropPayload(const std::string& type, const void* data, size_t size, int cond = 0);

    /**
     * @brief End drag drop source
     */
    void endDragDropSource();

    /**
     * @brief Begin drag drop target
     * @return True if potential drag and drop target
     */
    bool beginDragDropTarget();

    /**
     * @brief Accept drag drop payload
     * @param type Expected payload type
     * @param flags Accept flags
     * @return Payload data or nullptr
     */
    const void* acceptDragDropPayload(const std::string& type, int flags = 0);

    /**
     * @brief End drag drop target
     */
    void endDragDropTarget();

    //==== Utility Functions ====

    /**
     * @brief Check if UI is currently being hovered
     * @return True if any UI element is hovered
     */
    bool isAnyItemHovered() const;

    /**
     * @brief Check if UI is currently active (e.g., dragging)
     * @return True if any UI element is active
     */
    bool isAnyItemActive() const;

    /**
     * @brief Check if UI is currently focused
     * @return True if any UI element is focused
     */
    bool isAnyItemFocused() const;

    /**
     * @brief Get current UI system viewport size
     * @return Viewport size as vec2
     */
    glm::vec2 getViewportSize() const;

    /**
     * @brief Get current UI system display scale
     * @return Display scale factor
     */
    float getDisplayScale() const;

    /**
     * @brief Set theme colors
     * @param themeName Theme name ("Dark", "Light", "Classic")
     */
    void setTheme(const std::string& themeName);

    /**
     * @brief set UI font scale
     * @param scale Font scale factor
     */
    void setFontScale(float scale);

    /**
     * @brief Set default font
     * @param fontPath Path to font file
     * @param fontSize Font size in pixels
     * @param merge Whether to merge with existing fonts
     * @return True if font was set successfully
     */
    bool setFont(const std::string& fontPath, float fontSize = 13.0f, bool merge = false);

private:
    // Singleton instance
    static UIManager* s_instance;

    // Constructor and destructor for singleton
    UIManager();
    ~UIManager();

    // Prevent copy/move
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;
    UIManager(UIManager&&) = delete;
    UIManager& operator=(UIManager&&) = delete;

    // GLFW window reference
    GLFWwindow* m_window = nullptr;

    // Application reference
    Application* m_application = nullptr;

    // Texture registry (name -> ImTextureID)
    struct TextureInfo
    {
        void* textureId = nullptr;
        int width = 0;
        int height = 0;
    };
    std::unordered_map<std::string, TextureInfo> m_textures;

    // DPI scaling
    float m_dpiScale = 1.0f;

    // Notification system
    struct Notification
    {
        std::string message;
        bool isError;
        float timeRemaining;
        float totalTime;

        Notification(const std::string& msg, bool error, float duration) :
            message(msg), isError(error), timeRemaining(duration), totalTime(duration)
        {
        }
    };
    std::vector<Notification> m_notifications;
    float m_notificationDuration = 5.0f;

    // Modal dialog state
    bool m_showConfirmDialog = false;
    bool m_showInputDialog = false;
    std::string m_dialogTitle;
    std::string m_dialogMessage;
    std::string m_inputBuffer;
    std::function<void()> m_confirmCallback;
    std::function<void()> m_cancelCallback;
    std::function<void(const std::string&)> m_inputCallback;

    // Internal helpers
    void renderConfirmDialog();
    void renderInputDialog();
    void updateNotifications(float deltaTime);
};

// Flags for UI elements (matching ImGui flags but with our own names)
namespace UIFlags
{
    // Window flags
    const int WindowNoTitleBar = 1 << 0;
    const int WindowNoResize = 1 << 1;
    const int WindowNoMove = 1 << 2;
    const int WindowScrollBar = 1 << 3;
    const int WindowNoScrollWithMouse = 1 << 4;
    const int WindowNoCollapse = 1 << 5;
    const int WindowAlwaysAutoResize = 1 << 6;
    const int WindowNoBackground = 1 << 7;
    const int WindowNoSavedSettings = 1 << 8;
    const int WindowNoMouseInputs = 1 << 9;
    const int WindowMenuBar = 1 << 10;
    const int WindowHorizontalScrollbar = 1 << 11;
    const int WindowNoFocusOnAppearing = 1 << 12;
    const int WindowNoBringToFrontOnFocus = 1 << 13;
    const int WindowAlwaysVerticalScrollbar = 1 << 14;
    const int WindowAlwaysHorizontalScrollbar = 1 << 15;
    const int WindowAlwaysUseWindowPadding = 1 << 16;
    const int WindowNoNavInputs = 1 << 18;
    const int WindowNoNavFocus = 1 << 19;
    const int WindowUnsavedDocument = 1 << 20;

    // Combo flags
    const int ComboNoPreview = 1 << 0;
    const int ComboHeightSmall = 1 << 1;
    const int ComboHeightRegular = 1 << 2;
    const int ComboHeightLarge = 1 << 3;
    const int ComboHeightLargest = 1 << 4;
    const int ComboPopupAlignLeft = 1 << 5;
    const int ComboNoArrowButton = 1 << 6;
    const int ComboNoPreview = 1 << 7;

    // Selectable flags
    const int SelectableNoHoldingActiveID = 1 << 20;
    const int SelectableNoStateChange = 1 << 21;
    const int SelectableAllowDoubleClick = 1 << 22;
    const int SelectableDisabled = 1 << 23;

    // Input text flags
    const int InputTextNoHorizontalScroll = 1 << 0;
    const int InputTextPasswordMask = 1 << 1;
    const int InputTextCtrlEnterForNewLine = 1 << 4;
    const int InputTextNoUndoRedo = 1 << 5;
    const int InputTextCharsScientific = 1 << 6;
    const int InputTextCallbackCompletion = 1 << 7;
    const int InputTextCallbackHistory = 1 << 8;
    const int InputTextCallbackAlways = 1 << 9;
    const int InputTextCallbackCharFilter = 1 << 10;
    const int InputTextAllowTabInput = 1 << 11;
    const int InputTextCtrlEnterForNewLine = 1 << 12;
    const int InputTextNoHorizontalScroll = 1 << 13;
    const int InputTextAlwaysOverwrite = 1 << 14;
    const int InputTextReadOnly = 1 << 15;
    const int InputTextPassword = 1 << 16;
    const int InputTextNoUndoRedo = 1 << 17;
    const int InputTextCharsUppercase = 1 << 18;
    const int InputTextCharsNoBlank = 1 << 19;
    const int InputTextCharsDecimal = 1 << 20;
    const int InputTextCharsHexadecimal = 1 << 21;
    const int InputTextCharsScientific = 1 << 22;

    // TreeNode flags
    const int TreeNodeSelected = 1 << 0;
    const int TreeNodeFramed = 1 << 1;
    const int TreeNodeAllowItemOverlap = 1 << 2;
    const int TreeNodeNoTreePushOnOpen = 1 << 3;
    const int TreeNodeNoAutoOpenOnLog = 1 << 4;
    const int TreeNodeDefaultOpen = 1 << 5;
    const int TreeNodeOpenOnDoubleClick = 1 << 6;
    const int TreeNodeOpenOnArrow = 1 << 7;
    const int TreeNodeLeaf = 1 << 8;
    const int TreeNodeBullet = 1 << 9;
    const int TreeNodeCollapsingHeader = 1 << 11;

    // Color edit flags
    const int ColorEditNoAlpha = 1 << 1;
    const int ColorEditNoPicker = 1 << 2;
    const int ColorEditNoOptions = 1 << 3;
    const int ColorEditNoSmallPreview = 1 << 4;
    const int ColorEditNoInputs = 1 << 5;
    const int ColorEditNoTooltip = 1 << 6;
    const int ColorEditNoLabel = 1 << 7;
    const int ColorEditNoSidePreview = 1 << 8;
    const int ColorEditAlphaBar = 1 << 9;
    const int ColorEditAlphaPreview = 1 << 10;
    const int ColorEditAlphaPreviewHalf = 1 << 11;
    const int ColorEditHDR = 1 << 12;
    const int ColorEditPickerHueBar = 1 << 13;
    const int ColorEditPickerHueWheel = 1 << 14;
    const int ColorEditRGB = 1 << 15;
    const int ColorEditHSV = 1 << 16;
    const int ColorEditHEX = 1 << 17;
    const int ColorEditRGBFloat = 1 << 18;

    // Drag and Drop flags
    const int DragDropSourceNoPreviewTooltip = 1 << 0;
    const int DragDropSourceNoDisableHover = 1 << 1;
    const int DragDropSourceNoHoldToOpenOthers = 1 << 2;
    const int DragDropSourceAllowNullID = 1 << 3;
    const int DragDropSourceExtern = 1 << 4;
    const int DragDropSourceAutoExpirePayload = 1 << 5;
    const int DragDropAcceptBeforeDelivery = 1 << 10;
    const int DragDropAcceptNoDrawDefaultRect = 1 << 11;
    const int DragDropAcceptNoPreviewTooltip = 1 << 12;
}

// UI colors (matching ImGui colors)
namespace UIColors
{
    const int Text = 0;
    const int TextDisabled = 1;
    const int WindowBg = 2;
    const int ChildBg = 3;
    const int PopupBg = 4;
    const int Border = 5;
    const int BorderShadow = 6;
    const int FrameBg = 7;
    const int FrameBgHovered = 8;
    const int FrameBgActive = 9;
    const int TitleBg = 10;
    const int TitleBgActive = 11;
    const int TitleBgCollapsed = 12;
    const int MenuBarBg = 13;
    const int ScrollbarBg = 14;
    const int ScrollbarGrab = 15;
    const int ScrollbarGrabHovered = 16;
    const int ScrollbarGrabActive = 17;
    const int CheckMark = 18;
    const int SliderGrab = 19;
    const int SliderGrabActive = 20;
    const int Button = 21;
    const int ButtonHovered = 22;
    const int ButtonActive = 23;
    const int Header = 24;
    const int HeaderHovered = 25;
    const int HeaderActive = 26;
    const int Separator = 27;
    const int SeparatorHovered = 28;
    const int SeparatorActive = 29;
    const int ResizeGrip = 30;
    const int ResizeGripHovered = 31;
    const int ResizeGripActive = 32;
    const int Tab = 33;
    const int TabHovered = 34;
    const int TabActive = 35;
    const int TabUnfocused = 36;
    const int TabUnfocusedActive = 37;
    const int DockingPreview = 38;
    const int DockingEmptyBg = 39;
    const int PlotLines = 40;
    const int PlotLinesHovered = 41;
    const int PlotHistogram = 42;
    const int PlotHistogramHovered = 43;
    const int TableHeaderBg = 44;
    const int TableBorderStrong = 45;
    const int TableBorderLight = 46;
    const int TableRowBg = 47;
    const int TableRowBgAlt = 48;
    const int TextSelectedBg = 49;
    const int DragDropTarget = 50;
    const int NavHighlight = 51;
    const int NavWindowingHighlight = 52;
    const int NavWindowingDimBg = 53;
    const int ModalWindowDimBg = 54;
    const int Count = 55;
}