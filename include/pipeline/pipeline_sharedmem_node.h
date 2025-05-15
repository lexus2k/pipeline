#ifndef LEXUS2K_PIPELINE_SHAREDMEM_NODE_H__
#define LEXUS2K_PIPELINE_SHAREDMEM_NODE_H__

#include "pipeline_node.h"

#if defined(__linux__) || defined(__APPLE__)

#include <pthread.h>

namespace lexus2k::pipeline
{
    class SharedPublisherNode : public INode
    {
    public:
        SharedPublisherNode(const std::string name, size_t size = 1024 * 1024, uint32_t maxQueueSize = 1);
        SharedPublisherNode(const SharedPublisherNode&) = delete;
        SharedPublisherNode& operator=(const SharedPublisherNode&) = delete;

        ~SharedPublisherNode() override { stop(); }

        /**
         * @brief Adds a new input pad to the node.
         * @tparam T The type of the pad.
         * @param name The name of the pad.
         * @param args Additional arguments for the pad's constructor.
         * @return A reference to the newly added pad.
         */
        template <typename T, typename... Args>
        T& addChannel(const std::string& name, Args&&... args)
        {
            return INode::addInput<T>(name, std::forward<Args>(args)...);
        }

        /**
         * @brief Adds a new input pad to the node.
         * @param name The name of the pad.
         * @param args Additional arguments for the pad's constructor.
         * @return A reference to the newly added pad.
         */
        template<typename... Args>
        auto& addChannel(const std::string &name, Args&&... args)
        {
            return INode::addInput<SimplePad>(name, std::forward<Args>(args)...);
        }

        bool start() noexcept override;

        void stop() noexcept override;

    protected:
        bool processPacket(std::shared_ptr<IPacket> packet, IPad& inputPad, uint32_t timeoutMs) noexcept override;

    private:
        bool createSharedMem() noexcept;
        void destroySharedMem() noexcept;
        bool serializeToSharedMem(std::shared_ptr<IPacket> packet, IPad& inputPad) noexcept;
        bool waitForFreeSlot(uint32_t timeoutMs) noexcept;

    private:
        std::string m_name; ///< The name of the shared memory segment.
        size_t m_size = 0;  ///< Size of the shared memory segment.
        uint32_t m_maxQueueSize = 1; /// Maximum queue size
        void *m_ptr = nullptr; ///< Pointer to the shared memory segment.
    };


    class SharedSubscriberNode : public INode
    {
    public:
        SharedSubscriberNode(const std::string name);
        SharedSubscriberNode(const SharedSubscriberNode&) = delete;
        SharedSubscriberNode& operator=(const SharedSubscriberNode&) = delete;

        ~SharedSubscriberNode() override { stop(); }

        virtual bool start() noexcept override;

        virtual void stop() noexcept override;
    protected:
        virtual std::shared_ptr<IPacket> createPacket(IPad& pad) noexcept = 0;
    private:
        bool attachSharedMem() noexcept;
        void detachSharedMem() noexcept;
        void threadBody() noexcept;
        bool processPacket(std::shared_ptr<IPacket> packet, IPad& inputPad, uint32_t timeoutMs) noexcept override;
        int waitForPacket(uint32_t timeoutMs) noexcept;
        bool deserializeFromSharedMem() noexcept;
    private:
        std::string m_name; ///< The name of the shared memory segment.
        size_t m_size = 0;  ///< Size of the shared memory segment.
        void *m_ptr = nullptr; ///< Pointer to the shared memory segment.
        std::thread m_thread; ///< Thread for processing packets.
        std::atomic_bool m_stop_thread = true; ///< Flag to stop the thread.
    };

    template <typename T>
    class SharedSubscriberNodeT : public SharedSubscriberNode
    {
    public:
        SharedSubscriberNodeT(const std::string name) : SharedSubscriberNode(name) {}

        ~SharedSubscriberNodeT() override = default;
    protected:
        std::shared_ptr<IPacket> createPacket(IPad& pad) noexcept override
        {
            return std::make_shared<T>();
        }
    };

}

#endif

#endif // LEXUS2K_PIPELINE_SHAREDMEM_NODE_H__