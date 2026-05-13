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

            subscribers_[topic].push_back(wrapper);
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
                // LOCK ONLY FOR COPYING CALLBACK LIST
                // ------------------------------------------

                std::lock_guard<std::mutex> lock(mutex_);

                auto it = subscribers_.find(topic);

                if (it == subscribers_.end())
                {
                    return;
                }

                callbacks = it->second;
            }

            // ----------------------------------------------
            // EXECUTE OUTSIDE LOCK
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

    private:
        using Callback =
            std::function<
                void(std::shared_ptr<const void>)>;

        std::unordered_map<
            std::string,
            std::vector<Callback>>
            subscribers_;

        std::mutex mutex_;
    };

}