// -------------------------------------------------------------------------
// ThreadSafeQueue.h
// -------------------------------------------------------------------------
#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

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
            m_queue.push(item);
            m_cv.notify_one();
        }

        /**
         * @brief Wait and pop an item from the queue
         * @param item Output parameter for the popped item
         */
        void waitAndPop(T& item)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return !m_queue.empty() || m_shutdown; });

            if (m_shutdown)
            {
                return;
            }

            item = m_queue.front();
            m_queue.pop();
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

    private:
        std::queue<T> m_queue;
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
        bool m_shutdown = false;
    };

} // namespace PixelCraft::Core