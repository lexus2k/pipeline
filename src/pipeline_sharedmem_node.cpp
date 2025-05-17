#include <pipeline/pipeline_sharedmem_node.h>
#include <pipeline/pipeline_node.h>
#include "pipeline_shared_queue.h"

#if defined(__linux__) || defined(__APPLE__)

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

#include <atomic>
#include <iostream>
#include <thread>

#define PTR(x) static_cast<SharedMemoryHeader *>(x)

namespace lexus2k::pipeline
{

static bool InitializeSharedConditionVariable(pthread_cond_t& cond) noexcept
{
    pthread_condattr_t cattr;
    if (pthread_condattr_init(&cattr) != 0) {
        return false;
    }
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    if (pthread_cond_init(&cond, &cattr) != 0) {
        pthread_condattr_destroy(&cattr);
        return false;
    }
    pthread_condattr_destroy(&cattr);
    return true;
}

SharedPublisherNode::SharedPublisherNode(const std::string name, size_t size, uint32_t maxQueueSize)
    : INode()
    , m_name(name)
    , m_size(size)
    , m_maxQueueSize(maxQueueSize)
{
}

bool SharedPublisherNode::start() noexcept
{
    return createSharedMem();
}

void SharedPublisherNode::stop() noexcept
{
    destroySharedMem();
}

bool SharedPublisherNode::createSharedMem() noexcept
{
    int fd = -1;
    if (m_ptr != nullptr) {
        return false;
    }
    if (m_size == 0 || m_name.empty()) {
        return false;
    }
    // Try to unlink the shared memory if it already exists
    shm_unlink(m_name.c_str());
    fd = shm_open(m_name.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        return false;
    }
    if (ftruncate(fd, m_size) == -1) {
        close(fd);
        return false;
    }
    m_ptr = mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (m_ptr == MAP_FAILED) {
        // std::cerr << "[PUB] Failed to map shared memory: " << m_name << std::endl;
        shm_unlink(m_name.c_str());
        return false;
    }
    auto ptr = PTR(m_ptr);
    ptr->is_valid = false;
    ptr->version = random();
    ptr->size = m_size;

    // Initialize the shared memory mutex
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST);
    if (pthread_mutex_init(&ptr->mutex, &mattr) !=0) {
        munmap(m_ptr, m_size);
        m_ptr = nullptr;
        shm_unlink(m_name.c_str());
        return false;
    }
    pthread_mutexattr_destroy(&mattr);

    // Initialize the shared memory condition variable for packet ready
    if (!InitializeSharedConditionVariable(ptr->condPacketReady)) {
        pthread_mutex_destroy(&ptr->mutex);
        munmap(m_ptr, m_size);
        m_ptr = nullptr;
        shm_unlink(m_name.c_str());
        // std::cerr << "[PUB] Failed to initialize condition variable." << std::endl;
        return false;
    }
    // Initialize the shared memory condition variable for slot available
    if (!InitializeSharedConditionVariable(ptr->condSlotAvailable)) {
        pthread_cond_destroy(&ptr->condPacketReady);
        pthread_mutex_destroy(&ptr->mutex);
        munmap(m_ptr, m_size);
        m_ptr = nullptr;
        shm_unlink(m_name.c_str());
        // std::cerr << "[PUB] Failed to initialize condition variable. 1" << std::endl;
        return false;
    }

    ptr->queue.size = m_maxQueueSize;
    ptr->queue.count = 0;
    ptr->queue.head = 0;
    ptr->queue.tail = 0;
    ptr->writeOffset = sizeof(SharedMemoryHeader) + sizeof(PacketHeader) * m_maxQueueSize;

    ptr->is_valid = true;
    // std::cout << "[PUB] Shared memory created: " << m_name << std::endl;
    return true;
}

void SharedPublisherNode::destroySharedMem() noexcept
{
    // std::cout << "[PUB] Destroying shared memory..." << std::endl;
    if (m_ptr != nullptr) {
        pthread_mutex_lock(&PTR(m_ptr)->mutex);
        PTR(m_ptr)->is_valid = false;
        pthread_cond_signal(&PTR(m_ptr)->condPacketReady);
        pthread_cond_signal(&PTR(m_ptr)->condSlotAvailable);
        pthread_mutex_unlock(&PTR(m_ptr)->mutex);
        pthread_cond_destroy(&PTR(m_ptr)->condPacketReady);
        pthread_cond_destroy(&PTR(m_ptr)->condSlotAvailable);
        pthread_mutex_destroy(&PTR(m_ptr)->mutex);
        munmap(m_ptr, m_size);
        m_ptr = nullptr;
    }
    shm_unlink(m_name.c_str());
    // std::cout << "[PUB] Shared memory unlinked: " << m_name << std::endl;
}

bool SharedPublisherNode::processPacket(std::shared_ptr<IPacket> packet, IPad& inputPad, uint32_t timeoutMs) noexcept
{
    if (m_ptr == nullptr) {
        return false;
    }
    auto ptr = PTR(m_ptr);
    pthread_mutex_lock(&ptr->mutex);
    if (!waitForFreeSlot(timeoutMs)) {
        pthread_mutex_unlock(&ptr->mutex);
        return false;
    }
    auto result = serializeToSharedMem(packet, inputPad);
    if (result) {
        pthread_cond_signal(&ptr->condPacketReady);
    } else {
        // std::cout << "[PUB] Failed to serialize packet." << std::endl;
    }
    pthread_mutex_unlock(&ptr->mutex);
    return result;
}

bool SharedPublisherNode::waitForFreeSlot(uint32_t timeoutMs) noexcept
{
    auto ptr = PTR(m_ptr);
    if (!ptr->is_valid) {
        return false;
    }
    // TODO:: Check for slot and add waiting for slot
    while (ptr->queue.size == ptr->queue.count) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += timeoutMs * 1000000;
        ts.tv_sec += ts.tv_nsec / 1000000000;
        ts.tv_nsec = ts.tv_nsec % 1000000000;
        int result = pthread_cond_timedwait(&ptr->condSlotAvailable, &ptr->mutex, &ts);
        if (result == ETIMEDOUT) {
            // std::cerr << "[PUB] Timed out waiting for condition variable." << std::endl;
            return false;
        }
        else if (result == EINVAL) {
            // std::cerr << "[PUB] Invalid condition variable." << std::endl;
            return false;
        }
    }
    return true;
}

bool SharedPublisherNode::serializeToSharedMem(std::shared_ptr<IPacket> packet, IPad& inputPad) noexcept
{
    auto ptr = PTR(m_ptr);
    auto result = packet->serializeTo((uint8_t *)m_ptr + ptr->writeOffset, m_size - ptr->writeOffset);
    if (result < 0) {
        ptr->writeOffset = sizeof(SharedMemoryHeader) + sizeof(PacketHeader) * m_maxQueueSize;
        result = packet->serializeTo((uint8_t *)m_ptr + ptr->writeOffset, m_size - ptr->writeOffset);
    }
    if (result >= 0) {
        auto& header = ptr->queue.packets[ptr->queue.tail];
        ptr->queue.tail = (ptr->queue.tail + 1) % ptr->queue.size;
        ptr->queue.count++;
        header.size = result;
        header.channel = inputPad.getIndex();
        header.offset = ptr->writeOffset;
        ptr->writeOffset += result;
        if (ptr->writeOffset >= m_size) {
            ptr->writeOffset = sizeof(SharedMemoryHeader) + sizeof(PacketHeader) * m_maxQueueSize;
        }
        // std::cout << "[PUB] Packet serialized successfully. notifying" << std::endl;
    } else {
        // std::cout << "[PUB] Failed to serialize packet." << std::endl;
        return false;
    }
    return true;
}


//////////////////////////////////////////////////////////////////////////////

SharedSubscriberNode::SharedSubscriberNode(const std::string name)
    : m_name(name)
{
    m_size = sizeof(SharedMemoryHeader) + 1024 * 1024; // 1MB
}

bool SharedSubscriberNode::start() noexcept
{
    if (!m_stop_thread.load()) {
        return false;
    }
    if (m_thread.joinable()) {
        return true;
    }
    m_stop_thread.store(false);
    m_thread = std::thread(&SharedSubscriberNode::threadBody, this);
    return true;
}

void SharedSubscriberNode::stop() noexcept
{
    if (m_thread.joinable()) {
        // std::cout << "[SUB] Stopping thread..." << std::endl;
        m_stop_thread.store(true);
        m_thread.join();
    }
}

void SharedSubscriberNode::threadBody() noexcept
{
    while (!m_stop_thread.load()) {
        if (m_ptr == nullptr) {
            if (!attachSharedMem()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
        }
        auto ptr = PTR(m_ptr);
        {
            int result = pthread_mutex_lock(&ptr->mutex);
            if (result == EOWNERDEAD) {
                // pthread_mutex_consistent(&ptr->mutex);
                pthread_mutex_unlock(&ptr->mutex);
                // std::cerr << "[SUB] Mutex owner is dead." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                detachSharedMem();
                continue;
            } else if (result != 0) {
                // std::cerr << "[SUB] Failed to lock mutex." << std::endl;
                detachSharedMem();
                continue;
            }
        }
        if (auto result = waitForPacket(100)) {
            pthread_mutex_unlock(&ptr->mutex);
            if (result == EINVAL) {
                // std::cerr << "[SUB] Invalid condition variable." << std::endl;
                detachSharedMem();
                continue;
            } else {
                // std::cerr << "[SUB] Timed out waiting for condition variable." << std::endl;
            }
            continue;
        }
        deserializeFromSharedMem();
        pthread_cond_signal(&ptr->condSlotAvailable);
        pthread_mutex_unlock(&ptr->mutex);
    }
    if (m_ptr != nullptr) {
        detachSharedMem();
    }
}

int SharedSubscriberNode::waitForPacket(uint32_t timeoutMs) noexcept
{
    auto ptr = PTR(m_ptr);
    if (ptr->queue.count != 0) {
        return 0;
    }
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += timeoutMs * 1000000;
    ts.tv_sec += ts.tv_nsec / 1000000000;
    ts.tv_nsec = ts.tv_nsec % 1000000000;
    int result = pthread_cond_timedwait(&PTR(m_ptr)->condPacketReady, &PTR(m_ptr)->mutex, &ts);
    if (result == ETIMEDOUT) {
        // std::cout << "[SUB] Timed out waiting for condition variable." << std::endl;
    }
    else if (result == EINVAL) {
        // std::cerr << "[SUB] Invalid condition variable." << std::endl;
    }
    return result;
}

bool SharedSubscriberNode::deserializeFromSharedMem() noexcept
{
    auto ptr = PTR(m_ptr);
    auto& header = ptr->queue.packets[ptr->queue.head];
    ptr->queue.head = (ptr->queue.head + 1) % ptr->queue.size;
    ptr->queue.count--;
    IPad* pad = getPadByIndex(header.channel);
    if (!pad) {
        return false;
    }
    std::shared_ptr<IPacket> packet = createPacket(*pad);
    if (!packet) {
        return false;
    }
    auto result = packet->deserializeFrom((uint8_t *)m_ptr + header.offset, header.size);
    if (result < 0) {
        return false;
    }
    // TODO: Unlock the mutex before pushing the packet
    return pad->pushPacket(packet, 0);
}

bool SharedSubscriberNode::attachSharedMem() noexcept
{
    int fd = -1;
    if (m_ptr != nullptr) {
        return false;
    }
    if (m_name.empty()) {
        return false;
    }
    fd = shm_open(m_name.c_str(), O_RDWR, 0666);
    if (fd < 0) {
        // std::cerr << "[SUB] Failed to open shared memory: " << m_name << std::endl;
        return false;
    }
    struct stat _stat;
    if (fstat(fd, &_stat) == -1) {
        // std::cerr << "[SUB] Failed to get file status: " << m_name << std::endl;
        close(fd);
        return false;
    }
    m_size = _stat.st_size;
    m_ptr = mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (m_ptr == MAP_FAILED) {
        // std::cerr << "[SUB] Failed to map shared memory: " << m_name << std::endl;
        return false;
    }
    if (PTR(m_ptr)->is_valid.load() == false) {
        // std::cerr << "[SUB] Shared memory is not valid." << std::endl;
        munmap(m_ptr, m_size);
        m_ptr = nullptr;
        return false;
    }
    m_size = PTR(m_ptr)->size;
    // std::cout << "[SUB] Shared memory mapped: " << m_ptr << std::endl;
    return true;
}

void SharedSubscriberNode::detachSharedMem() noexcept
{
    // std::cout << "[SUB] Detaching shared memory..." << std::endl;
    if (m_ptr != nullptr) {
        munmap(m_ptr, m_size);
        m_ptr = nullptr;
        m_size = 0;
        // std::cout << "[SUB] Shared memory detached." << std::endl;
    }
}

bool SharedSubscriberNode::processPacket(std::shared_ptr<IPacket> packet, IPad& inputPad, uint32_t timeoutMs) noexcept
{
    return false;
}

} // namespace lexus2k::pipeline

#endif
