#include "pipeline/pipeline.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <algorithm>

using namespace lexus2k::pipeline;

class DataPacket: public IPacket {
public:
    explicit DataPacket(const std::string& data) : m_data(data) {}
    const std::string& getData() const { return m_data; }
private:
    std::string m_data;
};

class FileProducer : public INode {
public:
    explicit FileProducer(const std::string& filePath) : m_filePath(filePath) {
        addInput("input");
        m_outputPadIndex = addOutput("output").getIndex();
    }

    void start() noexcept override {
        m_file.open(m_filePath);
        if (!m_file.is_open()) {
            std::cerr << "Error opening file: " << m_filePath << std::endl;
        }
    }

    void stop() noexcept override {
        if (m_file.is_open()) {
            m_file.close();
        }
    }

    void produce() {
        std::string line;
        while (std::getline(m_file, line)) {
            auto packet = std::make_shared<DataPacket>(line);
            (*this)[m_outputPadIndex].pushPacket(packet, 100);
        }
    }

private:
    std::string m_filePath;
    std::ifstream m_file;
    int m_outputPadIndex = 0;
};

class LineReverser : public Node<DataPacket> {
public:
    explicit LineReverser() {
        addInput("input");
        m_outputIndex = addOutput("output").getIndex();
    }

protected:
    bool processPacket(std::shared_ptr<DataPacket> packet, IPad& inputPad) noexcept override {
        auto data = packet->getData();
        std::reverse(data.begin(), data.end());
        auto new_packet = std::make_shared<DataPacket>(data);
        (*this)[m_outputIndex].pushPacket(new_packet, 100);
        return true;
    }

    private:
    int m_outputIndex = 0;
};

class LinePrinter : public Node<DataPacket> {
public:
    explicit LinePrinter(): Node<DataPacket>() {
        addInput("input");
    }

protected:
    bool processPacket(std::shared_ptr<DataPacket> packet, IPad& inputPad) noexcept override {
        std::cout << packet->getData() << std::endl;
        return true;
    }
};

int main() {
    auto pipeline = std::make_shared<Pipeline>();

    // Create producer node
    auto& producer = *pipeline->addNode<FileProducer>("input.txt");

    // Create line reverser node
    auto& reverser = *pipeline->addNode<LineReverser>();

    // Create consumer node
    auto& printer = *pipeline->addNode<LinePrinter>();

    // Connect the nodes
    pipeline->connect(producer["output"], reverser["input"]);
    pipeline->connect(reverser["output"], printer["input"]);

    // Start the pipeline
    pipeline->start();

    // Produce packets
    producer.produce();

    // Stop the pipeline
    pipeline->stop();

    return 0;
}