// -------------------------------------------------------------------------
// ThreadSafeQueue.h
// -------------------------------------------------------------------------
#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

// In namespace PixelCraft
namespace PixelCraft::Core
{

    /**
     * @brief Thread-safe queue for asynchronous resource loading
     */
    template<typename T>
    class ThreadSafeQueue
    {
    public:
        /**
         * @brief Add an item to the queue
         * @param item The item to add
         */
        void push(const T& item)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_shutdown)
            {
                return; // Don't accept new items if shutting down
            }
            m_queue.push(item);
            m_cv.notify_one();
        }

        /**
         * @brief Wait and pop an item from the queue
         * @param item Output parameter for the popped item
         * @return True if an item was successfully popped, false if the queue was shut down
         */
        bool waitAndPop(T& item)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return !m_queue.empty() || m_shutdown; });

            if (m_shutdown)
            {
                return false; // Queue was shut down
            }

            if (m_queue.empty())
            {
                return false; // Spurious wakeup
            }

            item = m_queue.front();
            m_queue.pop();
            return true; // Successfully popped an item
        }

        /**
         * @brief Try to pop an item without waiting
         * @param item Output parameter for the popped item
         * @return True if an item was successfully popped, false if queue is empty or shut down
         */
        bool tryPop(T& item)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty() || m_shutdown)
            {
                return false;
            }

            item = m_queue.front();
            m_queue.pop();
            return true;
        }

        /**
         * @brief Check if the queue is empty
         * @return True if empty, false otherwise
         */
        bool empty() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.empty();
        }

        /**
         * @brief Get the size of the queue
         * @return Number of items in the queue
         */
        size_t size() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.size();
        }

        /**
         * @brief Signal shutdown to wake up any waiting threads
         */
        void shutdown()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_shutdown = true;
            m_cv.notify_all();
        }

        /**
         * @brief Check if the queue is in shutdown state
         * @return True if shutting down, false otherwise
         */
        bool isShutdown() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_shutdown;
        }

        /**
         * @brief Reset the shutdown state (for reuse)
         */
        void reset()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_shutdown = false;
            // Clear any remaining items
            while (!m_queue.empty())
            {
                m_queue.pop();
            }
        }

    private:
        std::queue<T> m_queue;
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
        bool m_shutdown = false;
    };

} // namespace PixelCraft::Core
