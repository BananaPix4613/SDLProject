#include "EventSystem.h"

EventSystem::EventSystem() :
    m_nextHandlerId(1),
    m_maxRecentEvents(10)
{
}

EventSystem::~EventSystem()
{
    shutdown();
}

void EventSystem::initialize()
{
    // Initialize any event system resources
    m_nextHandlerId = 1;
    clearEventQueue();
    m_recentEvents.clear();

    // Additional initialization if needed
}

void EventSystem::shutdown()
{
    // Clean up event system resources
    std::lock_guard<std::mutex> lock(m_eventMutex);

    clearEventQueue();
    m_eventHandlers.clear();
    m_handlers.clear();
    m_recentEvents.clear();
}

void EventSystem::update(float deltaTime)
{
    // Process all queued events
    processQueuedEvents();
}

bool EventSystem::unsubscribe(size_t handlerId)
{
    std::lock_guard<std::mutex> lock(m_eventMutex);

    auto handlerIt = m_handlers.find(handlerId);
    if (handlerIt == m_handlers.end())
    {
        return false; // Handler ID not found
    }

    std::type_index typeIndex = handlerIt->second.first;

    // Find and remove the handler from event handlers list
    auto& handlers = m_eventHandlers[typeIndex];
    auto it = std::find_if(handlers.begin(), handlers.end(),
                           [handlerId](const std::pair<size_t, void*>& pair) {
                               return pair.first == handlerId;
                           });

    if (it != handlers.end())
    {
        handlers.erase(it);
    }

    // Remove from handlers map
    m_handlers.erase(handlerId);

    return true;
}

void EventSystem::processQueuedEvents(bool clearQueue)
{
    std::queue<std::shared_ptr<Event>> processQueue;

    // Move events to local queue to minimize lock time
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        std::swap(processQueue, m_eventQueue);
    }

    // Process events
    while (!processQueue.empty())
    {
        auto event = processQueue.front();
        processQueue.pop();

        processEvent(event->getType(), event.get());
    }

    // Clear remaining events if requested
    if (clearQueue && !m_eventQueue.empty())
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        std::queue<std::shared_ptr<Event>> empty;
        std::swap(m_eventQueue, empty);
    }
}

void EventSystem::clearEventQueue()
{
    std::lock_guard<std::mutex> lock(m_eventMutex);
    std::queue<std::shared_ptr<Event>> empty;
    std::swap(m_eventQueue, empty);
}

size_t EventSystem::getQueuedEventCount() const
{
    std::lock_guard<std::mutex> lock(m_eventMutex);
    return m_eventQueue.size();
}

void EventSystem::setMaxRecentEvents(size_t count)
{
    std::lock_guard<std::mutex> lock(m_eventMutex);
    m_maxRecentEvents = count;

    // Trim recent events if necessary
    while (m_recentEvents.size() > m_maxRecentEvents)
    {
        m_recentEvents.pop_front();
    }
}

size_t EventSystem::getMaxRecentEvents() const
{
    std::lock_guard<std::mutex> lock(m_eventMutex);
    return m_maxRecentEvents;
}

void EventSystem::processEvent(const std::type_index& typeIndex, Event* event)
{
    // Make a local copy of handlers to avoid holding the lock during handler execution
    std::vector<std::pair<size_t, void*>> handlers;
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        auto it = m_eventHandlers.find(typeIndex);
        if (it != m_eventHandlers.end())
        {
            handlers = it->second;
        }
    }

    // Call each handler
    for (auto& handlerPair : handlers)
    {
        if (event->isHandled())
        {
            break; // Skip remaining handlers if event is marked as handled
        }

        auto handlerPtr = handlerPair.second;

        // We need to cast back to the appropriate handler type based on the event type
        // This requires some template magic to invoke the correct handler

        // This is implemented using type erasure - the actual call is made through the
        // templated EventHandler function stored in the handler

        // The handler itself knows how to handle the specific event type
        // that was registered with it

        // Use the type index to determine how to cast and call the handler
        if (typeIndex == std::type_index(typeid(Event)))
        {
            auto& handler = *static_cast<EventHandler<Event>*>(handlerPtr);
            handler(*event);
        }
        else
        {
            // For all other event types, we use a templated helper function
            // The actual implementation would have a more sophisticated mechanism
            // to invoke the correct handler type based on the event type

            // NOTE: This is simplified for demonstration - in practice, we would use
            // a more sophisticated mechanism to invoke handlers with the correct type
            auto& baseHandler = *static_cast<std::function<void(Event&)>*>(handlerPtr);
            baseHandler(*event);
        }
    }
}