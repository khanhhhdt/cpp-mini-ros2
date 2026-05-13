# mini_ros Architecture Design Document

---

# 1. Introduction

## Goal

`mini_ros` is a lightweight robotics middleware framework written in modern C++17.

The project is designed for:

- learning robotics software architecture
- understanding ROS2 core concepts
- experimenting with SLAM/navigation systems
- building concurrent event-driven systems
- integrating with Qt visualization

---

# 2. Design Philosophy

The framework follows several important architectural principles.

---

## 2.1 Runtime-centric Architecture

The runtime is the center of the system.

UI frameworks such as Qt are treated only as visualization layers.

```text
mini_ros = backend runtime
Qt       = frontend visualization
```

---

## 2.2 Message-driven System

All communication is based on asynchronous message passing.

Modules do not directly call each other.

Communication happens through:

- topics
- services
- events

---

## 2.3 Concurrent Execution

The framework is designed as a multi-threaded event runtime.

Callbacks are executed asynchronously by worker threads.

---

## 2.4 Modular Robotics Graph

Each robotics module is implemented as a node.

Examples:

- lidar driver
- SLAM
- mapping
- navigation
- visualization

---

# 3. High-Level Architecture

```text
+------------------------------------------------+
|                    Qt UI                       |
|      QML / Widgets / OpenGL / Visualization    |
+------------------------------------------------+
                    ^
                    |
              Qt Bridge Layer
                    |
+------------------------------------------------+
|                 mini_ros Runtime               |
|                                                |
|  +------------------------------------------+  |
|  |               Executor                   |  |
|  |        Multi-thread Task Scheduler       |  |
|  +------------------------------------------+  |
|                                                |
|  +------------------------------------------+  |
|  |             Topic Manager                |  |
|  |       Publish / Subscribe Routing        |  |
|  +------------------------------------------+  |
|                                                |
|  +------------------------------------------+  |
|  |             Service Manager              |  |
|  |            RPC Communication             |  |
|  +------------------------------------------+  |
|                                                |
|  +------------------------------------------+  |
|  |              Callback Groups             |  |
|  |     Concurrency Control Mechanism        |  |
|  +------------------------------------------+  |
+------------------------------------------------+
```

---

# 4. Core Components

---

# 4.1 Node

## Description

A node is the fundamental execution unit.

Each robotics module is represented as a node.

---

## Responsibilities

- create publishers
- create subscribers
- create services
- create clients
- create callback groups

---

## Example

```cpp
Node slamNode("slam");
```

---

# 4.2 Publisher

## Description

A publisher sends messages to a topic.

---

## Example

```cpp
auto pub =
    node.createPublisher<LaserScan>(
        "/scan");
```

---

## Responsibilities

- publish shared_ptr messages
- dispatch to subscribers
- async message routing

---

# 4.3 Subscriber

## Description

A subscriber receives topic messages asynchronously.

---

## Supported Callback Types

- lambda
- capture lambda
- free function
- functor
- member function

---

## Example

```cpp
node.createSubscriber<LaserScan>(
    "/scan",
    group,
    callback);
```

---

# 4.4 Executor

## Description

The executor is the runtime scheduler.

It executes callbacks using worker threads.

---

## Architecture

```text
Publisher
    ->
TopicManager
    ->
Executor Queue
    ->
Worker Thread
    ->
Callback Execution
```

---

## Responsibilities

- task queue
- thread pool
- callback dispatch
- synchronization

---

## Example

```cpp
Executor::instance().start(4);
```

---

# 4.5 Callback Groups

## Description

Callback groups control concurrency behavior.

Inspired by ROS2 callback groups.

---

## Types

### Reentrant

Allows parallel execution.

```text
Callback A
Callback B
can run simultaneously
```

---

### MutuallyExclusive

Only one callback runs at a time.

Used for:

- shared state
- non-thread-safe resources
- critical sections

---

## Example

```cpp
auto group =
    node.createCallbackGroup(
        CallbackGroupType::MutuallyExclusive);
```

---

# 4.6 Services / Clients

## Description

Implements RPC-style communication.

---

## Architecture

```text
Client
    ->
Request
    ->
Service Callback
    ->
Response
    ->
Client
```

---

## Example

### Service

```cpp
node.createService<
    AddTwoIntsRequest,
    AddTwoIntsResponse>(
        "/add",
        callback);
```

---

### Client

```cpp
auto response =
    client.call(request);
```

---

# 4.7 Timer

## Description

Periodic task execution.

---

## Example

```cpp
Timer timer(
    std::chrono::milliseconds(100),
    callback);
```

---

## Use Cases

- sensor simulation
- periodic publish
- control loop
- heartbeat

---

# 5. Message System

---

# 5.1 Shared Ownership

All messages use:

```cpp
std::shared_ptr<T>
```

Benefits:

- zero-copy style passing
- safe async lifetime
- reduced memory duplication

---

# 5.2 Message Header

Messages include:

```cpp
struct Header
{
    uint64_t timestamp;

    std::string frameId;
};
```

---

## Purpose

- synchronization
- sensor timing
- coordinate frames
- SLAM alignment

---

# 6. SLAM Pipeline Example

## Architecture

```text
FakeLidarNode
    |
    v
/scan
    |
    v
SlamNode
    |
    +--> /pose
    |
    +--> /map
    |
    v
VisualizationNode
```

---

# 7. Qt Integration Strategy

## Philosophy

Qt should only handle:

- visualization
- rendering
- user interaction
- dashboard UI

mini_ros handles:

- scheduling
- SLAM
- navigation
- sensor processing
- runtime execution

---

## Thread Boundary

Qt UI objects are not thread-safe.

mini_ros worker threads must never directly modify Qt widgets.

---

## Recommended Architecture

```text
mini_ros callback thread
    ->
Qt Bridge
    ->
Qt QueuedConnection
    ->
GUI Thread
```

---

# 8. Concurrency Model

## Thread Types

### Executor Worker Threads

Responsible for:

- callback execution
- topic dispatch
- service execution

---

### Timer Threads

Responsible for:

- periodic wake-up
- sensor simulation
- periodic publishing

---

### Qt GUI Thread

Responsible for:

- rendering
- widgets
- OpenGL
- user interaction

---

# 9. Current Features

| Feature | Status |
|---|---|
| Publish / Subscribe | Implemented |
| Multi-thread Executor | Implemented |
| Callback Groups | Implemented |
| Services / Clients | Implemented |
| Timers | Implemented |
| Shared_ptr Messaging | Implemented |
| Lifetime-safe Callbacks | Implemented |
| SLAM Simulation | Implemented |
| Qt-ready Architecture | Implemented |

---

# 10. Future Roadmap

## Planned Features

### Middleware

- bounded queues
- QoS
- serialization
- TCP transport
- UDP transport
- node discovery

---

### Robotics

- TF transform system
- occupancy mapping
- ICP scan matching
- localization
- navigation stack
- path planning

---

### Performance

- lock-free queue
- zero-copy transport
- executor affinity
- memory pool allocator

---

### Visualization

- Qt OpenGL viewer
- occupancy grid renderer
- laser scan renderer
- robot pose visualization

---

# 11. Architectural Goals

The long-term goal of mini_ros is to evolve into:

```text
A lightweight robotics operating middleware
```

with:

- modular robotics graph
- distributed communication
- concurrent runtime execution
- real-time robotics processing
- Qt visualization integration

---

# 12. Summary

mini_ros is no longer a simple C++ exercise.

The framework now contains the core architectural concepts behind:

- ROS2
- DDS middleware
- robotics runtime systems
- concurrent event-driven frameworks

The project serves as:

- a robotics software architecture learning platform
- a SLAM experimentation runtime
- a concurrent systems engineering project
- a Qt robotics integration platform

