#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace mini_ros
{

    // ===========================================================
    // FIX: Thêm shutdown() và tryPop() để Executor có thể stop
    // sạch mà không bị deadlock khi worker đang block tại wait().
    // ===========================================================

    template <typename T>
    class ThreadSafeQueue
    {
    public:

        // -----------------------------------------------------------
        // Push item. Nếu queue đã shutdown, bỏ qua silently.
        // -----------------------------------------------------------

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

        // -----------------------------------------------------------
        // Block cho đến khi có item hoặc queue bị shutdown.
        // Trả về std::nullopt khi shutdown để worker thoát sạch.
        // -----------------------------------------------------------

        std::optional<T> waitAndPop()
        {
            std::unique_lock<std::mutex> lock(mutex_);

            condition_.wait(
                lock,
                [this]()
                {
                    return !queue_.empty() || shutdown_;
                });

            if (queue_.empty())
            {
                // shutdown_ == true, không còn item nào
                return std::nullopt;
            }

            T value = std::move(queue_.front());

            queue_.pop();

            return value;
        }

        // -----------------------------------------------------------
        // Non-blocking pop. Trả về nullopt nếu rỗng.
        // -----------------------------------------------------------

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

        // -----------------------------------------------------------
        // Đánh dấu shutdown và wake tất cả worker đang chờ.
        // -----------------------------------------------------------

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
        std::queue<T>           queue_;

        mutable std::mutex      mutex_;

        std::condition_variable condition_;

        bool                    shutdown_{ false };
    };

}
