# README for the Pipeline Project

## Overview

The **Pipeline Library** is a lightweight and flexible C++ library designed to organize and manage **data processing pipelines**. A data pipeline is a sequence of steps where data flows through various stages of processing. Each stage processes the data and passes it to the next stage, enabling modular and reusable data processing workflows.

This library provides a framework for building pipelines with **nodes** and **pads**, where:
- **Nodes** represent processing units that perform specific tasks on the data.
- **Pads** connect nodes and facilitate the flow of data between them.

The library is designed to be **extensible**, **efficient**, and **easy to use**, making it suitable for real-time data processing, batch processing, and other scenarios.

---

## Features

- **Modular Design**: Build pipelines by connecting reusable nodes.
- **Customizable Nodes**: Create custom nodes to handle specific data processing tasks.
- **Flexible Pads**: Use different types of pads (e.g., `SimplePad`, `QueuePad`) to control data flow.
- **Type-Safe Processing**: Leverage C++ templates to ensure type safety for data packets.
- **Real-Time Processing**: Support for real-time data pipelines with minimal latency.
- **Unit Testing Support**: Includes unit tests using Google Test for easy validation.

---

## Project Structure

- `src/`: Contains the source files for the library.
- `include/`: Contains the public headers for the library.
- `unittests/`: Contains unit tests for the library using Google Test.
- `CMakeLists.txt`: The CMake configuration file for building the project.
- `Makefile`: An alternative build system for building the library and running tests.

---

## Building the Library

### Using CMake
1. Navigate to the project directory:
   ```bash
   cd pipeline-1
   ```
2. Create a build directory:
   ```bash
   mkdir build && cd build
   ```
3. Run CMake to configure the project:
   ```bash
   cmake ..
   ```
4. Build the library:
   ```bash
   make
   ```

### Using Makefile
Simply run the following command in the project root:
```bash
make
```

---

## Building the Library with Unit Tests

### Using CMake
1. Navigate to the project directory:
   ```bash
   cd pipeline-1
   ```
2. Create a build directory:
   ```bash
   mkdir build && cd build
   ```
3. Enable the `BUILD_TESTS` option when running CMake:
   ```bash
   cmake .. -DBUILD_TESTS=ON
   ```
4. Build the library and tests:
   ```bash
   make
   ```
5. Run the tests:
   ```bash
   ./unittests/test_pipeline
   ```

### Using Makefile
Run the following command in the project root:
```bash
make test
```

This will build the library and run the unit tests.

---

## Installing the Built Library

### Using CMake
1. After building the library, install it to the default location (`/usr/local`):
   ```bash
   cmake --install build
   ```
2. To install to a custom location, specify the `CMAKE_INSTALL_PREFIX` during configuration:
   ```bash
   cmake .. -DCMAKE_INSTALL_PREFIX=/custom/install/path
   cmake --install build
   ```

### Using Makefile
Run the following command in the project root:
```bash
make install
```

By default, the library and headers will be installed to:
- Libraries: `/usr/local/lib`
- Headers: `/usr/local/include`

To install to a custom location, override the `DESTDIR` variable:
```bash
make install DESTDIR=/custom/install/path
```

---

## Usage Examples

### Example 1: Basic Pipeline with a Producer and Consumer

This example demonstrates a simple pipeline with a producer node that generates data and a consumer node that processes it.

```cpp
#include "pipeline/pipeline.h"
#include "pipeline/pipeline_nodes.h"
#include "pipeline/pipeline_pads.h"
#include <iostream>
#include <memory>

using namespace lexus2k::pipeline;

int main() {
    auto pipeline = std::make_shared<Pipeline>();

    // Create a producer node
    auto& producer = pipeline->addNode<LambdaNode>([](std::shared_ptr<IPacket> packet, IPad* pad) {
        std::cout << "Producer generated a packet" << std::endl;
        pad->getParent()->pad<SimplePad>("output").pushPacket(packet, 0);
    });
    producer.addPad<SimplePad>("output");

    // Create a consumer node
    auto& consumer = pipeline->addNode<LambdaNode>([](std::shared_ptr<IPacket> packet, IPad* pad) {
        std::cout << "Consumer processed a packet" << std::endl;
    });
    consumer.addPad<SimplePad>("input");

    // Connect the producer to the consumer
    pipeline->connect(producer["output"], consumer["input"]);

    // Start the pipeline
    pipeline->start();

    // Push a packet to the producer
    producer["output"].pushPacket(std::make_shared<IPacket>(), 0);

    return 0;
}
```

### Example 2: Using `QueuePad` for Buffered Processing

This example demonstrates how to use a `QueuePad` to buffer packets before processing.

```cpp
#include "pipeline/pipeline.h"
#include "pipeline/pipeline_nodes.h"
#include "pipeline/pipeline_pads.h"
#include <iostream>
#include <memory>

using namespace lexus2k::pipeline;

int main() {
    auto pipeline = std::make_shared<Pipeline>();

    // Create a producer node
    auto& producer = pipeline->addNode<LambdaNode>([](std::shared_ptr<IPacket> packet, IPad* pad) {
        std::cout << "Producer generated a packet" << std::endl;
        pad->getParent()->pad<QueuePad>("output").pushPacket(packet, 0);
    });
    producer.addPad<QueuePad>("output");

    // Create a consumer node
    auto& consumer = pipeline->addNode<LambdaNode>([](std::shared_ptr<IPacket> packet, IPad* pad) {
        std::cout << "Consumer processed a packet" << std::endl;
    });
    consumer.addPad<SimplePad>("input");

    // Connect the producer to the consumer
    pipeline->connect(producer["output"], consumer["input"]);

    // Start the pipeline
    pipeline->start();

    // Push multiple packets to the producer
    for (int i = 0; i < 5; ++i) {
        producer["output"].pushPacket(std::make_shared<IPacket>(), 0);
    }

    return 0;
}
```

---

## License

This project is licensed under the MIT License. See the LICENSE file for more details.