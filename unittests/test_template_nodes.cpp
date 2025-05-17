#include <gtest/gtest.h>
#include "pipeline/pipeline.h"
#include <memory>
#include <iostream>

using namespace lexus2k::pipeline;

// Define custom packet types for testing
class PacketA : public IPacket {
public:
    PacketA() : data(0) {}
    PacketA(size_t value) : data(value) {}

    size_t serializeTo(void* ptr, size_t maxSize) noexcept override {
        if (maxSize < sizeof(size_t)) {
            return -1; // Not enough space
        }
        *(size_t *)ptr = data;
        return sizeof(size_t);
    }

    size_t deserializeFrom(const void* ptr, size_t size) noexcept override {
        if (size < sizeof(size_t)) {
            return -1;
        }
        data = *(size_t *)ptr;
        return sizeof(size_t);
    }

    size_t getData() const {
        return data;
    }
private:
    size_t data; // Example data member
};

class PacketB : public IPacket {
public:
};

// Define a custom Node class for testing
class TestNode : public Node<PacketA> {
protected:
    bool processPacket(const std::shared_ptr<PacketA> packet, IPad& inputPad, uint32_t timeoutMs) noexcept override {
        std::cout << "Processing PacketA in TestNode" << std::endl;
        processed = true;
        return true;
    }

public:
    bool processed = false;
};

// Define a custom Node2 class for testing
class TestNode2 : public Node2<PacketA, PacketB> {
protected:
    bool processPacket(std::shared_ptr<PacketA> packet, IPad& inputPad, uint32_t timeoutMs) noexcept override {
        std::cout << "Processing PacketA in TestNode2" << std::endl;
        processedA = true;
        return true;
    }

    bool processPacket(std::shared_ptr<PacketB> packet, IPad& inputPad, uint32_t timeoutMs) noexcept override {
        std::cout << "Processing PacketB in TestNode2" << std::endl;
        processedB = true;
        return true;
    }

public:
    bool processedA = false;
    bool processedB = false;
};

// Unit test fixture
class TemplateNodeTest : public ::testing::Test {
protected:
    std::shared_ptr<Pipeline> pipeline;

    void SetUp() override {
        pipeline = std::make_shared<Pipeline>();
    }

    void TearDown() override {
        pipeline.reset();
    }
};

// Test for Node<PacketA>
TEST_F(TemplateNodeTest, SingleTypeNodeTest) {
    auto& testNode = *pipeline->addNode<TestNode>();
    testNode.addInput("input");

    EXPECT_TRUE(pipeline->start());

    // Push a PacketA to the node
    testNode["input"].pushPacket(std::make_shared<PacketA>(), 0);

    EXPECT_TRUE(testNode.processed);
}

// Test for Node2<PacketA, PacketB>
TEST_F(TemplateNodeTest, DualTypeNodeTest) {
    auto& testNode2 = *pipeline->addNode<TestNode2>();
    testNode2.addInput("input_0");
    testNode2.addInput("input_1");

    EXPECT_TRUE(pipeline->start());

    // Push a PacketA to input_0
    testNode2["input_0"].pushPacket(std::make_shared<PacketA>(), 0);
    EXPECT_TRUE(testNode2.processedA);
    EXPECT_FALSE(testNode2.processedB);

    // Push a PacketB to input_1
    testNode2["input_1"].pushPacket(std::make_shared<PacketB>(), 0);
    EXPECT_TRUE(testNode2.processedB);
}

// Test for Node2 with invalid packet type
TEST_F(TemplateNodeTest, InvalidPacketTypeTest) {
    auto& testNode2 = *pipeline->addNode<TestNode2>();
    testNode2.addInput("input_0");
    testNode2.addInput("input_1");

    EXPECT_TRUE(pipeline->start());

    // Push an invalid packet type (not PacketA or PacketB)
    class InvalidPacket : public IPacket {
    public:
    };

    auto invalidPacket = std::make_shared<InvalidPacket>();
    testNode2["input_0"].pushPacket(invalidPacket, 0);

    // Ensure no processing occurred
    EXPECT_FALSE(testNode2.processedA);
    EXPECT_FALSE(testNode2.processedB);
}

TEST_F(TemplateNodeTest, SharedMemoryNodeTest) {
    auto publisher = pipeline;
    auto& publisherNode = *publisher->addNode<SharedPublisherNode>("shared", 512, 8);
    auto &input = publisherNode.addChannel("channel1");

    auto subscriber = std::make_shared<Pipeline>();
    auto& subscriberNode = *subscriber->addNode<SharedSubscriberNodeT<PacketA>>("shared");
    subscriberNode.addOutput("channel1");

    uint32_t consumedSum = 0;
    auto &consumer = *pipeline->addNode([&consumedSum](std::shared_ptr<IPacket> packet, IPad& pad) -> bool {
        auto packetA = std::dynamic_pointer_cast<PacketA>(packet);
        if (!packetA) {
            std::cerr << "Invalid packet type" << std::endl;
            return false;
        }
        consumedSum += packetA->getData();
        // std::cout << "Packet processed by consumer" << std::endl;
        return true;
    });
    consumer.addInput("input");

    // Connect nodes using pad template method,
    pipeline->connect(subscriberNode["channel1"], consumer["input"]);

    subscriber->start();
    publisher->start();
    for (int i = 1; i < 1000; ++i) {
        auto packet = std::make_shared<PacketA>(i);
        // std::cout << "Pushing packet with data: " << i << std::endl;
        input.pushPacket(packet, 200);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(consumedSum, 499500); // Sum of first 1000 numbers
}

TEST_F(TemplateNodeTest, SharedMemoryPerformanceTest) {
    auto publisher = pipeline;
    const size_t sharedMemorySize = 2048;
    auto& publisherNode = *publisher->addNode<SharedPublisherNode>("shared",
                                sharedMemorySize, std::min<size_t>((size_t)64, sharedMemorySize / sizeof(size_t)));
    auto &input = publisherNode.addChannel("channel1");

    auto subscriber = std::make_shared<Pipeline>();
    auto& subscriberNode = *subscriber->addNode<SharedSubscriberNodeT<PacketA>>("shared");
    subscriberNode.addOutput("channel1");

    uint64_t consumedPackets = 0;
    auto &consumer = *pipeline->addNode([&consumedPackets](std::shared_ptr<IPacket> packet, IPad& pad) -> bool {
        consumedPackets++;
        return true;
    });
    consumer.addInput("input");

    // Connect nodes using pad template method,
    pipeline->connect(subscriberNode["channel1"], consumer["input"]);

    subscriber->start();
    publisher->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // Using high resolution steady clock for better accuracy
    auto startTs = std::chrono::steady_clock::now();
    const uint64_t packetCount = 300000;
    for (uint64_t i = 0; i < packetCount; ++i) {
        auto packet = std::make_shared<PacketA>(i);
        // std::cout << "Pushing packet with data: " << i << std::endl;
        EXPECT_EQ(input.pushPacket(packet, 200), true);
    }
    auto endTs = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(consumedPackets, packetCount); // All packets should be consumed
    auto calculatedPeformance = (consumedPackets * 1000) / std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs).count();
    std::cout << "Packets per second through single shared memory block: "
              << calculatedPeformance << " packets/s" << std::endl;
    EXPECT_GE(calculatedPeformance, 200000);
}
