#include "InputManager.h"
#include "Application.h"
#include "EventSystem.h"
#include "Camera.h"
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

// Static callback wrapper functions for GLFW
static InputManager* s_currentInputManager = nullptr;

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (s_currentInputManager)
    {
        s_currentInputManager->processKeyInput(window, key, scancode, action, mods);
    }
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (s_currentInputManager)
    {
        s_currentInputManager->processMouseButtonInput(window, button, action, mods);
    }
}

static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (s_currentInputManager)
    {
        s_currentInputManager->processMouseMoveInput(window, xpos, ypos);
    }
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (s_currentInputManager)
    {
        s_currentInputManager->processMouseScrollInput(window, xoffset, yoffset);
    }
}

// InputContext implementation
InputContext::InputContext(const std::string& name) :
    m_name(name)
{
}

InputContext::~InputContext()
{
    clearBindings();
}

void InputContext::addActionBinding(const std::string& actionName, const InputBinding& binding)
{
    m_actionBindings[actionName].push_back(binding);
}

void InputContext::addAxisBinding(const std::string& axisName, const InputBinding& positiveBinding,
                                  const InputBinding& negativeBinding, float deadzone)
{
    AxisBinding axisBinding;
    axisBinding.positiveBinding = positiveBinding;
    axisBinding.negativeBinding = negativeBinding;
    axisBinding.deadzone = deadzone;

    m_axisBindings[axisName] = axisBinding;
}

void InputContext::mapAxis(InputAxis axis, const InputBinding& positiveBinding,
                           const InputBinding& negativeBinding, float deadzone)
{
    std::string axisName;

    switch (axis)
    {
        case InputAxis::HORIZONTAL:
            axisName = "Horizontal";
            break;
        case InputAxis::VERTICAL:
            axisName = "Vertical";
            break;
        case InputAxis::LOOK_X:
            axisName = "LookX";
            break;
        case InputAxis::LOOK_Y:
            axisName = "LookY";
            break;
        case InputAxis::ZOOM:
            axisName = "Zoom";
            break;
        case InputAxis::TRIGGER_LEFT:
            axisName = "TriggerLeft";
            break;
        case InputAxis::TRIGGER_RIGHT:
            axisName = "TriggerRight";
            break;
        default:
            axisName = "Unknown";
            break;
    }

    // Store the mapping
    m_predefinedAxisMap[axis] = axisName;

    // Create the actual axis binding
    addAxisBinding(axisName, positiveBinding, negativeBinding, deadzone);
}

const std::string& InputContext::getName() const
{
    return m_name;
}

void InputContext::clearBindings()
{
    m_actionBindings.clear();
    m_axisBindings.clear();
    m_predefinedAxisMap.clear();
}

// InputManager implementation
InputManager::InputManager(Application* application) :
    m_application(application),
    m_eventSystem(nullptr),
    m_camera(nullptr),
    m_mousePosition(0.0f),
    m_lastMousePosition(0.0f),
    m_mouseDelta(0.0f),
    m_mouseScrollValue(0.0f),
    m_nextCallbackId(1)
{
    // Set default sensitivity values
    m_sensitivity[InputDevice::KEYBOARD] = 1.0f;
    m_sensitivity[InputDevice::MOUSE] = 1.0f;
    m_sensitivity[InputDevice::GAMEPAD] = 1.0f;
}

InputManager::~InputManager()
{
    // Unregister GLFW callbacks
    if (m_application && m_application->getWindow())
    {
        GLFWwindow* window = m_application->getWindow();
        glfwSetKeyCallback(window, nullptr);
        glfwSetMouseButtonCallback(window, nullptr);
        glfwSetCursorPosCallback(window, nullptr);
        glfwSetScrollCallback(window, nullptr);
    }

    // Clear the static reference
    if (s_currentInputManager == this)
    {
        s_currentInputManager = nullptr;
    }

    // Clear contexts
    m_contexts.clear();
}

bool InputManager::initialize()
{
    // Store reference to event system
    m_eventSystem = m_application->getEventSystem();
    if (!m_eventSystem)
    {
        return false;
    }

    // Store reference to this instance for callbacks
    s_currentInputManager = this;

    // Set up GLFW callbacks
    setupGLFWCallbacks();

    // Create default input contexts
    createDefaultContexts();

    return true;
}

void InputManager::update()
{
    // Calculate mouse delta
    m_mouseDelta = m_mousePosition - m_lastMousePosition;
    m_lastMousePosition = m_mousePosition;

    // Update gamepad state
    updateGamepadState();

    // Update input states (transition from PRESSED to HELD, etc.)
    updateInputStates();

    // Update action states based on current inputs
    updateActionStates();

    // Update axis values based on current inputs
    updateAxisValues();

    // Trigger callbacks for actions and axes
    triggerCallbacks();

    // Reset mouse scroll value after processing
    m_mouseScrollValue = 0.0f;
}

void InputManager::processKeyInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Update key state
    InputState newState = convertGLFWState(action, m_keyStates[key]);
    m_keyStates[key] = newState;

    // Dispatch event through event system
    if (m_eventSystem)
    {
        // KeyEvent could be defined and dispatched here if needed
        // m_eventSystem->dispatch<KeyEvent>(key, scancode, action, mods);
    }
}

void InputManager::processMouseButtonInput(GLFWwindow* window, int button, int action, int mods)
{
    // Update mouse button state
    InputState newState = convertGLFWState(action, m_mouseButtonStates[button]);
    m_mouseButtonStates[button] = newState;

    // Dispatch event through event system
    if (m_eventSystem)
    {
        // MouseButtonEvent could be defined and dispatched here if needed
        // m_eventSystem->dispatch<MouseButtonEvent>(button, active, mods);
    }
}

void InputManager::processMouseMoveInput(GLFWwindow* window, double xpos, double ypos)
{
    // Update mouse position
    m_mousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));

    // On first update, initialize last position to current to avoid huge delta
    if (m_lastMousePosition.x == 0 && m_lastMousePosition.y == 0)
    {
        m_lastMousePosition = m_mousePosition;
    }

    // Dispatch event through event system
    if (m_eventSystem)
    {
        // MouseMoveEvent could be defined and dispatched here if needed
        // m_eventSystem->dispatch<MouseMoveEvent>(xpos, ypos);
    }
}

void InputManager::processMouseScrollInput(GLFWwindow* window, double xoffset, double yoffset)
{
    // Update mouse scroll
    m_mouseScrollValue = static_cast<float>(yoffset);

    // Dispatch event through event system
    if (m_eventSystem)
    {
        // MouseScrollEvent could be defined and dispatched here if needed
        // m_eventSystem->dispatch<MouseScrollEvent>(xoffset, yoffset);
    }
}

InputContext* InputManager::createContext(const std::string& contextName)
{
    // Check if context already exists
    auto it = m_contexts.find(contextName);
    if (it != m_contexts.end())
    {
        return it->second.get();
    }

    // Create a new context
    auto context = std::make_unique<InputContext>(contextName);
    InputContext* contextPtr = context.get();

    // Store the context
    m_contexts[contextName] = std::move(context);

    return contextPtr;
}

InputContext* InputManager::getContext(const std::string& contextName)
{
    auto it = m_contexts.find(contextName);
    if (it != m_contexts.end())
    {
        return it->second.get();
    }

    return nullptr;
}

void InputManager::activateContext(const std::string& contextName)
{
    // Check if context exists
    if (m_contexts.find(contextName) == m_contexts.end())
    {
        return;
    }

    // Check if context is already active
    auto it = std::find(m_activeContexts.begin(), m_activeContexts.end(), contextName);
    if (it == m_activeContexts.end())
    {
        m_activeContexts.push_back(contextName);
    }
}

void InputManager::deactivateContext(const std::string& contextName)
{
    // Remove context from active list
    auto it = std::find(m_activeContexts.begin(), m_activeContexts.end(), contextName);
    if (it != m_activeContexts.end())
    {
        m_activeContexts.erase(it);
    }
}

bool InputManager::isActionTriggered(const std::string& actionName) const
{
    // Check if action was just pressed this frame
    for (const auto& contextName : m_activeContexts)
    {
        auto contextIt = m_contexts.find(contextName);
        if (contextIt == m_contexts.end())
        {
            continue;
        }

        const InputContext* context = contextIt->second.get();
        auto actionIt = context->m_actionBindings.find(actionName);

        if (actionIt != context->m_actionBindings.find(actionName))
        {
            for (const auto& binding : actionIt->second)
            {
                if (binding.device == InputDevice::KEYBOARD)
                {
                    // Check key state
                    auto keyIt = m_keyStates.find(binding.primaryCode);
                    if (keyIt != m_keyStates.end() && keyIt->second == InputState::PRESSED)
                    {
                        // Check modifier if needed
                        if (binding.modifierCode != -1)
                        {
                            auto modIt = m_keyStates.find(binding.modifierCode);
                            if (modIt != m_keyStates.end() &&
                                (modIt->second == InputState::PRESSED || modIt->second == InputState::HELD))
                            {
                                return true;
                            }
                        }
                        else
                        {
                            return true;
                        }
                    }
                }
                else if (binding.device == InputDevice::MOUSE)
                {
                    // Check mouse button state
                    auto buttonIt = m_mouseButtonStates.find(binding.primaryCode);
                    if (buttonIt != m_mouseButtonStates.end() && buttonIt->second == InputState::PRESSED)
                    {
                        // Check modifier if needed
                        if (binding.modifierCode != -1)
                        {
                            auto modIt = m_keyStates.find(binding.modifierCode);
                            if (modIt != m_keyStates.end() &&
                                (modIt->second == InputState::PRESSED || modIt->second == InputState::HELD))
                            {
                                return true;
                            }
                        }
                        else
                        {
                            return true;
                        }
                    }
                }
                else if (binding.device == InputDevice::GAMEPAD)
                {
                    // Check each connected gamepad
                    for (int i = 0; i < GLFW_JOYSTICK_LAST; i++)
                    {
                        if (isGamepadConnected(i))
                        {
                            auto gamepadIt = m_gamepadButtonStates.find(i);
                            if (gamepadIt != m_gamepadButtonStates.end())
                            {
                                auto buttonIt = gamepadIt->second.find(binding.primaryCode);
                                if (buttonIt != gamepadIt->second.end() && buttonIt->second == InputState::PRESSED)
                                {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool InputManager::isActionActive(const std::string& actionName) const
{
    // Check if action is active (pressed or held)
    for (const auto& contextName : m_activeContexts)
    {
        auto contextIt = m_contexts.find(contextName);
        if (contextIt == m_contexts.end())
        {
            continue;
        }

        const InputContext* context = contextIt->second.get();
        auto actionIt = context->m_actionBindings.find(actionName);

        if (actionIt != context->m_actionBindings.end())
        {
            for (const auto& binding : actionIt->second)
            {
                if (isBindingActive(binding))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool InputManager::wasActionReleased(const std::string& actionName) const
{
    // Check if action was just released this frame
    for (const auto& contextName : m_activeContexts)
    {
        auto contextIt = m_contexts.find(contextName);
        if (contextIt == m_contexts.end())
        {
            continue;
        }

        const InputContext* context = contextIt->second.get();
        auto actionIt = context->m_actionBindings.find(actionName);

        if (actionIt != context->m_actionBindings.end())
        {
            for (const auto& binding : actionIt->second)
            {
                if (binding.device == InputDevice::KEYBOARD)
                {
                    // Check key state
                    auto keyIt = m_keyStates.find(binding.primaryCode);
                    if (keyIt != m_keyStates.end() && keyIt->second == InputState::RELEASED_THIS_FRAME)
                    {
                        // Check modifier if needed
                        if (binding.modifierCode != -1)
                        {
                            auto modIt = m_keyStates.find(binding.modifierCode);
                            if (modIt != m_keyStates.end() &&
                                (modIt->second == InputState::PRESSED || modIt->second == InputState::HELD))
                            {
                                return true;
                            }
                        }
                        else
                        {
                            return true;
                        }
                    }
                }
                else if (binding.device == InputDevice::MOUSE)
                {
                    // Check mouse button state
                    auto buttonIt = m_mouseButtonStates.find(binding.primaryCode);
                    if (buttonIt != m_mouseButtonStates.end() && buttonIt->second == InputState::RELEASED_THIS_FRAME)
                    {
                        // Check modifier if needed
                        if (binding.modifierCode != -1)
                        {
                            auto modIt = m_keyStates.find(binding.modifierCode);
                            if (modIt != m_keyStates.end() &&
                                (modIt->second == InputState::PRESSED || modIt->second == InputState::HELD))
                            {
                                return true;
                            }
                        }
                        else
                        {
                            return true;
                        }
                    }
                }
                else if (binding.device == InputDevice::GAMEPAD)
                {
                    // Check each connected gamepad
                    for (int i = 0; i < GLFW_JOYSTICK_LAST; i++)
                    {
                        if (isGamepadConnected(i))
                        {
                            auto gamepadIt = m_gamepadButtonStates.find(i);
                            if (gamepadIt != m_gamepadButtonStates.end())
                            {
                                auto buttonIt = gamepadIt->second.find(binding.primaryCode);
                                if (buttonIt != gamepadIt->second.end() &&
                                    buttonIt->second == InputState::RELEASED_THIS_FRAME)
                                {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}

float InputManager::getAxisValue(const std::string& axisName) const
{
    // Check if we have a cached value
    auto cacheIt = m_axisValues.find(axisName);
    if (cacheIt != m_axisValues.end())
    {
        return cacheIt->second;
    }

    // Not in cache, calculate it
    for (const auto& contextName : m_activeContexts)
    {
        auto contextIt = m_contexts.find(contextName);
        if (contextIt == m_contexts.end())
        {
            continue;
        }

        const InputContext* context = contextIt->second.get();
        auto axisIt = context->m_axisBindings.find(axisName);

        if (axisIt != context->m_axisBindings.end())
        {
            const auto& axisBinding = axisIt->second;

            // Get positive and negative values
            float positiveValue = getBindingValue(axisBinding.positiveBinding);
            float negativeValue = getBindingValue(axisBinding.negativeBinding);

            // Apply deadzone
            if (std::abs(positiveValue) < axisBinding.deadzone)
            {
                positiveValue = 0.0f;
            }

            if (std::abs(negativeValue) < axisBinding.deadzone)
            {
                negativeValue = 0.0f;
            }

            // Combine values (positive - negative)
            float value = positiveValue - negativeValue;

            // Clamp to [-1, 1]
            value = std::max(-1.0f, std::min(1.0f, value));

            // Cache the value
            m_axisValues[axisName] = value;

            return value;
        }
    }

    // Axis not found in any active context
    return 0.0f;
}

float InputManager::getAxisValue(InputAxis axis) const
{
    // Find the axis name mapping in active contexts
    for (const auto& contextName : m_activeContexts)
    {
        auto contextIt = m_contexts.find(contextName);
        if (contextIt == m_contexts.end())
        {
            continue;
        }

        const InputContext* context = contextIt->second.get();
        auto axisIt = context->m_predefinedAxisMap.find(axis);

        if (axisIt != context->m_predefinedAxisMap.end())
        {
            const std::string& axisName = axisIt->second;
            return getAxisValue(axisName);
        }
    }

    // Get the sensitivity for the mouse
    float mouseSensitivity = 1.0f;
    auto mouseIt = m_sensitivity.find(InputDevice::MOUSE);
    if (mouseIt != m_sensitivity.end())
    {
        mouseSensitivity = mouseIt->second;
    }

    // Fallback for common axes when no mapping exists
    switch (axis)
    {
        case InputAxis::HORIZONTAL:
            // Try keyboard keys
        {
            float value = 0.0f;
            if (isKeyPressed(GLFW_KEY_D) || isKeyPressed(GLFW_KEY_RIGHT)) value += 1.0f;
            if (isKeyPressed(GLFW_KEY_A) || isKeyPressed(GLFW_KEY_LEFT)) value -= 1.0f;
            return value;
        }
        case InputAxis::VERTICAL:
            // Try keyboard keys
        {
            float value = 0.0f;
            if (isKeyPressed(GLFW_KEY_W) || isKeyPressed(GLFW_KEY_UP)) value += 1.0f;
            if (isKeyPressed(GLFW_KEY_S) || isKeyPressed(GLFW_KEY_DOWN)) value -= 1.0f;
            return value;
        }
        case InputAxis::LOOK_X:
            return m_mouseDelta.x * mouseSensitivity;
        case InputAxis::LOOK_Y:
            return m_mouseDelta.y * mouseSensitivity;
        case InputAxis::ZOOM:
            return m_mouseScrollValue;
        default:
            return 0.0f;
    }
}

glm::vec2 InputManager::getMousePosition() const
{
    return m_mousePosition;
}

glm::vec2 InputManager::getMouseDelta() const
{
    float mouseSensitivity = 1.0f;
    auto it = m_sensitivity.find(InputDevice::MOUSE);
    if (it != m_sensitivity.end())
    {
        mouseSensitivity = it->second;
    }

    return m_mouseDelta * mouseSensitivity;
}

float InputManager::getMouseScrollDelta() const
{
    return m_mouseScrollValue;
}

bool InputManager::isKeyPressed(int key) const
{
    auto it = m_keyStates.find(key);
    return it != m_keyStates.end() && (it->second == InputState::PRESSED || it->second == InputState::HELD);
}

bool InputManager::isMouseButtonPressed(int button) const
{
    auto it = m_mouseButtonStates.find(button);
    return it != m_mouseButtonStates.end() && (it->second == InputState::PRESSED || it->second == InputState::HELD);
}

bool InputManager::isGamepadButtonPressed(int gamepadIndex, int button) const
{
    auto gamepadIt = m_gamepadButtonStates.find(gamepadIndex);
    if (gamepadIt != m_gamepadButtonStates.end())
    {
        auto buttonIt = gamepadIt->second.find(button);
        return buttonIt != gamepadIt->second.end() &&
            (buttonIt->second == InputState::PRESSED || buttonIt->second == InputState::HELD);
    }
    return false;
}

float InputManager::getGamepadAxisValue(int gamepadIndex, int axis) const
{
    // Check if the gamepad is connected
    if (!isGamepadConnected(gamepadIndex))
    {
        return 0.0f;
    }

    // Get the axes values
    int axesCount;
    const float* axes = glfwGetJoystickAxes(gamepadIndex, &axesCount);

    float gamepadSensitivity = 1.0f;
    auto it = m_sensitivity.find(InputDevice::MOUSE);
    if (it != m_sensitivity.end())
    {
        gamepadSensitivity = it->second;
    }

    if (axes && axes[axis])
    {
        float value = axes[axis];

        // Apply deadzone of 0.1 to avoid drift
        if (std::abs(value) < 0.1f)
        {
            return 0.0f;
        }

        // Apply sensitivity
        value *= gamepadSensitivity;

        return value;
    }

    return 0.0f;
}

bool InputManager::isGamepadConnected(int gamepadIndex) const
{
    return glfwJoystickPresent(gamepadIndex) == GLFW_TRUE &&
        glfwJoystickIsGamepad(gamepadIndex) == GLFW_TRUE;
}

std::string InputManager::getGamepadName(int gamepadIndex) const
{
    if (isGamepadConnected(gamepadIndex))
    {
        const char* name = glfwGetGamepadName(gamepadIndex);
        return name ? std::string(name) : "Unknown Gamepad";
    }
    return "";
}

void InputManager::setCursorMode(int mode)
{
    GLFWwindow* window = m_application->getWindow();
    if (window)
    {
        glfwSetInputMode(window, GLFW_CURSOR, mode);
    }
}

int InputManager::getCursorMode() const
{
    GLFWwindow* window = m_application->getWindow();
    if (window)
    {
        return glfwGetInputMode(window, GLFW_CURSOR);
    }
    return GLFW_CURSOR_NORMAL;
}

bool InputManager::saveBindings(const std::string& filename) const
{
    try
    {
        // Create RapidJSON document
        rapidjson::Document document;
        document.SetObject();

        // Create allocator for document
        rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

        // Save all contexts
        rapidjson::Value contextsObject(rapidjson::kObjectType);

        for (const auto& contextPair : m_contexts)
        {
            const std::string& contextName = contextPair.first;
            const InputContext* context = contextPair.second.get();

            rapidjson::Value contextObject(rapidjson::kObjectType);

            // Save action bindings
            rapidjson::Value actionsObject(rapidjson::kObjectType);

            for (const auto& actionPair : context->m_actionBindings)
            {
                const std::string& actionName = actionPair.first;
                const std::vector<InputBinding>& bindings = actionPair.second;

                rapidjson::Value bindingsArray(rapidjson::kArrayType);

                for (const auto& binding : bindings)
                {
                    rapidjson::Value bindingObject(rapidjson::kObjectType);

                    bindingObject.AddMember("device", static_cast<int>(binding.device), allocator);
                    bindingObject.AddMember("primaryCode", binding.primaryCode, allocator);
                    bindingObject.AddMember("modifierCode", binding.modifierCode, allocator);
                    bindingObject.AddMember("scale", binding.scale, allocator);

                    bindingsArray.PushBack(bindingObject, allocator);
                }

                rapidjson::Value actionNameValue;
                actionNameValue.SetString(actionName.c_str(), actionName.length(), allocator);

                actionsObject.AddMember(actionNameValue, bindingsArray, allocator);
            }

            contextObject.AddMember("actions", actionsObject, allocator);

            // Save axis bindings
            rapidjson::Value axesObject(rapidjson::kObjectType);

            for (const auto& axisPair : context->m_axisBindings)
            {
                const std::string& axisName = axisPair.first;
                const InputContext::AxisBinding& axisBinding = axisPair.second;

                rapidjson::Value axisObject(rapidjson::kObjectType);

                // Positive binding
                rapidjson::Value positiveObject(rapidjson::kObjectType);
                positiveObject.AddMember("device", static_cast<int>(axisBinding.positiveBinding.device), allocator);
                positiveObject.AddMember("primaryCode", axisBinding.positiveBinding.primaryCode, allocator);
                positiveObject.AddMember("modifierCode", axisBinding.positiveBinding.modifierCode, allocator);
                positiveObject.AddMember("scale", axisBinding.positiveBinding.scale, allocator);

                axisObject.AddMember("positive", positiveObject, allocator);

                // Negative binding
                rapidjson::Value negativeObject(rapidjson::kObjectType);
                negativeObject.AddMember("device", static_cast<int>(axisBinding.negativeBinding.device), allocator);
                negativeObject.AddMember("primaryCode", axisBinding.negativeBinding.primaryCode, allocator);
                negativeObject.AddMember("modifierCode", axisBinding.negativeBinding.modifierCode, allocator);
                negativeObject.AddMember("scale", axisBinding.negativeBinding.scale, allocator);

                axisObject.AddMember("negative", negativeObject, allocator);

                // Deadzone
                axisObject.AddMember("deadzone", axisBinding.deadzone, allocator);

                // Add axis to axes object
                rapidjson::Value axisNameValue;
                axisNameValue.SetString(axisName.c_str(), axisName.length(), allocator);

                axesObject.AddMember(axisNameValue, axisObject, allocator);
            }

            contextObject.AddMember("axes", axesObject, allocator);

            // Save predefined axis mappings
            rapidjson::Value predefinedObject(rapidjson::kObjectType);

            for (const auto& predefinedPair : context->m_predefinedAxisMap)
            {
                InputAxis axis = predefinedPair.first;
                const std::string& axisName = predefinedPair.second;

                std::string axisIndexStr = std::to_string(static_cast<int>(axis));
                rapidjson::Value axisIndexValue;
                axisIndexValue.SetString(axisIndexStr.c_str(), axisIndexStr.length(), allocator);

                rapidjson::Value axisNameValue;
                axisNameValue.SetString(axisName.c_str(), axisName.length(), allocator);

                predefinedObject.AddMember(axisIndexValue, axisNameValue, allocator);
            }

            contextObject.AddMember("predefined", predefinedObject, allocator);

            // Add context to contexts object
            rapidjson::Value contextNameValue;
            contextNameValue.SetString(contextName.c_str(), contextName.length(), allocator);

            contextsObject.AddMember(contextNameValue, contextObject, allocator);
        }

        document.AddMember("contexts", contextsObject, allocator);

        // Save active contexts
        rapidjson::Value activeContextsArray(rapidjson::kArrayType);

        for (const auto& contextName : m_activeContexts)
        {
            rapidjson::Value contextNameValue;
            contextNameValue.SetString(contextName.c_str(), contextName.length(), allocator);

            activeContextsArray.PushBack(contextNameValue, allocator);
        }

        document.AddMember("activeContexts", activeContextsArray, allocator);

        // Save sensitivity settings
        rapidjson::Value sensitivityObject(rapidjson::kObjectType);

        for (const auto& sensitivityPair : m_sensitivity)
        {
            InputDevice device = sensitivityPair.first;
            float value = sensitivityPair.second;

            std::string deviceIndexStr = std::to_string(static_cast<int>(device));
            rapidjson::Value deviceIndexValue;
            deviceIndexValue.SetString(deviceIndexStr.c_str(), deviceIndexStr.length(), allocator);

            sensitivityObject.AddMember(deviceIndexValue, value, allocator);
        }

        document.AddMember("sensitivity", sensitivityObject, allocator);

        // Write to file
        std::ofstream file(filename);
        if (!file.is_open())
        {
            return false;
        }

        rapidjson::OStreamWrapper osw(file);
        rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
        document.Accept(writer);

        file.close();

        return true;
    }
    catch (const std::exception& e)
    {
        // Log error
        return false;
    }
}

bool InputManager::loadBindings(const std::string& filename)
{
    try
    {
        // Read the file
        std::ifstream file(filename);
        if (!file.is_open())
        {
            return false;
        }

        // Parse JSON document
        rapidjson::IStreamWrapper isw(file);
        rapidjson::Document document;
        document.ParseStream(isw);
        file.close();

        if (document.HasParseError())
        {
            return false;
        }

        // Clear existing contexts and create new ones from file
        m_contexts.clear();
        m_activeContexts.clear();

        // Load contexts
        if (document.HasMember("contexts") && document["contexts"].IsObject())
        {
            const rapidjson::Value& contextsObject = document["contexts"];

            for (rapidjson::Value::ConstMemberIterator contextIt = contextsObject.MemberBegin();
                 contextIt != contextsObject.MemberEnd(); contextIt++)
            {
                // Get context name
                const std::string contextName = contextIt->name.GetString();
                const rapidjson::Value& contextObject = contextIt->value;

                // Create the context
                InputContext* context = createContext(contextName);

                // Load action bindings
                if (contextObject.HasMember("actions") && contextObject["actions"].IsObject())
                {
                    const rapidjson::Value& actionsObject = contextObject["actions"];

                    for (rapidjson::Value::ConstMemberIterator actionIt = actionsObject.MemberBegin();
                         actionIt != actionsObject.MemberEnd(); actionIt++)
                    {
                        // Get action name
                        const std::string actionName = actionIt->name.GetString();
                        const rapidjson::Value& bindingsArray = actionIt->value;

                        if (bindingsArray.IsArray())
                        {
                            for (rapidjson::SizeType i = 0; i < bindingsArray.Size(); i++)
                            {
                                const rapidjson::Value& bindingObject = bindingsArray[i];

                                InputBinding binding;
                                binding.device = static_cast<InputDevice>(bindingObject["device"].GetInt());
                                binding.primaryCode = bindingObject["primaryCode"].GetInt();
                                binding.modifierCode = bindingObject["modifierCode"].GetInt();
                                binding.scale = bindingObject["scale"].GetFloat();

                                context->addActionBinding(actionName, binding);
                            }
                        }
                    }
                }

                // Load axis bindings
                if (contextObject.HasMember("axes") && contextObject["axes"].IsObject())
                {
                    const rapidjson::Value& axesObject = contextObject["axes"];

                    for (rapidjson::Value::ConstMemberIterator axisIt = axesObject.MemberBegin();
                         axisIt != axesObject.MemberEnd(); axisIt++)
                    {
                        // Get axis name
                        const std::string axisName = axisIt->name.GetString();
                        const rapidjson::Value& axisObject = axisIt->value;

                        if (axisObject.HasMember("positive") && axisObject.HasMember("negative"))
                        {
                            // Load positive binding
                            InputBinding positiveBinding;
                            const rapidjson::Value& positiveObject = axisObject["positive"];
                            positiveBinding.device = static_cast<InputDevice>(positiveObject["device"].GetInt());
                            positiveBinding.primaryCode = positiveObject["primaryCode"].GetInt();
                            positiveBinding.modifierCode = positiveObject["modifierCode"].GetInt();
                            positiveBinding.scale = positiveObject["scale"].GetFloat();

                            // Load negative binding
                            InputBinding negativeBinding;
                            const rapidjson::Value& negativeObject = axisObject["negative"];
                            negativeBinding.device = static_cast<InputDevice>(negativeObject["device"].GetInt());
                            negativeBinding.primaryCode = negativeObject["primaryCode"].GetInt();
                            negativeBinding.modifierCode = negativeObject["modifierCode"].GetInt();
                            negativeBinding.scale = negativeObject["scale"].GetFloat();

                            // Get deadzone
                            float deadzone = axisObject["deadzone"].GetFloat();

                            context->addAxisBinding(axisName, positiveBinding, negativeBinding, deadzone);
                        }
                    }
                }

                // Load predefined axis mappings
                if (contextObject.HasMember("predefined") && contextObject["predefined"].IsObject())
                {
                    const rapidjson::Value& predefinedObject = contextObject["predefined"];

                    for (rapidjson::Value::ConstMemberIterator predefinedIt = predefinedObject.MemberBegin();
                         predefinedIt != predefinedObject.MemberEnd(); predefinedIt++)
                    {
                        // Get axis index and name
                        int axisIndex = std::stoi(predefinedIt->name.GetString());
                        const std::string axisName = predefinedIt->value.GetString();

                        InputAxis axis = static_cast<InputAxis>(axisIndex);
                        context->m_predefinedAxisMap[axis] = axisName;
                    }
                }
            }
        }

        // Load active contexts
        if (document.HasMember("activeContexts") && document["activeContexts"].IsArray())
        {
            const rapidjson::Value& activeContextsArray = document["activeContexts"];

            for (rapidjson::SizeType i = 0; i < activeContextsArray.Size(); i++)
            {
                if (activeContextsArray[i].IsString())
                {
                    m_activeContexts.push_back(activeContextsArray[i].GetString());
                }
            }
        }

        // Load sensitivity settings
        if (document.HasMember("sensitivity") && document["sensitivity"].IsObject())
        {
            const rapidjson::Value& sensitivityObject = document["sensitivity"];

            for (rapidjson::Value::ConstMemberIterator sensitivityIt = sensitivityObject.MemberBegin();
                 sensitivityIt != sensitivityObject.MemberEnd(); sensitivityIt++)
            {
                // Get device type and sensitivity value
                int deviceIndex = std::stoi(sensitivityIt->name.GetString());
                float value = sensitivityIt->value.GetFloat();

                InputDevice device = static_cast<InputDevice>(deviceIndex);
                m_sensitivity[device] = value;
            }
        }

        return true;
    }
    catch (const std::exception& e)
    {
        // Log error
        return false;
    }
}

void InputManager::setCamera(Camera* camera)
{
    m_camera = camera;
}

InputState InputManager::getKeyState(int key) const
{
    auto it = m_keyStates.find(key);
    if (it != m_keyStates.end())
    {
        return it->second;
    }
    return InputState::RELEASED;
}

InputState InputManager::getMouseButtonState(int button) const
{
    auto it = m_mouseButtonStates.find(button);
    if (it != m_mouseButtonStates.end())
    {
        return it->second;
    }
    return InputState::RELEASED;
}

InputState InputManager::getGamepadButtonState(int gamepadIndex, int button) const
{
    auto gamepadIt = m_gamepadButtonStates.find(gamepadIndex);
    if (gamepadIt != m_gamepadButtonStates.end())
    {
        auto buttonIt = gamepadIt->second.find(button);
        if (buttonIt != gamepadIt->second.end())
        {
            return buttonIt->second;
        }
    }
    return InputState::RELEASED;
}

void InputManager::setSensitivity(InputDevice deviceType, float sensitivity)
{
    m_sensitivity[deviceType] = sensitivity;
}

float InputManager::getSensitivity(InputDevice deviceType) const
{
    auto it = m_sensitivity.find(deviceType);
    if (it != m_sensitivity.end())
    {
        return it->second;
    }
    return 1.0f;
}

void InputManager::createDefaultContexts()
{
    // Create a default gameplay context
    InputContext* gameplay = createContext("Gameplay");

    // Map common keyboard actions
    gameplay->addActionBinding("Jump", InputBinding(InputDevice::KEYBOARD, GLFW_KEY_SPACE));
    gameplay->addActionBinding("Attack", InputBinding(InputDevice::MOUSE, GLFW_MOUSE_BUTTON_LEFT));
    gameplay->addActionBinding("Interact", InputBinding(InputDevice::KEYBOARD, GLFW_KEY_E));
    gameplay->addActionBinding("Pause", InputBinding(InputDevice::KEYBOARD, GLFW_KEY_ESCAPE));

    // Map common axes
    gameplay->mapAxis(InputAxis::HORIZONTAL,
                      InputBinding(InputDevice::KEYBOARD, GLFW_KEY_D),
                      InputBinding(InputDevice::KEYBOARD, GLFW_KEY_A));

    gameplay->mapAxis(InputAxis::VERTICAL,
                      InputBinding(InputDevice::KEYBOARD, GLFW_KEY_W),
                      InputBinding(InputDevice::KEYBOARD, GLFW_KEY_S));

    gameplay->mapAxis(InputAxis::LOOK_X,
                      InputBinding(InputDevice::MOUSE, 0, -1, 0.01f), // Using special code for mouse movement
                      InputBinding(InputDevice::MOUSE, 0, -1, -0.01f));

    gameplay->mapAxis(InputAxis::LOOK_Y,
                      InputBinding(InputDevice::MOUSE, 1, -1, 0.01f), // Using special code for mouse movement
                      InputBinding(InputDevice::MOUSE, 1, -1, -0.01f));

    gameplay->mapAxis(InputAxis::ZOOM,
                      InputBinding(InputDevice::MOUSE, 2, -1, 0.1f), // Using special code for mouse wheel
                      InputBinding(InputDevice::MOUSE, 2, -1, -0.1f));

    // Map gamepad controls if available
    gameplay->addActionBinding("Jump", InputBinding(InputDevice::GAMEPAD, GLFW_GAMEPAD_BUTTON_A));
    gameplay->addActionBinding("Attack", InputBinding(InputDevice::GAMEPAD, GLFW_GAMEPAD_BUTTON_X));
    gameplay->addActionBinding("Interact", InputBinding(InputDevice::GAMEPAD, GLFW_GAMEPAD_BUTTON_B));
    gameplay->addActionBinding("Pause", InputBinding(InputDevice::GAMEPAD, GLFW_GAMEPAD_BUTTON_START));

    // Create an editor context
    InputContext* editor = createContext("Editor");

    // Map common editor actions
    editor->addActionBinding("Undo", InputBinding(InputDevice::KEYBOARD, GLFW_KEY_Z, GLFW_KEY_LEFT_CONTROL));
    editor->addActionBinding("Redo", InputBinding(InputDevice::KEYBOARD, GLFW_KEY_Y, GLFW_KEY_LEFT_CONTROL));
    editor->addActionBinding("Save", InputBinding(InputDevice::KEYBOARD, GLFW_KEY_S, GLFW_KEY_LEFT_CONTROL));
    editor->addActionBinding("Delete", InputBinding(InputDevice::KEYBOARD, GLFW_KEY_DELETE));
    editor->addActionBinding("Copy", InputBinding(InputDevice::KEYBOARD, GLFW_KEY_C, GLFW_KEY_LEFT_CONTROL));
    editor->addActionBinding("Paste", InputBinding(InputDevice::KEYBOARD, GLFW_KEY_V, GLFW_KEY_LEFT_CONTROL));
    editor->addActionBinding("Cut", InputBinding(InputDevice::KEYBOARD, GLFW_KEY_X, GLFW_KEY_LEFT_CONTROL));
    editor->addActionBinding("SelectAll", InputBinding(InputDevice::KEYBOARD, GLFW_KEY_A, GLFW_KEY_LEFT_CONTROL));

    // Map camera movement in editor
    editor->mapAxis(InputAxis::HORIZONTAL,
                    InputBinding(InputDevice::KEYBOARD, GLFW_KEY_D),
                    InputBinding(InputDevice::KEYBOARD, GLFW_KEY_A));

    editor->mapAxis(InputAxis::VERTICAL,
                    InputBinding(InputDevice::KEYBOARD, GLFW_KEY_W),
                    InputBinding(InputDevice::KEYBOARD, GLFW_KEY_S));

    editor->mapAxis(InputAxis::LOOK_X,
                    InputBinding(InputDevice::MOUSE, 0, -1, 0.005f),
                    InputBinding(InputDevice::MOUSE, 0, -1, -0.005f));

    editor->mapAxis(InputAxis::LOOK_Y,
                    InputBinding(InputDevice::MOUSE, 1, -1, 0.005f),
                    InputBinding(InputDevice::MOUSE, 1, -1, -0.005f));

    editor->mapAxis(InputAxis::ZOOM,
                    InputBinding(InputDevice::MOUSE, 2, -1, 0.1f),
                    InputBinding(InputDevice::MOUSE, 2, -1, -0.1f));

    // Create a UI context
    InputContext* ui = createContext("UI");

    // Map common UI actions
    ui->addActionBinding("Confirm", InputBinding(InputDevice::KEYBOARD, GLFW_KEY_ENTER));
    ui->addActionBinding("Cancel", InputBinding(InputDevice::KEYBOARD, GLFW_KEY_ESCAPE));
    ui->addActionBinding("Click", InputBinding(InputDevice::MOUSE, GLFW_MOUSE_BUTTON_LEFT));
    ui->addActionBinding("RightClick", InputBinding(InputDevice::MOUSE, GLFW_MOUSE_BUTTON_RIGHT));

    // UI navigation
    ui->mapAxis(InputAxis::HORIZONTAL,
                InputBinding(InputDevice::KEYBOARD, GLFW_KEY_RIGHT),
                InputBinding(InputDevice::KEYBOARD, GLFW_KEY_LEFT));

    ui->mapAxis(InputAxis::VERTICAL,
                InputBinding(InputDevice::KEYBOARD, GLFW_KEY_DOWN),
                InputBinding(InputDevice::KEYBOARD, GLFW_KEY_UP));

    // Activate the gameplay context by default
    activateContext("Gameplay");
}

int InputManager::registerActionCallback(const std::string& actionName, std::function<void()> callback)
{
    CallbackData data;
    data.id = m_nextCallbackId++;
    data.actionCallback = callback;

    m_actionCallbacks[actionName].push_back(data);

    return data.id;
}

int InputManager::registerAxisCallback(const std::string& axisName, std::function<void(float)> callback)
{
    CallbackData data;
    data.id = m_nextCallbackId++;
    data.axisCallback = callback;

    m_axisCallbacks[axisName].push_back(data);

    return data.id;
}

void InputManager::unregisterActionCallback(const std::string& actionName, int callbackId)
{
    auto it = m_actionCallbacks.find(actionName);
    if (it != m_actionCallbacks.end())
    {
        auto& callbacks = it->second;
        callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(),
                                       [callbackId](const CallbackData& data) { return data.id == callbackId; }),
                        callbacks.end());
    }
}

void InputManager::unregisterAxisCallback(const std::string& axisName, int callbackId)
{
    auto it = m_axisCallbacks.find(axisName);
    if (it != m_axisCallbacks.end())
    {
        auto& callbacks = it->second;
        callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(),
                                       [callbackId](const CallbackData& data) { return data.id == callbackId; }),
                        callbacks.end());
    }
}

// Implementation of internal methods
void InputManager::updateActionStates()
{
    // No need to update action states directly - they are calculated on demand
    // from the input states by the isAction* methods
}

void InputManager::updateAxisValues()
{
    // Clear the axis cache
    m_axisValues.clear();

    // Axes are calculated on demand by getAxisValue methods
}

bool InputManager::isBindingActive(const InputBinding& binding) const
{
    if (binding.device == InputDevice::KEYBOARD)
    {
        // Check key state
        auto keyIt = m_keyStates.find(binding.primaryCode);
        if (keyIt != m_keyStates.end() &&
            (keyIt->second == InputState::PRESSED || keyIt->second == InputState::HELD))
        {
            // Check modifier if needed
            if (binding.modifierCode != -1)
            {
                auto modIt = m_keyStates.find(binding.modifierCode);
                if (modIt != m_keyStates.end() &&
                    (modIt->second == InputState::PRESSED || modIt->second == InputState::HELD))
                {
                    return true;
                }
            }
            else
            {
                return true;
            }
        }
    }
    else if (binding.device == InputDevice::MOUSE)
    {
        // Check mouse button state
        auto buttonIt = m_mouseButtonStates.find(binding.primaryCode);
        if (buttonIt != m_mouseButtonStates.end() &&
            (buttonIt->second == InputState::PRESSED || buttonIt->second == InputState::HELD))
        {
            // Check modifier if needed
            if (binding.modifierCode != -1)
            {
                auto modIt = m_keyStates.find(binding.modifierCode);
                if (modIt != m_keyStates.end() &&
                    (modIt->second == InputState::PRESSED || modIt->second == InputState::HELD))
                {
                    return true;
                }
            }
            else
            {
                return true;
            }
        }
    }
    else if (binding.device == InputDevice::GAMEPAD)
    {
        // Check each connected gamepad
        for (int i = 0; i < GLFW_JOYSTICK_LAST; i++)
        {
            if (isGamepadConnected(i))
            {
                auto gamepadIt = m_gamepadButtonStates.find(i);
                if (gamepadIt != m_gamepadButtonStates.end())
                {
                    auto buttonIt = gamepadIt->second.find(binding.primaryCode);
                    if (buttonIt != gamepadIt->second.end() &&
                        (buttonIt->second == InputState::PRESSED || buttonIt->second == InputState::HELD))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

float InputManager::getBindingValue(const InputBinding& binding) const
{
    if (binding.device == InputDevice::KEYBOARD)
    {
        // Keyboard bindings are digital (0 or 1)
        if (isBindingActive(binding))
        {
            return binding.scale;
        }
    }
    else if (binding.device == InputDevice::MOUSE)
    {
        // Special case for mouse movement and scroll
        if (binding.primaryCode == 0) // Mouse X movement
        {
            return m_mouseDelta.x * binding.scale;
        }
        else if (binding.primaryCode == 1) // Mouse Y movement
        {
            return m_mouseDelta.y * binding.scale;
        }
        else if (binding.primaryCode == 2) // Mouse scroll
        {
            return m_mouseScrollValue * binding.scale;
        }
        else if (isBindingActive(binding)) // Mouse buttons
        {
            return binding.scale;
        }
    }
    else if (binding.device == InputDevice::GAMEPAD)
    {
        // Check if it's a gamepad axis
        if (binding.primaryCode >= GLFW_GAMEPAD_AXIS_LEFT_X && binding.primaryCode <= GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER)
        {
            // Find first connected gamepad
            for (int i = 0; i < GLFW_JOYSTICK_LAST; i++)
            {
                if (isGamepadConnected(i))
                {
                    int axis = binding.primaryCode - GLFW_GAMEPAD_AXIS_LEFT_X;
                    float value = getGamepadAxisValue(i, axis) * binding.scale;

                    // Apply deadzone if modifier code is used as deadzone value
                    if (binding.modifierCode != -1)
                    {
                        float deadzone = binding.modifierCode / 100.0f; // Convert to percentage
                        if (std::abs(value) < deadzone)
                        {
                            value = 0.0f;
                        }
                    }

                    return value;
                }
            }
        }
        else if (isBindingActive(binding)) // Gamepad buttons
        {
            return binding.scale;
        }
    }

    return 0.0f;
}

void InputManager::updateInputStates()
{
    // Update keyboard states
    for (auto& pair : m_keyStates)
    {
        int key = pair.first;
        InputState& state = pair.second;

        // Transition from PRESSED to HELD
        if (state == InputState::PRESSED)
        {
            state = InputState::HELD;
        }
        // Transition from RELEASED_THIS_FRAME to RELEASED
        else if (state == InputState::RELEASED_THIS_FRAME)
        {
            state = InputState::RELEASED;
        }
    }

    // Update mouse button states
    for (auto& pair : m_mouseButtonStates)
    {
        int button = pair.first;
        InputState& state = pair.second;

        // Transition from PRESSED to HELD
        if (state == InputState::PRESSED)
        {
            state = InputState::HELD;
        }
        // Transition from RELEASED_THIS_FRAME to RELEASED
        else if (state == InputState::RELEASED_THIS_FRAME)
        {
            state = InputState::RELEASED;
        }
    }

    // Update gamepad button states
    for (auto& gamepadPair : m_gamepadButtonStates)
    {
        int gamepadIndex = gamepadPair.first;
        auto& buttonStates = gamepadPair.second;

        for (auto& buttonPair : buttonStates)
        {
            int button = buttonPair.first;
            InputState& state = buttonPair.second;

            // Transition from PRESSED to HELD
            if (state == InputState::PRESSED)
            {
                state = InputState::HELD;
            }
            // Transition from RELEASED_THIS_FRAME to RELEASED
            else if (state == InputState::RELEASED_THIS_FRAME)
            {
                state = InputState::RELEASED;
            }
        }
    }
}

void InputManager::triggerCallbacks()
{
    // Trigger action callbacks
    for (const auto& contextName : m_activeContexts)
    {
        auto contextIt = m_contexts.find(contextName);
        if (contextIt == m_contexts.end())
        {
            continue;
        }

        const InputContext* context = contextIt->second.get();

        // Check each action in the context
        for (const auto& actionPair : context->m_actionBindings)
        {
            const std::string& actionName = actionPair.first;

            // If action is triggered, call the callbacks
            if (isActionTriggered(actionName))
            {
                auto callbackIt = m_actionCallbacks.find(actionName);
                if (callbackIt != m_actionCallbacks.end())
                {
                    const auto& callbacks = callbackIt->second;
                    for (const auto& callback : callbacks)
                    {
                        if (callback.actionCallback)
                        {
                            callback.actionCallback();
                        }
                    }
                }
            }
        }

        // Check each axis in the context
        for (const auto& axisPair : context->m_axisBindings)
        {
            const std::string& axisName = axisPair.first;

            // Get the axis value
            float value = getAxisValue(axisName);

            // Call the callbacks if value is non-zero
            if (value != 0.0f)
            {
                auto callbackIt = m_axisCallbacks.find(axisName);
                if (callbackIt != m_axisCallbacks.end())
                {
                    const auto& callbacks = callbackIt->second;
                    for (const auto& callback : callbacks)
                    {
                        if (callback.axisCallback)
                        {
                            callback.axisCallback(value);
                        }
                    }
                }
            }
        }
    }
}

void InputManager::setupGLFWCallbacks()
{
    GLFWwindow* window = m_application->getWindow();
    if (window)
    {
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorPositionCallback);
        glfwSetScrollCallback(window, scrollCallback);
    }
}

void InputManager::updateGamepadState()
{
    // For each possible gamepad
    for (int gamepadIndex = 0; gamepadIndex < GLFW_JOYSTICK_LAST; gamepadIndex++)
    {
        // Check if gamepad is connected
        if (isGamepadConnected(gamepadIndex))
        {
            // Get gamepad state
            GLFWgamepadstate state;
            if (glfwGetGamepadState(gamepadIndex, &state))
            {
                // Create entry for this gamepad if it doesn't exist
                auto& buttonStates = m_gamepadButtonStates[gamepadIndex]; // Safe to use here as we're modifying

                // Update button states
                for (int button = 0; button < GLFW_GAMEPAD_BUTTON_LAST; button++)
                {
                    int glfwState = state.buttons[button];

                    // Get current state - use find to avoid insertion for reading
                    auto buttonIt = buttonStates.find(button);
                    InputState currentState = (buttonIt != buttonStates.end()) ?
                        buttonIt->second : InputState::RELEASED;

                    // Update state - using [] is ok here as we're assigning a new value
                    buttonStates[button] = convertGLFWState(glfwState, currentState);
                }

                // For axes, we handle them directly in getGamepadAxisValue
            }
        }
    }
}

InputState InputManager::convertGLFWState(int glfwState, InputState currentState) const
{
    if (glfwState == GLFW_PRESS)
    {
        // If already pressed, keep as HELD, otherwise it's a new press
        return (currentState == InputState::HELD) ? InputState::HELD : InputState::PRESSED;
    }
    else if (glfwState == GLFW_RELEASE)
    {
        // If was pressed or held, now it's released this frame
        if (currentState == InputState::PRESSED || currentState == InputState::HELD)
        {
            return InputState::RELEASED_THIS_FRAME;
        }
        // Otherwise, it's still released
        return InputState::RELEASED;
    }

    // For GLFW_REPEAT, we keep the current state
    return currentState;
}