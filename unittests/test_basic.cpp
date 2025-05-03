#include <gtest/gtest.h>
#include "pipeline/pipeline.h"
#include <memory>

using namespace lexus2k::pipeline;

class PipelineTest : public ::testing::Test
{
protected:
    std::shared_ptr<Pipeline> pipeline;

    void SetUp() override
    {
        pipeline = std::make_shared<Pipeline>();
    }

    void TearDown() override
    {
        pipeline.reset();
    }
};

TEST_F(PipelineTest, BasicUsage)
{
    bool consumed = false;

    auto &producer = *pipeline->addNode([](std::shared_ptr<IPacket> packet, IPad& pad) -> bool {
        std::cout << "Packet processed by processor" << std::endl;
        pad.node()["output"].pushPacket(packet, 0);
        return true;
    });
    producer.addInput("input");
    producer.addOutput("output");

    auto &consumer = *pipeline->addNode([&consumed](std::shared_ptr<IPacket> packet, IPad& pad) -> bool {
        consumed = true;
        std::cout << "Packet processed by consumer" << std::endl;
        return true;
    });
    consumer.addInput("input");

    pipeline->connect(producer["output"], consumer["input"]);

    EXPECT_TRUE(pipeline->start());

    producer["input"].pushPacket(std::make_shared<IPacket>(), 0);

    EXPECT_TRUE(consumed);
}

TEST_F(PipelineTest, ConnectTestUsingThen)
{
    bool consumed = false;
    auto &producer = *pipeline->addNode([](std::shared_ptr<IPacket> packet, IPad& pad) -> bool {
        std::cout << "Packet processed by producer" << std::endl;
        pad.node()["output"].pushPacket(packet, 0);
        return true;
    });
    producer.addInput("input");
    producer.addOutput("output");

    auto &processor = *pipeline->addNode([](std::shared_ptr<IPacket> packet, IPad& pad) -> bool {
        std::cout << "Packet processed by processor" << std::endl;
        pad.node()["output"].pushPacket(packet, 0);
        return true;
    });
    processor.addInput<QueuePad>("input");
    processor.addOutput("output");

    auto &consumer = *pipeline->addNode([&consumed](std::shared_ptr<IPacket> packet, IPad& pad) -> bool {
        consumed = true;
        std::cout << "Packet processed by consumer" << std::endl;
        return true;
    });
    consumer.addInput("input");

    // Connect nodes using pad template method,
    // In this case pads are created automatically if they don't exist
    producer["output"]
        .then(processor["input"])["output"]
        .then(consumer["input"]);

    EXPECT_TRUE(pipeline->start());

    producer["input"].pushPacket(std::make_shared<IPacket>(), 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(consumed);
}

TEST_F(PipelineTest, TeeNodeTest) {
    bool consumed1 = false;
    bool consumed2 = false;

    auto &producer = *pipeline->addNode([](std::shared_ptr<IPacket> packet, IPad& pad) {
        std::cout << "Packet processed by producer" << std::endl;
        pad.node()["output"].pushPacket(packet, 0);
        return true;
    });
    producer.addInput("input");
    producer.addOutput("output");

    auto &tee = *pipeline->addNode<Splitter<2, SimplePad>>();

    auto &consumer1 = *pipeline->addNode([&consumed1](std::shared_ptr<IPacket> packet, IPad& pad) {
        consumed1 = true;
        std::cout << "Packet processed by consumer1" << std::endl;
        return true;
    });
    consumer1.addInput("input");

    auto &consumer2 = *pipeline->addNode([&consumed2](std::shared_ptr<IPacket> packet, IPad& pad) {
        consumed2 = true;
        std::cout << "Packet processed by consumer2" << std::endl;
        return true;
    });
    consumer2.addInput("input");

    pipeline->connect(producer["output"], tee["input"]);
    pipeline->connect(tee["output_1"], consumer1["input"]);
    pipeline->connect(tee["output_2"], consumer2["input"]);

    EXPECT_TRUE(pipeline->start());

    producer["input"].pushPacket(std::make_shared<IPacket>(), 0);

    EXPECT_TRUE(consumed1);
    EXPECT_TRUE(consumed2);
}