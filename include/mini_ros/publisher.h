#pragma once

#include <memory>
#include <string>

#include "topic_manager.h"

namespace mini_ros
{
    /**
     * @brief A publisher for sending messages on a specific topic.
     * @tparam T The type of message to publish.
     */
    template <typename T>
    class Publisher
    {
    public:
        Publisher(const std::string &topic)
            : topic_(topic)
        {
        }

        /**
         * @brief Publish a message to the topic.
         * @param message The message to publish.
         */
        void publish(std::shared_ptr<const T> message)
        {
            TopicManager::instance().publish<T>(topic_, message);
        }

    private:
        std::string topic_;
    };

}
