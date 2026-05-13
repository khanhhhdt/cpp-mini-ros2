// ======================================================
// include/mini_ros/messages.h
// ======================================================

#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace mini_ros
{

    struct Header
    {
        uint64_t timestamp = 0;

        std::string frameId;
    };

    // ======================================================
    // LASER SCAN
    // ======================================================

    struct LaserScan
    {
        Header header;

        std::vector<float> ranges;

        float angleMin = -1.57f;

        float angleMax = 1.57f;

        float angleIncrement = 0.01f;
    };

    // ======================================================
    // POSE
    // ======================================================

    struct Pose2D
    {
        Header header;

        float x = 0.0f;

        float y = 0.0f;

        float theta = 0.0f;
    };

    // ======================================================
    // OCCUPANCY GRID
    // ======================================================

    struct OccupancyGrid
    {
        Header header;

        int width = 0;

        int height = 0;

        float resolution = 0.05f;

        std::vector<int8_t> data;
    };

}