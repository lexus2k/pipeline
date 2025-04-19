#ifndef PIPELINE_PACKET_H
#define PIPELINE_PACKET_H

namespace lexus2k::pipeline
{
    /**
     * @class IPacket
     * @brief Represents a generic data packet in the pipeline.
     *
     * The `IPacket` interface serves as the base class for all data packets
     * that are passed between nodes in the pipeline. Derived classes can
     * implement specific types of packets with additional data and functionality.
     */
    class IPacket {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived classes when deleted through
         * a pointer to `IPacket`.
         */
        virtual ~IPacket() = default;
    };

} // namespace lexus2k::pipeline

#endif // PIPELINE_PACKET_H