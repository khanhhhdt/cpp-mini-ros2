#pragma once

#include <memory>
#include <mutex>

namespace mini_ros
{

    enum class CallbackGroupType
    {
        MutuallyExclusive, // Tại một thời điểm chỉ có 1 callback chạy
        Reentrant          // Nhiều callback có thể chạy song song
    };

    class CallbackGroup
    {
    public:
        explicit CallbackGroup(CallbackGroupType type)
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

    using CallbackGroupPtr = std::shared_ptr<CallbackGroup>;

}
