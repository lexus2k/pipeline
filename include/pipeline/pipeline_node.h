#ifndef LEXUS2K_PIPELINE_NODE_H
#define LEXUS2K_PIPELINE_NODE_H

#include <vector>
#include <memory>
#include <string>

#include "pipeline_pad.h"
#include "pipeline_pads.h"
#include "pipeline_packet.h"

namespace lexus2k::pipeline
{
    class Pipeline;

    /**
     * @class INode
     * @brief Represents a node in the pipeline, managing pads and processing packets.
     */
    class INode
    {
    public:
        /**
         * @brief Default constructor.
         */
        INode() = default;

        /**
         * @brief Deleted copy constructor.
         */
        INode(const INode&) = delete;

        /**
         * @brief Deleted copy assignment operator.
         */
        INode& operator=(const INode&) = delete;

        /**
         * @brief Virtual destructor.
         */
        virtual ~INode() = default;

        /**
         * @brief Pushes a packet to a pad by name.
         * @param name The name of the pad.
         * @param packet The packet to push.
         * @param timeout The timeout for the operation.
         * @return True if the packet was successfully pushed, false otherwise.
         */
        bool pushPacket(const std::string& name, std::shared_ptr<IPacket> packet, uint32_t timeout = 0) const noexcept;

        /**
         * @brief Adds a new input pad to the node.
         * @tparam T The type of the pad.
         * @param name The name of the pad.
         * @param args Additional arguments for the pad's constructor.
         * @return A reference to the newly added pad.
         */
        template <typename T, typename... Args>
        T& addInput(const std::string& name, Args&&... args)
        {
            auto pad = std::make_shared<T>(std::forward<Args>(args)...);
            pad->setType(PadType::INPUT);
            pad->setParent(this);
            m_pads.emplace_back(name, pad);
            pad->setIndex(m_pads.size() - 1);
            return *pad;
        }

        /**
         * @brief Adds a new input pad to the node.
         * @param name The name of the pad.
         * @param args Additional arguments for the pad's constructor.
         * @return A reference to the newly added pad.
         */
        template<typename... Args>
        auto& addInput(const std::string &name, Args&&... args)
        {
            return addInput<SimplePad>(name, std::forward<Args>(args)...);
        }

        /**
         * @brief Adds a new output pad to the node.
         * @param name The name of the pad.
         * @return A reference to the newly added pad.
         */
        auto& addOutput(const std::string &name)
        {
            auto pad = std::make_shared<SimplePad>();
            pad->setType(PadType::OUTPUT);
            pad->setParent(this);
            m_pads.emplace_back(name, pad);
            pad->setIndex(m_pads.size() - 1);
            return *pad;
        }

        /**
         * @brief Retrieves a pad by name.
         * @param name The name of the pad.
         * @return A reference to the pad.
         * @throws std::runtime_error if the pad is not found.
         */
        IPad& operator[](const std::string &name) const;

        /**
         * @brief Retrieves a pad by index.
         * @param index The index of the pad.
         * @return A reference to the pad.
         * @throws std::runtime_error if the pad is not found.
         */
        IPad& operator[](size_t index) const;

        /**
         * @brief Starts the node.
         *
         * This method is called when the pipeline is started. It can be overridden
         * by derived classes to perform any initialization or setup required before
         * the node begins processing packets. For example, a node might open files,
         * initialize resources, or start background threads in this method.
         * 
         * It guarantees that if any pad start fails, node will not be started.
         *
         * By default, this method does nothing.
         *
         * @note This method is invoked automatically by the pipeline when the
         *       `Pipeline::start()` method is called.
         */
        virtual bool start() noexcept { return true; }

        /**
         * @brief Stops the node.
         *
         * This method is called when the pipeline is stopped. It can be overridden
         * by derived classes to perform any cleanup or resource deallocation required
         * after the node finishes processing packets. For example, a node might close
         * files, release resources, or stop background threads in this method.
         *
         * By default, this method does nothing.
         *
         * @note This method is invoked automatically by the pipeline when the
         *       `Pipeline::stop()` method is called.
         */
        virtual void stop() noexcept {};

    protected:
        /**
         * @brief Processes a packet received on an input pad.
         * @param packet The packet to process.
         * @param inputPad The input pad that received the packet.
         * @return True if the packet was successfully processed, false otherwise.
         */
        virtual bool processPacket(std::shared_ptr<IPacket> packet, IPad& inputPad) noexcept { return false; }

        // Helper method to find a pad by name
        IPad* getPadByName(const std::string& name, PadType type = PadType::UNDEFINED) const noexcept;

        IPad* getPadByIndex(size_t index) const noexcept;

        /**
         * @brief Starts the node. It guarantees that if any pad start fails,
         *        node will not be started
         * 
         */
        bool _start() noexcept;

        /**
         * @brief Stops the node.
         */
        void _stop() noexcept;

    private:
        std::vector<std::pair<std::string, std::shared_ptr<IPad>>> m_pads; ///< Collection of pads.

        friend class IPad;
        friend class Pipeline;
    };

    /**
     * @class Node
     * @brief A template class for processing packets derived from `IPacket`.
     *
     * The `Node` class is a specialized implementation of `INode` that provides
     * functionality for processing packets of a specific type derived from `IPacket`.
     * It uses `std::dynamic_pointer_cast` to safely cast incoming packets to the
     * specified type and delegates the processing to a type-specific `processPacket`
     * method.
     *
     * @tparam T The type of the packet to process. Must be derived from `IPacket`.
     */
    template <typename T, typename = std::enable_if_t<std::is_base_of_v<IPacket, T>>>
    class Node : public INode
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the `Node` instance. This constructor does not perform
         * any specific initialization beyond the base class constructor.
         */
        Node() : INode() {}

        /**
         * @brief Default destructor.
         *
         * Ensures proper cleanup of the `Node` instance. This destructor is
         * defaulted and does not perform any specific cleanup beyond the base
         * class destructor.
         */
        ~Node() = default;

    protected:
        /**
         * @brief Processes a packet received on an input pad.
         *
         * This method overrides the `processPacket` method in the `INode` base
         * class. It attempts to cast the incoming packet to the specified type
         * `T` using `std::dynamic_pointer_cast`. If the cast is successful, it
         * delegates the processing to the type-specific `processPacket` method.
         *
         * @param packet The packet to process, as a shared pointer to `IPacket`.
         * @param inputPad The input pad that received the packet.
         */
        bool processPacket(std::shared_ptr<IPacket> packet, IPad& inputPad) noexcept override final
        {
            // TODO: Make 2 implementations with RTTI support and without
            auto derivedPacket = std::dynamic_pointer_cast<T>(packet);
            if (derivedPacket)
            {
                return processPacket(derivedPacket, inputPad);
            }
            return false; // Packet type mismatch
        }

        /**
         * @brief Processes a packet of the specified type.
         *
         * This method must be implemented by derived classes to provide
         * type-specific processing logic for packets of type `T`.
         *
         * @param packet The packet to process, as a shared pointer to `T`.
         * @param inputPad The input pad that received the packet.
         */
        virtual bool processPacket(std::shared_ptr<T> packet, IPad& inputPad) noexcept = 0;
    };

    /**
     * @class Node2
     * @brief A template class for processing packets of two types derived from `IPacket`.
     *
     * The `Node2` class is a specialized implementation of `INode` that provides
     * functionality for processing packets of two specific types derived from `IPacket`.
     * It uses `std::dynamic_pointer_cast` to safely cast incoming packets to the
     * specified types (`T1` or `T2`) and delegates the processing to type-specific
     * `processPacket` methods.
     *
     * @tparam T1 The first type of the packet to process. Must be derived from `IPacket`.
     * @tparam T2 The second type of the packet to process. Must be derived from `IPacket`.
     */
    template <typename T1, typename T2, typename = std::enable_if_t<std::is_base_of_v<IPacket, T1> && std::is_base_of_v<IPacket, T2>>>
    class Node2 : public INode
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the `Node2` instance. This constructor does not perform
         * any specific initialization beyond the base class constructor.
         */
        Node2() : INode() {}

        /**
         * @brief Default destructor.
         *
         * Ensures proper cleanup of the `Node2` instance. This destructor is
         * defaulted and does not perform any specific cleanup beyond the base
         * class destructor.
         */
        ~Node2() = default;

    protected:
        /**
         * @brief Processes a packet received on an input pad.
         *
         * This method overrides the `processPacket` method in the `INode` base
         * class. It determines the type of the incoming packet based on the
         * index of the input pad. If the input pad index is `0`, it attempts
         * to cast the packet to type `T1`. If the input pad index is `1`, it
         * attempts to cast the packet to type `T2`. If the cast is successful,
         * it delegates the processing to the corresponding type-specific
         * `processPacket` method.
         *
         * @param packet The packet to process, as a shared pointer to `IPacket`.
         * @param inputPad The input pad that received the packet.
         */
        bool processPacket(std::shared_ptr<IPacket> packet, IPad& inputPad) noexcept override final
        {
            if (inputPad.getIndex() == 0) {
                auto derivedPacket = std::dynamic_pointer_cast<T1>(packet);
                if (derivedPacket)
                {
                    return processPacket(derivedPacket, inputPad);
                }
            }
            else if (inputPad.getIndex() == 1)
            {
                auto derivedPacket = std::dynamic_pointer_cast<T2>(packet);
                if (derivedPacket)
                {
                    return processPacket(derivedPacket, inputPad);
                }
            }
            return false; // Packet type mismatch
        }

        /**
         * @brief Processes a packet of the first type (`T1`).
         *
         * This method must be implemented by derived classes to provide
         * type-specific processing logic for packets of type `T1`.
         *
         * @param packet The packet to process, as a shared pointer to `T1`.
         * @param inputPad The input pad that received the packet.
         */
        virtual bool processPacket(std::shared_ptr<T1> packet, IPad& inputPad) noexcept = 0;

        /**
         * @brief Processes a packet of the second type (`T2`).
         *
         * This method must be implemented by derived classes to provide
         * type-specific processing logic for packets of type `T2`.
         *
         * @param packet The packet to process, as a shared pointer to `T2`.
         * @param inputPad The input pad that received the packet.
         */
        virtual bool processPacket(std::shared_ptr<T2> packet, IPad& inputPad) noexcept = 0;
    };
} // namespace lexus2k::pipeline

#endif // LEXUS2K_PIPELINE_NODE_H