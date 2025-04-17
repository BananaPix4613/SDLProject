#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <glm.hpp>
#include <GLFW/glfw3.h>

// Forward declarations
class Application;
class EventSystem;
class Camera;

/**
 * @enum InputDevice
 * @brief Defines the type of input device
 */
enum class InputDevice
{
    KEYBOARD,
    MOUSE,
    GAMEPAD
};

/**
 * @enum InputState
 * @brief Defines the current state of an input
 */
enum class InputState
{
    RELEASED,    // Input is not active
    PRESSED,     // Input was just pressed this frame
    HELD,        // Input is being held down
    RELEASED_THIS_FRAME // Input was just released this frame
};

/**
 * @enum InputAxis
 * @brief Predefined input axes for common movements
 */
enum class InputAxis
{
    HORIZONTAL,  // Left/right movement
    VERTICAL,    // Up/down movement
    LOOK_X,      // Horizontal look/camera movement
    LOOK_Y,      // Vertical look/camera movement
    ZOOM,        // Zoom in/out
    TRIGGER_LEFT,  // Left trigger (gamepad)
    TRIGGER_RIGHT  // Right trigger (gamepad)
};

/**
 * @struct InputBinding
 * @brief Maps a physical input to a logical action
 */
struct InputBinding
{
    InputDevice device;
    int primaryCode;     // Key code, mouse button, or gamepad button
    int modifierCode;    // Optional modifier key (e.g., CTRL, SHIFT) for keyboard, -1 if not used
    float scale;         // Scale factor for axis inputs

    InputBinding() : device(InputDevice::KEYBOARD), primaryCode(0), modifierCode(-1), scale(1.0f)
    {
    }

    InputBinding(InputDevice dev, int code, int modifier = -1, float scaleValue = 1.0f) :
        device(dev), primaryCode(code), modifierCode(modifier), scale(scaleValue)
    {
    }

    bool operator==(const InputBinding& other) const
    {
        return device == other.device &&
            primaryCode == other.primaryCode &&
            modifierCode == other.modifierCode;
    }
};

/**
 * @class InputContext
 * @brief Defines a set of input bindings for a specific context (e.g., gameplay, editor)
 */
class InputContext
{
public:
    InputContext(const std::string& name);
    ~InputContext();

    /**
     * @brief Add an action binding to this context
     * @param actionName Name of the action
     * @param binding Input binding to trigger the action
     */
    void addActionBinding(const std::string& actionName, const InputBinding& binding);

    /**
     * @brief Add an axis binding to this context
     * @param axisName Name of the axis
     * @param positiveBinding Input binding for positive axis value
     * @param negativeBinding Input binding for negative axis value
     * @param deadzone Minimum value to register axis movement
     */
    void addAxisBinding(const std::string& axisName, const InputBinding& positiveBinding,
                        const InputBinding& negativeBinding, float deadzone = 0.1f);

    /**
     * @brief Map a predefined axis to custom bindings
     * @param axis Predefined axis enum
     * @param positiveBinding Input binding for positive axis value
     * @param negative Minimum value to register axis movement
     */
    void mapAxis(InputAxis axis, const InputBinding& positiveBinding,
                 const InputBinding& negativeBinding, float deadzone = 0.1f);

    /**
     * @brief Get the context name
     * @return Name of this context
     */
    const std::string& getName() const;

    /**
     * @brief Remove all bindings in this context
     */
    void clearBindings();

    // Internal use by InputManager
    friend class InputManager;

private:
    std::string m_name;
    std::unordered_map<std::string, std::vector<InputBinding>> m_actionBindings;

    struct AxisBinding
    {
        InputBinding positiveBinding;
        InputBinding negativeBinding;
        float deadzone;

        AxisBinding() : deadzone(0.1f)
        {
        }
    };

    std::unordered_map<std::string, AxisBinding> m_axisBindings;
    std::unordered_map<InputAxis, std::string> m_predefinedAxisMap;
};

/**
 * @class InputManager
 * @brief Manages input from various devices and dispatches input events
 * 
 * The InputManager provides a unified interface for handling input across keyboard,
 * mouse, the gamepad devices. It supports mapping physical inputs to logical actions,
 * managing different input contexts, and processing both digital and analog inputs.
 */
class InputManager
{
public:
    /**
     * @brief Constructor
     * @param application Reference to the main application
     */
    InputManager(Application* application);

    /**
     * @brief Destructor
     */
    ~InputManager();

    /**
     * @brief Initialize the input manager
     * @return True if initialize was successful
     */
    bool initialize();

    /**
     * @brief Process input events for the current frame
     */
    void update();

    /**
     * @brief Process raw input events from GLFW
     * @param window GLFW window receiving the event
     * @param key Key code
     * @param scancode System-specific scancode
     * @param action GLFW action (press, release, repeat)
     * @param mods Modifier key flags
     */
    void processKeyInput(GLFWwindow* window, int key, int scancode, int action, int mods);

    /**
     * @brief Process mouse button events
     * @param window GLFW window receiving the event
     * @param button Mouse button code
     * @param action GLFW action (press, release)
     * @param mods Modifier key flags
     */
    void processMouseButtonInput(GLFWwindow* window, int button, int action, int mods);

    /**
     * @brief Process mouse movement events
     * @param window GLFW window receiving the event
     * @param xpos X position of the cursor
     * @param ypos Y position of the cursor
     */
    void processMouseMoveInput(GLFWwindow* window, double xpos, double ypos);

    /**
     * @brief Process mouse scroll events
     * @param window GLFW window receiving the event
     * @param xoffset Horizontal scroll offset
     * @param yoffset Vertical scroll offset
     */
    void processMouseScrollInput(GLFWwindow* window, double xoffset, double yoffset);

    /**
     * @brief Create a new input context
     * @param contextName Name of the context
     * @return Pointer to the created context
     */
    InputContext* createContext(const std::string& contextName);

    /**
     * @brief Get an existing input context
     * @param contextName Name of the context
     * @return Pointer to the context or nullptr if not found
     */
    InputContext* getContext(const std::string& contextName);

    /**
     * @brief Activate an input context
     * @param contextName Name of the conte
     */
    void activateContext(const std::string& contextName);

    /**
     * @brief Deactivates an input context
     * @param contextName Name of the context to deactivate
     */
    void deactivateContext(const std::string& contextName);

    /**
     * @brief Check if an action was triggered this frame
     * @param actionName Name of the action
     * @return True if the action was triggered
     */
    bool isActionTriggered(const std::string& actionName) const;

    /**
     * @brief Check if an action is currently active
     * @param actionName Name of the action
     * @return True if the action is active
     */
    bool isActionActive(const std::string& actionName) const;

    /**
     * @brief Check if an action was released this frame
     * @param actionName Name of the action
     * @return True if the action was released
     */
    bool wasActionReleased(const std::string& actionName) const;

    /**
     * @brief Get the value of an axis
     * @param axisName Name of the axis
     * @return Value of the axis (-1.0 to 1.0)
     */
    float getAxisValue(const std::string& axisName) const;

    /**
     * @brief Get the value of a predefined axis
     * @param axis Predefined axis enum
     * @return Value of the axis (-1.0 to 1.0)
     */
    float getAxisValue(InputAxis axis) const;

    /**
     * @brief Get the current mouse position
     * @return Mouse position as a 2D vector
     */
    glm::vec2 getMousePosition() const;

    /**
     * @brief Get the mouse movement delta since last frame
     * @return Mouse movement delta as a 2D vector
     */
    glm::vec2 getMouseDelta() const;

    /**
     * @brief Get the current mouse scroll value
     * @return Mouse scroll value
     */
    float getMouseScrollDelta() const;

    /**
     * @brief Check if a specific key is pressed
     * @param key GLFW key code
     * @return True if the key is pressed
     */
    bool isKeyPressed(int key) const;

    /**
     * @brief Check if a specific mouse button is pressed
     * @param button GLFW mouse button code
     * @return True if the button is pressed
     */
    bool isMouseButtonPressed(int button) const;

    /**
     * @brief Check if a gamepad button is pressed
     * @param gamepadIndex Gamepad index (0-15)
     * @param button GLFW gamepad button code
     * @return True if the button is pressed
     */
    bool isGamepadButtonPressed(int gamepadIndex, int button) const;

    /**
     * @brief Get the value of a gamepad axis
     * @param gamepadIndex Gamepad index (0-15)
     * @param axis GLFW gamepad axis code
     * @return Value of the axis (-1.0 to 1.0)
     */
    float getGamepadAxisValue(int gamepadIndex, int axis) const;

    /**
     * @brief Check if a gamepad is connected
     * @param gamepadIndex Gamepad index (0-15)
     * @return True if gamepad is connected
     */
    bool isGamepadConnected(int gamepadIndex) const;

    /**
     * @brief Get the name of a connected gamepad
     * @param gamepadIndex Gamepad index (0-15)
     * @return Gamepad name or empty string if not connected
     */
    std::string getGamepadName(int gamepadIndex) const;

    /**
     * @brief Set cursor mode (normal, hidden, captured)
     * @param mode GLFW cursor mode
     */
    void setCursorMode(int mode);

    /**
     * @brief Get current cursor mode
     * @return GLFW cursor mode
     */
    int getCursorMode() const;

    /**
     * @brief Save current input bindings to a file
     * @param filename Path to save file
     * @return True if successfully saved
     */
    bool saveBindings(const std::string& filename) const;

    /**
     * @brief Load input bindings from a file
     * @param filename Path to load file
     * @return True if successfully loaded
     */
    bool loadBindings(const std::string& filename);

    /**
     * @brief Set camera for input processing
     * @param camera Camera to use for movement
     */
    void setCamera(Camera* camera);

    /**
     * @brief Get the current state of a key
     * @param key GLFW key code
     * @return Current input state
     */
    InputState getKeyState(int key) const;

    /**
     * @brief Get the current state of a mouse button
     * @param button GLFW mouse button code
     * @return Current input state
     */
    InputState getMouseButtonState(int button) const;

    /**
     * @brief Get the current state of a gamepad button
     * @param gamepadIndex Gamepad index (0-15)
     * @param button GLFW gamepad button code
     * @return Current input state
     */
    InputState getGamepadButtonState(int gamepadIndex, int button) const;

    /**
     * @brief Set input sensitivity
     * @param deviceType Type of input device
     * @param sensitivity Sensitivity value (1.0 is default)
     */
    void setSensitivity(InputDevice deviceType, float sensitivity);

    /**
     * @brief Get input sensitivity
     * @param deviceType Type of input device
     * @return Current sensitivity value
     */
    float getSensitivity(InputDevice deviceType) const;

    /**
     * @brief Create default input contexts
     */
    void createDefaultContexts();

    /**
     * @brief Register an action listener callback
     * @param actionName Name of the action
     * @param callback Function to call when action is triggered
     * @return ID of the registered callback for removal
     */
    int registerActionCallback(const std::string& actionName, std::function<void()> callback);

    /**
     * @brief Register an axis listener callback
     * @param axisName Name of the axis
     * @param callback Function to call when axis value changes
     * @return ID of the registered callback for removal
     */
    int registerAxisCallback(const std::string& axisName, std::function<void(float)> callback);

    /**
     * @brief Unregister an action callback
     * @param actionName Name of the action
     * @param callbackId ID of the callback to remove
     */
    void unregisterActionCallback(const std::string& actionName, int callbackId);

    /**
     * @brief Unregister an axis callback
     * @param axisName Name of the axis
     * @param callbackId ID of the callback to remove
     */
    void unregisterAxisCallback(const std::string& axisName, int callbackId);

private:
    // Application reference
    Application* m_application;

    // Event system reference
    EventSystem* m_eventSystem;

    // Camera reference for camera controls
    Camera* m_camera;

    // Input contexts
    std::unordered_map<std::string, std::unique_ptr<InputContext>> m_contexts;
    std::vector<std::string> m_activeContexts;

    // Input state tracking
    std::unordered_map<int, InputState> m_keyStates;
    std::unordered_map<int, InputState> m_mouseButtonStates;
    std::unordered_map<int, std::unordered_map<int, InputState>> m_gamepadButtonStates;

    // Mouse state
    glm::vec2 m_mousePosition;
    glm::vec2 m_lastMousePosition;
    glm::vec2 m_mouseDelta;
    float m_mouseScrollValue;

    // Axis value cache
    mutable std::unordered_map<std::string, float> m_axisValues;

    // Callback tracking
    struct CallbackData
    {
        int id;
        std::function<void()> actionCallback;
        std::function<void(float)> axisCallback;
    };

    std::unordered_map<std::string, std::vector<CallbackData>> m_actionCallbacks;
    std::unordered_map<std::string, std::vector<CallbackData>> m_axisCallbacks;
    int m_nextCallbackId;

    // Sensitivity settings
    std::unordered_map<InputDevice, float> m_sensitivity;

    // Internal methods
    /**
     * @brief Update action states based on current inputs
     */
    void updateActionStates();

    /**
     * @brief Update axis values based on current inputs
     */
    void updateAxisValues();

    /**
     * @brief Check if an input binding is active
     * @param binding Input binding to check
     * @return True if the binding is active
     */
    bool isBindingActive(const InputBinding& binding) const;

    /**
     * @brief Get the value of an input binding (for analog inputs)
     * @param binding Input binding to check
     * @return Value of the binding (-1.0 to 1.0)
     */
    float getBindingValue(const InputBinding& binding) const;

    /**
     * @brief Update input state tracking
     */
    void updateInputStates();

    /**
     * @brief Trigger registered callbacks for actions and axes
     */
    void triggerCallbacks();

    /**
     * @brief Create and register GLFW callbacks
     */
    void setupGLFWCallbacks();

    /**
     * @brief Update gamepad state
     */
    void updateGamepadState();

    /**
     * @brief Convert GLFW input state to InputState
     * @param glfwState GLFW input state
     * @param currentState Current InputState
     * @return Updated InputState
     */
    InputState convertGLFWState(int glfwState, InputState currentState) const;
};