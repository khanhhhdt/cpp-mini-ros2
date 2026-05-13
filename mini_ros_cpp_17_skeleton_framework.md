# MiniROS — C++17 Mini Robotics Middleware Skeleton

Mục tiêu:
- học kiến trúc middleware robotics kiểu ROS2
- pub/sub
- executor
- timer
- multithreading
- service skeleton
- modular architecture

---

# 1. Project Structure

```text
mini_ros/
│
├── CMakeLists.txt
├── app/
│   └── main.cpp
│
├── include/
│   └── mini_ros/
│       ├── core/
│       │   ├── node.hpp
│       │   ├── publisher.hpp
│       │   ├── subscriber.hpp
│       │   ├── executor.hpp
│       │   ├── timer.hpp
│       │   └── types.hpp
│       │
│       ├── topic/
│       │   └── topic_manager.hpp
│       │
│       ├── utils/
│       │   └── thread_safe_queue.hpp
│       │
│       └── messages/
│           └── laser_scan.hpp
│
├── src/
│   ├── node.cpp
│   ├── executor.cpp
│   ├── topic_manager.cpp
│   └── timer.cpp
│
└── examples/
    └── simple_pub_sub.cpp
```

---

# 2. Thread Safe Queue

## include/mini_ros/utils/thread_safe_queue.hpp

```cpp
#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

namespace mini_ros
{

template<typename T>
class ThreadSafeQueue
{
public:

    void push(const T& value)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(value);
        }

        condition_.notify_one();
    }

    T waitAndPop()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        condition_.wait(lock,
            [this]()
            {
                return !queue_.empty();
            });

        T value = queue_.front();
        queue_.pop();

        return value;
    }

private:

    std::queue<T> queue_;

    std::mutex mutex_;

    std::condition_variable condition_;
};

}
```

---

# 3. Core Types

## include/mini_ros/core/types.hpp

```cpp
#pragma once

#include <functional>

namespace mini_ros
{

using Task = std::function<void()>;

}
```

---

# 4. Executor

Executor là trái tim framework.

## include/mini_ros/core/executor.hpp

```cpp
#pragma once

#include <atomic>
#include <thread>

#include "mini_ros/utils/thread_safe_queue.hpp"
#include "mini_ros/core/types.hpp"

namespace mini_ros
{

class Executor
{
public:

    static Executor& instance();

    void post(const Task& task);

    void spin();

    void stop();

private:

    Executor() = default;

    ThreadSafeQueue<Task> tasks_;

    std::atomic<bool> running_ {true};
};

}
```

---

## src/executor.cpp

```cpp
#include "mini_ros/core/executor.hpp"

namespace mini_ros
{

Executor& Executor::instance()
{
    static Executor executor;

    return executor;
}

void Executor::post(const Task& task)
{
    tasks_.push(task);
}

void Executor::spin()
{
    while(running_)
    {
        auto task = tasks_.waitAndPop();

        task();
    }
}

void Executor::stop()
{
    running_ = false;
}

}
```

---

# 5. Topic Manager

Message broker của framework.

## include/mini_ros/topic/topic_manager.hpp

```cpp
#pragma once

#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace mini_ros
{

class TopicManager
{
public:

    static TopicManager& instance();

    template<typename T>
    void subscribe(
        const std::string& topic,
        std::function<void(const T&)> callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto wrapper =
            [callback](std::shared_ptr<void> data)
            {
                callback(*std::static_pointer_cast<T>(data));
            };

        subscribers_[topic].push_back(wrapper);
    }

    template<typename T>
    void publish(const std::string& topic,
                 const T& message)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto data = std::make_shared<T>(message);

        auto it = subscribers_.find(topic);

        if(it == subscribers_.end())
        {
            return;
        }

        for(auto& subscriber : it->second)
        {
            subscriber(data);
        }
    }

private:

    TopicManager() = default;

    using Callback =
        std::function<void(std::shared_ptr<void>)>;

    std::unordered_map<
        std::string,
        std::vector<Callback>> subscribers_;

    std::mutex mutex_;
};

}
```

---

## src/topic_manager.cpp

```cpp
#include "mini_ros/topic/topic_manager.hpp"

namespace mini_ros
{

TopicManager& TopicManager::instance()
{
    static TopicManager manager;

    return manager;
}

}
```

---

# 6. Publisher

## include/mini_ros/core/publisher.hpp

```cpp
#pragma once

#include <string>

#include "mini_ros/topic/topic_manager.hpp"

namespace mini_ros
{

template<typename T>
class Publisher
{
public:

    Publisher(const std::string& topic)
        : topic_(topic)
    {
    }

    void publish(const T& message)
    {
        TopicManager::instance()
            .publish<T>(topic_, message);
    }

private:

    std::string topic_;
};

}
```

---

# 7. Subscriber

## include/mini_ros/core/subscriber.hpp

```cpp
#pragma once

namespace mini_ros
{

template<typename T>
class Subscriber
{
};

}
```

---

# 8. Node

## include/mini_ros/core/node.hpp

```cpp
#pragma once

#include <string>
#include <functional>

#include "mini_ros/core/publisher.hpp"
#include "mini_ros/topic/topic_manager.hpp"
#include "mini_ros/core/executor.hpp"

namespace mini_ros
{

class Node
{
public:

    explicit Node(const std::string& name)
        : name_(name)
    {
    }

    template<typename T>
    Publisher<T> createPublisher(
        const std::string& topic)
    {
        return Publisher<T>(topic);
    }

    template<typename T>
    void createSubscriber(
        const std::string& topic,
        std::function<void(const T&)> callback)
    {
        TopicManager::instance()
            .subscribe<T>(
                topic,
                [callback](const T& msg)
                {
                    Executor::instance().post(
                        [callback, msg]()
                        {
                            callback(msg);
                        });
                });
    }

private:

    std::string name_;
};

}
```

---

# 9. Timer

## include/mini_ros/core/timer.hpp

```cpp
#pragma once

#include <thread>
#include <chrono>
#include <functional>
#include <atomic>

namespace mini_ros
{

class Timer
{
public:

    Timer(
        std::chrono::milliseconds interval,
        std::function<void()> callback)
    {
        thread_ = std::thread(
            [=]()
            {
                while(running_)
                {
                    std::this_thread::sleep_for(interval);

                    callback();
                }
            });
    }

    ~Timer()
    {
        running_ = false;

        if(thread_.joinable())
        {
            thread_.join();
        }
    }

private:

    std::thread thread_;

    std::atomic<bool> running_ {true};
};

}
```

---

# 10. Message Definition

## include/mini_ros/messages/laser_scan.hpp

```cpp
#pragma once

#include <vector>

namespace mini_ros
{

struct LaserScan
{
    std::vector<float> ranges;
};

}
```

---

# 11. Example Application

## examples/simple_pub_sub.cpp

```cpp
#include <iostream>
#include <thread>

#include "mini_ros/core/node.hpp"
#include "mini_ros/core/timer.hpp"
#include "mini_ros/core/executor.hpp"
#include "mini_ros/messages/laser_scan.hpp"

using namespace mini_ros;

int main()
{
    Node lidarNode("lidar_node");

    Node mappingNode("mapping_node");

    auto publisher =
        lidarNode.createPublisher<LaserScan>("/scan");

    mappingNode.createSubscriber<LaserScan>(
        "/scan",
        [](const LaserScan& scan)
        {
            std::cout
                << "Received scan size: "
                << scan.ranges.size()
                << std::endl;
        });

    Timer timer(
        std::chrono::milliseconds(100),
        [&]()
        {
            LaserScan scan;

            scan.ranges =
            {
                1.0f,
                2.0f,
                3.0f
            };

            publisher.publish(scan);
        });

    Executor::instance().spin();

    return 0;
}
```

---

# 12. CMake

## CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)

project(mini_ros)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

file(GLOB_RECURSE SRC_FILES
    src/*.cpp)

add_executable(
    mini_ros_app
    app/main.cpp
    ${SRC_FILES})

add_executable(
    simple_pub_sub
    examples/simple_pub_sub.cpp
    ${SRC_FILES})
```

---

# 13. Event Flow

```text
Publisher.publish()
        |
        v
TopicManager
        |
        v
Executor.post(task)
        |
        v
ThreadSafeQueue
        |
        v
Executor.spin()
        |
        v
Subscriber callback
```

---

# 14. Hướng nâng cấp tiếp theo

## V2

### MultiThreadedExecutor

```text
Worker Threads
      |
Task Queue
```

---

## V3

### QoS

- queue depth
- reliable
- best effort

---

## V4

### Zero Copy Transport

Dùng:

```cpp
std::shared_ptr<Message>
```

---

## V5

### TCP Distributed Node

```text
Process A <----TCP----> Process B
```

---

## V6

### Service Architecture

```text
Client -> Request
Server -> Response
```

---

## V7

### Lifecycle Node

```text
UNCONFIGURED
INACTIVE
ACTIVE
FINALIZED
```

---

# 15. Architecture Lessons

Khi tự build framework này bạn sẽ học:

- middleware design
- event-driven architecture
- realtime threading
- distributed systems
- robotics software architecture
- callback scheduling
- executor model
- message routing
- synchronization
- lock contention
- performance bottleneck

---

# 16. Điều QUAN TRỌNG nhất

Đừng cố clone ROS2.

Hãy:

- architecture clean
- module nhỏ
- dễ debug
- hiểu sâu từng layer
- incremental evolution

Framework nhỏ nhưng clean sẽ giúp bạn hiểu ROS2 nhanh hơn rất nhiều.

