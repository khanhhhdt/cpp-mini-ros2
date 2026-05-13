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

        std::mutex &mutex()
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