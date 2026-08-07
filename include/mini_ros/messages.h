#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <chrono>

namespace mini_ros
{

    // ===========================================================
    // FIX: Thêm helper nowNs() để lấy timestamp chuẩn.
    // Không cần tự viết lại mỗi lần tạo message.
    // ===========================================================

    inline uint64_t nowNs()
    {
        return static_cast<uint64_t>(std::chrono::duration_cast<
                                         std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
                                         .count());
    }

    // ===========================================================
    // HEADER
    // ===========================================================

    struct Header
    {
        uint64_t timestamp = 0; // nanoseconds, dùng nowNs()

        std::string frameId;
    };

    // ===========================================================
    // LASER SCAN
    // ===========================================================

    struct LaserScan
    {
        Header header;

        std::vector<float> ranges;

        float angleMin = -1.57f;

        float angleMax = 1.57f;

        float angleIncrement = 0.01f;
    };

    // ===========================================================
    // POSE 2D
    // ===========================================================

    struct Pose2D
    {
        Header header;

        float x = 0.0f;

        float y = 0.0f;

        float theta = 0.0f;
    };

    // ===========================================================
    // OCCUPANCY GRID
    // ===========================================================

    struct OccupancyGrid
    {
        Header header;

        int width = 0;

        int height = 0;

        float resolution = 0.05f; // meters per cell

        std::vector<int8_t> data; // -1=unknown, 0=free, 100=occupied
    };

}
