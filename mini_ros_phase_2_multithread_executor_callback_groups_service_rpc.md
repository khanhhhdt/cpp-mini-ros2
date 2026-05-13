# Mini ROS Phase 2

Triển khai:

1. MultiThreadedExecutor
2. Callback Groups
3. Service / Client RPC

Framework hiện tại của bạn đã có:
- async pub/sub
- lifetime safe callback
- shared_ptr<const T>
- subscription handle

Phase 2 sẽ biến framework thành:

```text
mini middleware runtime
```

---

# 1. MultiThreadedExecutor

## Ý tưởng

Thay vì:

```text
1 thread xử lý callback
```

Ta dùng:

```text
thread pool
```

---

# FILE: include/mini_ros/executor.h

```cpp
#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include <iostream>

#include "thread_safe_queue.h"
#include "types.h"

namespace mini_ros
{

class Executor
{
public:

    static Executor& instance()
    {
        static Executor executor;

        return executor;
    }

    // =============================================
    // START THREAD POOL
    // =============================================

    void start(size_t threadCount = std::thread::hardware_concurrency())
    {
        if(threadCount == 0)
        {
            threadCount = 4;
        }

        running_ = true;

        for(size_t i = 0; i < threadCount; ++i)
        {
            workers_.emplace_back(
                [this, i]()
                {
                    workerLoop(i);
                });
        }

        std::cout
            << "[Executor] Started with "
            << threadCount
            << " threads"
            << std::endl;
    }

    // =============================================
    // POST TASK
    // =============================================

    void post(const Task& task)
    {
        tasks_.push(task);
    }

    // =============================================
    // STOP
    // =============================================

    void stop()
    {
        running_ = false;

        // wake all workers
        for(size_t i = 0; i < workers_.size(); ++i)
        {
            tasks_.push([](){});
        }

        for(auto& worker : workers_)
        {
            if(worker.joinable())
            {
                worker.join();
            }
        }

        workers_.clear();
    }

private:

    Executor() = default;

    void workerLoop(size_t id)
    {
        while(running_)
        {
            auto task = tasks_.waitAndPop();

            task();
        }
    }

private:

    ThreadSafeQueue<Task> tasks_;

    std::vector<std::thread> workers_;

    std::atomic<bool> running_ {false};
};

}
```

---

# 2. Callback Groups

## Problem

Multi-thread executor gây:

```text
race condition
```

---

# Solution

Cho phép callback chạy theo policy.

---

# FILE: include/mini_ros/callback_group.h

```cpp
#pragma once

#include <memory>
#include <mutex>

namespace mini_ros
{

enum class CallbackGroupType
{
    MutuallyExclusive,
    Reentrant
};

class CallbackGroup
{
public:

    explicit CallbackGroup(
        CallbackGroupType type)
        : type_(type)
    {
    }

    CallbackGroupType type() const
    {
        return type_;
    }

    std::mutex& mutex()
    {
        return mutex_;
    }

private:

    CallbackGroupType type_;

    std::mutex mutex_;
};

using CallbackGroupPtr =
    std::shared_ptr<CallbackGroup>;

}
```

---

# Node API

## FILE: include/mini_ros/node.h

Thêm:

```cpp
CallbackGroupPtr createCallbackGroup(
    CallbackGroupType type)
{
    return std::make_shared<CallbackGroup>(type);
}
```

---

# Subscriber integration

Trong createSubscriber():

```cpp
SubscriptionPtr createSubscriber(
    const std::string& topic,
    CallbackGroupPtr group,
    Callback&& callback)
```

---

# Generic Callback Version

```cpp
template<
    typename T,
    typename Callback>
SubscriptionPtr createSubscriber(
    const std::string& topic,
    CallbackGroupPtr group,
    Callback&& callback)
{
    auto subscription =
        std::make_shared<Subscription>();

    auto wrapper =
        [group,
         cb = std::forward<Callback>(callback)]
        (std::shared_ptr<const T> msg) mutable
        {
            if(group &&
               group->type() ==
               CallbackGroupType::MutuallyExclusive)
            {
                std::lock_guard<std::mutex>
                    lock(group->mutex());

                cb(msg);
            }
            else
            {
                cb(msg);
            }
        };

    TopicManager::instance()
        .subscribe<T>(
            topic,
            wrapper,
            subscription);

    return subscription;
}
```

---

# Ý nghĩa

## MutuallyExclusive

```text
only 1 callback chạy cùng lúc
```

---

## Reentrant

```text
parallel allowed
```

---

# 3. Service / Client RPC

---

# Ý tưởng

Pub/Sub:

```text
fire and forget
```

Service:

```text
request -> response
```

---

# FILE: include/mini_ros/service_manager.h

```cpp
#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace mini_ros
{

class ServiceManager
{
public:

    static ServiceManager& instance()
    {
        static ServiceManager manager;

        return manager;
    }

    template<typename Req, typename Res>
    void registerService(
        const std::string& name,
        std::function<void(
            std::shared_ptr<const Req>,
            std::shared_ptr<Res>)> callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto wrapper =
            [callback]
            (
                std::shared_ptr<const void> req,
                std::shared_ptr<void> res)
            {
                callback(
                    std::static_pointer_cast<const Req>(req),
                    std::static_pointer_cast<Res>(res));
            };

        services_[name] = wrapper;
    }

    template<typename Req, typename Res>
    std::shared_ptr<Res> call(
        const std::string& name,
        std::shared_ptr<const Req> request)
    {
        ServiceCallback callback;

        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = services_.find(name);

            if(it == services_.end())
            {
                return nullptr;
            }

            callback = it->second;
        }

        auto response =
            std::make_shared<Res>();

        callback(request, response);

        return response;
    }

private:

    using ServiceCallback =
        std::function<void(
            std::shared_ptr<const void>,
            std::shared_ptr<void>)>;

    std::unordered_map<
        std::string,
        ServiceCallback> services_;

    std::mutex mutex_;
};

}
```

---

# FILE: include/mini_ros/service.h

```cpp
#pragma once

#include <memory>
#include <string>

#include "service_manager.h"

namespace mini_ros
{

template<typename Req, typename Res>
class Service
{
public:

    Service(const std::string& name)
        : name_(name)
    {
    }

private:

    std::string name_;
};

}
```

---

# FILE: include/mini_ros/client.h

```cpp
#pragma once

#include <memory>
#include <string>

#include "service_manager.h"

namespace mini_ros
{

template<typename Req, typename Res>
class Client
{
public:

    Client(const std::string& name)
        : name_(name)
    {
    }

    std::shared_ptr<Res> call(
        std::shared_ptr<const Req> request)
    {
        return ServiceManager::instance()
            .call<Req, Res>(
                name_,
                request);
    }

private:

    std::string name_;
};

}
```

---

# NODE INTEGRATION

## FILE: include/mini_ros/node.h

Thêm:

```cpp
#include "service.h"
#include "client.h"
#include "service_manager.h"
```

---

# createService

```cpp
template<
    typename Req,
    typename Res,
    typename Callback>
void createService(
    const std::string& name,
    Callback&& callback)
{
    ServiceManager::instance()
        .registerService<Req, Res>(
            name,
            std::forward<Callback>(callback));
}
```

---

# createClient

```cpp
template<typename Req, typename Res>
Client<Req, Res> createClient(
    const std::string& name)
{
    return Client<Req, Res>(name);
}
```

---

# EXAMPLE MESSAGE

## FILE: include/mini_ros/service_messages.h

```cpp
#pragma once

namespace mini_ros
{

struct AddTwoIntsRequest
{
    int a;
    int b;
};

struct AddTwoIntsResponse
{
    int sum;
};

}
```

---

# MAIN EXAMPLE

## FILE: src/main.cpp

```cpp
#include <iostream>

#include "mini_ros/node.h"
#include "mini_ros/executor.h"
#include "mini_ros/callback_group.h"
#include "mini_ros/service_messages.h"

using namespace mini_ros;

int main()
{
    Node node("main_node");

    // =========================================
    // START MULTI THREAD EXECUTOR
    // =========================================

    Executor::instance().start(4);

    // =========================================
    // CALLBACK GROUPS
    // =========================================

    auto exclusiveGroup =
        node.createCallbackGroup(
            CallbackGroupType::MutuallyExclusive);

    auto reentrantGroup =
        node.createCallbackGroup(
            CallbackGroupType::Reentrant);

    // =========================================
    // SUBSCRIBER
    // =========================================

    auto sub =
        node.createSubscriber<LaserScan>(
            "/scan",
            exclusiveGroup,
            [](std::shared_ptr<const LaserScan> scan)
            {
                std::cout
                    << "[SCAN CALLBACK] thread="
                    << std::this_thread::get_id()
                    << " size="
                    << scan->ranges.size()
                    << std::endl;
            });

    // =========================================
    // SERVICE SERVER
    // =========================================

    node.createService<
        AddTwoIntsRequest,
        AddTwoIntsResponse>(
        "/add_two_ints",
        [](std::shared_ptr<const AddTwoIntsRequest> req,
           std::shared_ptr<AddTwoIntsResponse> res)
        {
            res->sum = req->a + req->b;

            std::cout
                << "[SERVICE] "
                << req->a
                << " + "
                << req->b
                << " = "
                << res->sum
                << std::endl;
        });

    // =========================================
    // CLIENT
    // =========================================

    auto client =
        node.createClient<
            AddTwoIntsRequest,
            AddTwoIntsResponse>(
            "/add_two_ints");

    auto request =
        std::make_shared<AddTwoIntsRequest>();

    request->a = 10;
    request->b = 20;

    auto response =
        client.call(request);

    std::cout
        << "[CLIENT RESPONSE] "
        << response->sum
        << std::endl;

    while(true)
    {
        std::this_thread::sleep_for(
            std::chrono::seconds(1));
    }

    return 0;
}
```

---

# Kiến trúc sau Phase 2

Framework của bạn đã có:

```text
Executor Runtime
Thread Pool
Callback Scheduling
Callback Groups
Pub/Sub
Service RPC
Lifetime Safety
Shared Message Ownership
```

---

# Framework lúc này đã bắt đầu giống:

- ROS2 executor
- Boost.Asio runtime
- DDS middleware simplified
- distributed robotics runtime

---

# Bước tiếp theo cực hợp lý

## Phase 3

1. Bounded queue
2. QoS
3. Futures/async service
4. TCP transport
5. Node graph
6. Topic discovery
7. Serialization layer
8. Intra-process zero-copy

