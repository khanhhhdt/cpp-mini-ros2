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
        void subscribe(const std::string &topic,
                       std::function<void(std::shared_ptr<const T>)> callback,
                       SubscriptionPtr subscription)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto wrapper = [callback, subscription](std::shared_ptr<const void> data)
            {
                if (!subscription->alive())
                {
                    return;
                }

                callback(std::static_pointer_cast<const T>(data));
            };

            subscribers_[topic].push_back({subscription, wrapper});

            // Cleanup dead entries whenever have new subscriber
            pruneDeadSubscribers(topic);
        }

        /**
         * @brief public a topic with the message to all subscribers
         */
        template <typename T>
        void publish(const std::string &topic, std::shared_ptr<const T> message)
        {
            std::vector<Callback> callbacks;

            {
                // Get the subscribers for the topic
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

            // execute all callbacks
            for (auto &callback : callbacks)
            {
                Executor::instance().post([callback, message]()
                                          { callback(message); });
            }
        }

        /**
         * @brief remove all dead subscribers in all topics
         */
        void pruneAllTopics()
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (auto &[topic, _] : subscribers_)
            {
                pruneDeadSubscribers(topic);
            }
        }

        /**
         * @brief count number of subscriber in one specific topic
         */
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
        // Cleanup dead entries
        void pruneDeadSubscribers(const std::string &topic)
        {
            auto it = subscribers_.find(topic);

            if (it == subscribers_.end())
            {
                return;
            }

            auto &vec = it->second;

            vec.erase(std::remove_if(vec.begin(),
                                     vec.end(),
                                     [](const SubscriberEntry &e)
                                     {
                                         return !e.subscription->alive();
                                     }),
                      vec.end());
        }

    private:
        using Callback = std::function<void(std::shared_ptr<const void>)>;

        struct SubscriberEntry
        {
            SubscriptionPtr subscription;
            Callback callback;
        };

        mutable std::mutex mutex_;
        std::unordered_map<std::string, std::vector<SubscriberEntry>> subscribers_;
    };

}
