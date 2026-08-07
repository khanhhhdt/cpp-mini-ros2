#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace mini_ros
{
    template <typename T>
    class ThreadSafeQueue
    {
    public:
        void push(const T &value)
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);

                if (shutdown_)
                {
                    return;
                }

                queue_.push(value);
            }

            condition_.notify_one();
        }

        std::optional<T> waitAndPop()
        {
            std::unique_lock<std::mutex> lock(mutex_);

            condition_.wait(lock, [this]()
                            { return !queue_.empty() || shutdown_; });

            if (queue_.empty())
            {
                // shutdown_ == true, không còn item nào
                return std::nullopt;
            }

            T value = std::move(queue_.front());

            queue_.pop();

            return value;
        }

        std::optional<T> tryPop()
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (queue_.empty())
            {
                return std::nullopt;
            }

            T value = std::move(queue_.front());

            queue_.pop();

            return value;
        }

        void shutdown()
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);

                shutdown_ = true;
            }

            condition_.notify_all();
        }

        bool isShutdown() const
        {
            std::lock_guard<std::mutex> lock(mutex_);

            return shutdown_;
        }

        size_t size() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }

    private:
        std::queue<T> queue_;
        mutable std::mutex mutex_;
        std::condition_variable condition_;
        bool shutdown_{false};
    };

}
