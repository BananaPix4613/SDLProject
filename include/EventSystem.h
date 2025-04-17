#pragma once

#include <unordered_map>
#include <typeindex>
#include <functional>
#include <vector>
#include <mutex>
#include <memory>
#include <queue>
#include <algorithm>

/**
 * @class Event
 * @brief Base class for all events in the system
 * 
 * Events derive from this class to enable type identification and polymorphic handling.
 * Concrete event types can add specific data members relevant to the event.
 */
class Event
{
public:
    virtual ~Event() = default;

    /**
     * @brief Get the type index of this event
     * @return std::type_index representing the event type
     */
    virtual std::type_index getType() const = 0;
    
    /**
     * @brief Check if event has been handled
     * @return True if event has been handled, false otherwise
     */
    bool isHandled() const
    {
        return m_handled;
    }

    /**
     * @brief Mark the event as handled
     * @param handled New handled state
     */
    void setHandled(bool handled = true)
    {
        m_handled = handled;
    }
    
private:
    bool m_handled = false;
};

/**
 * @class EventType
 * @brief Template class to create concrete event types
 * @tparam T The concrete event type
 * 
 * Provides automatic type index implementation for derived event classes.
 */
template<typename T>
class EventType : public Event
{
public:
    /**
     * @brief Get the type index of this event
     * @return std::type_index representing the event type
     */
    std::type_index getType() const override
    {
        return std::type_index(typeid(T));
    }

    /**
     * @brief Static method to get the type index of this event type
     * @return std::type_index representing the event type
     */
    static std::type_index staticType()
    {
        return std::type_index(typeid(T));
    }
};

/**
 * @class EventDispatcher
 * @brief Helper class to dispatch events to specific handler types
 * 
 * Used to easily dispatch events to handlers that handle specific event types.
 */
class EventDispatcher
{
public:
    /**
     * @brief Constructor
     * @param event Reference to event being dispatched
     */
    EventDispatcher(Event& event) : m_event(event)
    {
    }

    /**
     * @brief Dispatch event to specific event type handler
     * @tparam T Specific event type
     * @tparam F Function type that takes T& as parameter
     * @param func Function or lambda to handle the event
     * @return True if event was handled, false otherwise
     */
    template<typename T, typename F>
    bool dispatch(const F& func)
    {
        if (m_event.getType() == std::type_index(typeid(T))
        {
            T& derivedEvent = static_cast<T&>(m_event);
            func(derivedEvent);
            return true;
        }
        return false;
    }

private:
    Event& m_event;
};

// Forward declaration of event handler types
template<typename EventType>
using EventHandler = std::function<void(EventType&)>;

template<typename EventType>
using EventHandlerPtr = std::shared_ptr<EventHandler<EventType>>;

/**
 * @class EventSystem
 * @brief Central event management system for the engine
 * 
 * Provides facilities for event subscription, dispatching, and processing.
 * Supports both immediate (synchronous) and queued (asynchronous) event handling.
 */
class EventSystem
{
public:
    /**
     * @brief Constructor
     */
    EventSystem();

    /**
     * @brief Destructor
     */
    ~EventSystem();

    /**
     * @brief Initialize the event system
     */
    void initialize();

    /**
     * @brief Shut down the event system
     */
    void shutdown();

    /**
     * @brief Update the event system, processing queued events
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime);

    /**
     * @brief Subscribe to an event type with a handler function
     * @tparam EventType Specific event type to subscribe to
     * @param handler Function or lambda to handle the event
     * @return Handler ID that can be used to unsubscribe
     */
    template<typename T>
    size_t subscribe(const EventHandler<T>& handler)
    {
        std::type_index typeIndex = std::type_index(typeid(T));

        std::lock_guard<std::mutex> lock(m_eventMutex);

        auto handlerPtr = std::make_shared<EventHandler<T>>(handler);
        size_t handlerId = m_nextHandlerId++;

        if (m_eventHandlers.find(typeIndex) == m_eventHandlers.end())
        {
            m_eventHandlers[typeIndex] = std::vector<std::pair<size_t, void*>>();
        }

        m_eventHandlers[typeIndex].push_back(
            std::make_pair(handlerId, static_cast<void*>(handlerPtr.get()))
        );

        m_handlers[handlerId] = std::make_pair(typeIndex, handlerPtr);

        return handlerId;
    }

    /**
     * @brief Unsubscribe from an event using handler ID
     * @param handlerId ID returned from subscribe method
     * @return True if successfully unsubscribed, false if handler not found
     */
    bool unsubscribe(size_t handlerId);

    /**
     * @brief Immediately dispatch an event to all registered handlers
     * @tparam T Event type
     * @tparam Args Event constructor argument types
     * @param args Event constructor arguments
     * @return Reference to the dispatched event
     */
    template<typename T, typename... Args>
    T& dispatch(Args&&... args)
    {
        // Create the event
        auto eventPtr = std::make_shared<T>(std::forward<Args>(args)...);
        T& event = *eventPtr;

        std::type_index typeIndex = std::type_index(typeid(T));

        // Process the event immediately
        processEvent(typeIndex, static_cast<Event*>(eventPtr.get()));

        // Store in recent events (for potential later reference)
        {
            std::lock_guard<std::mutex> lock(m_eventMutex);
            m_recentEvents.push_back(std::static_pointer_cast<Event>(eventPtr));
            if (m_recentEvents.size() > m_maxRecentEvents)
            {
                m_recentEvents.pop_front();
            }
        }

        return event;
    }

    /**
     * @brief Queue an event for later processing
     * @tparam T Event type
     * @tparam Args Event constructor argument types
     * @param args Event constructor arguments
     */
    template<typename T, typename... Args>
    void enqueueEvent(Args&&... args)
    {
        auto eventPtr = std::make_shared<T>(std::forward<Args>(args)...);

        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_eventQueue.push(std::static_pointer_cast<Event>(eventPtr));
    }

    /**
     * @brief Process all queued events immediately
     * @param clearQueue Whether to clear any remaining events after processing
     */
    void processQueuedEvents(bool clearQueue = true);

    /**
     * @brief Clear all queued events without processing
     */
    void clearEventQueue();

    /**
     * @brief Check if any handlers are registered for an event type
     * @tparam T Event type
     * @return True if handlers exist for this type, false otherwise
     */
    template<typename T>
    bool hasHandlersForEventType() const
    {
        std::type_index typeIndex = std::type_index(typeid(T));

        std::lock_guard<std::mutex> lock(m_eventMutex);

        auto it = m_eventHandlers.find(typeIndex);
        return (it != m_eventHandlers.end() && !it->second.empty());
    }

    /**
     * @brief Get number of queued events
     * @return Current number of events in the queue
     */
    size_t getQueuedEventCount() const;

    /**
     * @brief Set the maximum number of recent events to keep
     * @param count Maximum number of events
     */
    void setMaxRecentEvents(size_t count);

    /**
     * @brief Get the maximum number of recent events kept
     * @return Maximum number of recent events
     */
    size_t getMaxRecentEvents() const;

private:
    // Handler storage typedefs for clarity
    using HandlerId = size_t;
    using HandlerPtr = void*;
    using TypeHandlerPair = std::pair<std::type_index, std::shared_ptr<void>>;

    // Event handlers by type
    std::unordered_map<std::type_index, std::vector<std::pair<HandlerId, HandlerPtr>>> m_eventHandlers;

    // Handler IDs to actual handlers (for unsubscribing)
    std::unordered_map<HandlerId, TypeHandlerPair> m_handlers;

    // Event queue for asynchronous events
    std::queue<std::shared_ptr<Event>> m_eventQueue;

    // Recent events list (for debugging or handling references)
    std::deque<std::shared_ptr<Event>> m_recentEvents;

    // Thread synchronization
    mutable std::mutex m_eventMutex;

    // Handler management
    HandlerId m_nextHandlerId;

    // Configuration
    size_t m_maxRecentEvents;

    /**
     * @brief Process a single event, dispatching to all relevant handlers
     * @param typeIndex Type index of the event
     * @param event Pointer to the event
     */
    void processEvent(const std::type_index& typeIndex, Event* event);
};