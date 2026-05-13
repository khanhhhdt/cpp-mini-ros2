#pragma once

#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "executor.h"
#include "subscription.h"

namespace mini_ros
{

    class TopicManager
    {
    public:
        static TopicManager &instance()
        {
            static TopicManager manager;

            return manager;
        }

        // ==================================================
        // SUBSCRIBE
        // ==================================================

        template <typename T>
        void subscribe(
            const std::string &topic,
            std::function<void(std::shared_ptr<const T>)> callback,
            SubscriptionPtr subscription)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto wrapper =
                [callback, subscription](std::shared_ptr<const void> data)
            {
                if (!subscription->alive())
                {
                    return;
                }

                callback(
                    std::static_pointer_cast<const T>(data));
            };

            subscribers_[topic].push_back(
                { subscription, wrapper });

            // -----------------------------------------------
            // FIX: Cleanup dead entries khi có subscriber mới
            // đăng ký vào topic này. Tránh vector phình vô hạn.
            // -----------------------------------------------

            pruneDeadSubscribers(topic);
        }

        // ==================================================
        // PUBLISH
        // ==================================================

        template <typename T>
        void publish(
            const std::string &topic,
            std::shared_ptr<const T> message)
        {
            std::vector<Callback> callbacks;

            {
                // ------------------------------------------
                // LOCK CHỈ ĐỂ COPY DANH SÁCH CALLBACK
                // ------------------------------------------

                std::lock_guard<std::mutex> lock(mutex_);

                auto it = subscribers_.find(topic);

                if (it == subscribers_.end())
                {
                    return;
                }

                // Chỉ lấy callback của subscriber còn alive
                for (auto &entry : it->second)
                {
                    if (entry.subscription->alive())
                    {
                        callbacks.push_back(entry.callback);
                    }
                }
            }

            // ----------------------------------------------
            // EXECUTE NGOÀI LOCK
            // ----------------------------------------------

            for (auto &callback : callbacks)
            {
                Executor::instance().post(
                    [callback, message]()
                    {
                        callback(message);
                    });
            }
        }

        // ==================================================
        // PRUNE (có thể gọi thủ công nếu muốn)
        // ==================================================

        void pruneAllTopics()
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (auto &[topic, _] : subscribers_)
            {
                pruneDeadSubscribers(topic);
            }
        }

        size_t subscriberCount(const std::string &topic) const
        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = subscribers_.find(topic);

            if (it == subscribers_.end())
            {
                return 0;
            }

            size_t count = 0;

            for (const auto &entry : it->second)
            {
                if (entry.subscription->alive())
                {
                    ++count;
                }
            }

            return count;
        }

    private:

        // ==================================================
        // INTERNAL: phải được gọi khi đang giữ mutex_
        // ==================================================

        void pruneDeadSubscribers(const std::string &topic)
        {
            auto it = subscribers_.find(topic);

            if (it == subscribers_.end())
            {
                return;
            }

            auto &vec = it->second;

            vec.erase(
                std::remove_if(
                    vec.begin(),
                    vec.end(),
                    [](const SubscriberEntry &e)
                    {
                        return !e.subscription->alive();
                    }),
                vec.end());
        }

    private:
        using Callback =
            std::function<
                void(std::shared_ptr<const void>)>;

        struct SubscriberEntry
        {
            SubscriptionPtr subscription;
            Callback        callback;
        };

        std::unordered_map<
            std::string,
            std::vector<SubscriberEntry>>
            subscribers_;

        mutable std::mutex mutex_;
    };

}
