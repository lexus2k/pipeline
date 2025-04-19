#include "pipeline/pipeline_pads.h"
#include <iostream>

namespace lexus2k::pipeline
{
    /// @brief SimplePad pad

    bool SimplePad::queuePacket(std::shared_ptr<IPacket> packet, uint32_t timeout) noexcept
    {
        return processPacket(packet, timeout);
    }

    /// @brief Queued pad

    bool QueuePad::queuePacket(std::shared_ptr<IPacket> packet, uint32_t timeout) noexcept
    {
        std::cout << "QueuePad: Attempting to add packet to queue" << std::endl;
        std::unique_lock<std::mutex> lock(m_mutex);

        // Wait for space in the queue or timeout
        bool hasSpace = m_hasSpace.wait_for(lock, std::chrono::milliseconds(timeout), 
            [this] { return !m_isRunning.load(std::memory_order_relaxed) || m_queue.size() < m_maxQueueSize; });

        std::cout << "QueuePad: Space in queue: " << hasSpace << std::endl;
        std::cout << "QueuePad: Queue size: " << m_queue.size() << std::endl;
        std::cout << "QueuePad: isRunning: " << m_isRunning.load(std::memory_order_relaxed) << std::endl;
        if (!hasSpace || !m_isRunning.load(std::memory_order_relaxed))
        {
            return false; // Timeout or pad is not running
        }

        std::cout << "QueuePad: Adding packet to queue" << std::endl;
        // Add the packet to the queue
        m_queue.emplace_back(timeout, packet);
        lock.unlock();
        m_hasPackets.notify_one(); // Notify waiting threads
        return true;
    }

    void QueuePad::start() noexcept
    {
        if (m_isRunning.load(std::memory_order_relaxed) || m_thread.joinable())
        {
            return; // Already running
        }

        m_isRunning.store(true, std::memory_order_relaxed);

        // Start the processing thread
        m_thread = std::thread([this]() {
            while (m_isRunning.load(std::memory_order_relaxed))
            {
                std::pair<uint32_t,std::shared_ptr<IPacket>> packet;

                {
                    std::unique_lock<std::mutex> lock(m_mutex);

                    std::cout << "QueuePad: Waiting for packets" << std::endl;
                    // Wait for packets or stop signal
                    m_hasPackets.wait(lock, [this] { return !m_isRunning || !m_queue.empty(); });

                    if (!m_isRunning.load(std::memory_order_relaxed) && m_queue.empty())
                    {
                        break; // Exit if stopped and no packets remain
                    }

                    std::cout << "QueuePad: Retrieving packet from queue" << std::endl;

                    // Retrieve the next packet
                    packet = std::move(m_queue.front());
                    m_queue.erase(m_queue.begin());
                }

                // Process the packet
                processPacket(packet.second, packet.first);
            }
        });
    }

    void QueuePad::stop() noexcept
    {
        if (!m_thread.joinable())
        {
            return; // Not running
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_isRunning.store(false, std::memory_order_relaxed);
        }

        // Notify all waiting threads
        m_hasPackets.notify_all();
        m_hasSpace.notify_all();

        // Join the thread
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }
}