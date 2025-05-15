// -------------------------------------------------------------------------
// ThreadSafeQueue.h
// -------------------------------------------------------------------------
#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace PixelCraft::Utility
{

    /**
     * @brief Thread-safe queue implementation for producer-consumer pattern
     * 
     * This queue is designed for safe sharing between threads, such as
     * background worker threads or job systems. It provides blocking and
     * non-blocking operations with proper synchronization.
     * 
     * @tparam T Type of elements stored in the queue
     */
    template<typename T>
    class ThreadSafeQueue
    {
    public:
        /**
         * @brief Constructor
         */
        ThreadSafeQueue()
            : m_shutdown(false)
        {
        }

        /**
         * @brief Destructor, ensures proper shutdown
         */
        ~ThreadSafeQueue()
        {
            shutdown();
        }

        /**
         * @brief Push an item to the back of the queue
         * @param item Item to add to the queue
         */
        void push(T item)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            // Don't accept new items if we're shutting down
            if (m_shutdown)
            {
                return;
            }

            m_queue.push(std::move(item));
            m_condition.notify_one();
        }

        /**
         * @brief Try to pop an item from the front of the queue without blocking
         * @param item Reference to store the popped item
         * @return True if an item was popped, false if the queue was empty
         */
        bool tryPop(T& item)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_queue.empty() || m_shutdown)
            {
                return false;
            }

            item = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }

        /**
         * @brief Wait for an item and pop it from the front of the queue
         * This method will block until an item is available or the queue is shut down
         * @param item Reference to store the popped item
         * @return True if an item was popped, false if the queue was shut down
         */
        bool waitAndPop(T& item)
        {
            std::unique_lock<std::mutex> lock(m_mutex);

            // Wait until the queue has an item or is shut down
            m_condition.wait(lock, [this]() {
                return !m_queue.empty() || m_shutdown;
                             });

            // If we were woken up due to shutdown and the queue is empty, return false
            if (m_queue.empty())
            {
                return false;
            }

            item = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }

        /**
         * @brief Check if the queue is empty
         * @return True if the queue is empty
         */
        bool empty() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.empty();
        }

        /**
         * @brief Get the current size of the queue
         * @return Number of items in the queue
         */
        size_t size() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.size();
        }

        /**
         * @brief Shut down the queue
         * This will wake any waiting threads and prevent new items from being added
         */
        void shutdown()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_shutdown = true;
            m_condition.notify_all();
        }

        /**
         * @brief Check if the queue is shut down
         * @return True if the queue is shut down
         */
        bool isShutdown() const
        {
            return m_shutdown;
        }

        /**
         * @brief Clear all items from the queue
         */
        void clear()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::queue<T> empty;
            std::swap(m_queue, empty);
        }

    private:
        std::queue<T> m_queue;                ///< The underlying queue
        mutable std::mutex m_mutex;           ///< Mutex for synchronization
        std::condition_variable m_condition;  ///< Condition variable for blocking operations
        std::atomic<bool> m_shutdown;         ///< Flag to indicate if the queue is shut down

        // Disable copying and assignment
        ThreadSafeQueue(const ThreadSafeQueue&) = delete;
        ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
    };

} // namespace PixelCraft::Utility