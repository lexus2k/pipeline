#ifndef __LEXUS2K_PIPELINE_SHAREDMEM_NODE_QUEUE_H__
#define __LEXUS2K_PIPELINE_SHAREDMEM_NODE_QUEUE_H__

#include <atomic>
#include <cstdint>
#include <pthread.h>

namespace lexus2k::pipeline
{

    struct PacketHeader
    {
        uint32_t size; ///< Size of the packet.
        uint32_t channel; ///< Channel number.
        uintptr_t offset; ///< Offset of the packet data in the shared memory.
    };

    struct QueueHeader
    {
        uint32_t size; ///< Size of the queue.
        uint32_t count; ///< Number of packets in the queue.
        uint32_t head; ///< Head index of the queue.
        uint32_t tail; ///< Tail index of the queue.
        PacketHeader packets[0]; ///< Offsets of the packets in the queue.
    };

    struct SharedMemoryHeader
    {
        std::atomic_int version; ///< Version of the shared memory segment.
        std::atomic_int size;   ///< Size of the shared memory segment.
        std::atomic_bool is_valid; ///< Flag indicating if the shared memory is valid.
        pthread_mutex_t mutex; ///< Mutex for thread safety.
        pthread_cond_t condPacketReady; ///< Condition variable for packet readiness.
        pthread_cond_t condSlotAvailable; ///< Condition variable for slot availability.
        uintptr_t writeOffset; ///< Offset of the data in the shared memory.
        QueueHeader queue; ///< Header for the queue of packets.
    };

}

#endif