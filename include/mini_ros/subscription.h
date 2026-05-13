#pragma once

#include <atomic>
#include <memory>

namespace mini_ros
{

    class Subscription
    {
    public:
        Subscription()
            : alive_(true)
        {
        }

        void unsubscribe()
        {
            alive_ = false;
        }

        bool alive() const
        {
            return alive_;
        }

    private:
        std::atomic<bool> alive_;
    };

    using SubscriptionPtr =
        std::shared_ptr<Subscription>;

}
