#pragma once

#include <memory>
#include <string>

#include "topic_manager.h"

namespace mini_ros
{

    template <typename T>
    class Publisher
    {
    public:
        Publisher(const std::string &topic)
            : topic_(topic)
        {
        }

        // ==================================================
        // ZERO COPY PUBLISH
        // ==================================================

        void publish(
            std::shared_ptr<const T> message)
        {
            TopicManager::instance()
                .publish<T>(
                    topic_,
                    message);
        }

    private:
        std::string topic_;
    };

}