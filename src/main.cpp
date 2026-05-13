#include <iostream>
#include <thread>
#include <cmath>

#include "mini_ros/node.h"
#include "mini_ros/messages.h"
#include "mini_ros/executor.h"
#include "mini_ros/timer.h"
#include "mini_ros/callback_group.h"
#include "mini_ros/slam_node.h"

using namespace mini_ros;

// ======================================================
// VISUALIZATION NODE
// ======================================================

class VisualizationNode
{
public:
    explicit VisualizationNode(
        Node &node)
    {
        group_ =
            node.createCallbackGroup(
                CallbackGroupType::Reentrant);

        // ==========================================
        // POSE SUB
        // ==========================================

        poseSub_ =
            node.createSubscriber<Pose2D>(
                "/pose",
                group_,
                [this](std::shared_ptr<
                       const Pose2D>
                           pose)
                {
                    onPose(pose);
                });

        // ==========================================
        // MAP SUB
        // ==========================================

        mapSub_ =
            node.createSubscriber<
                OccupancyGrid>(
                "/map",
                group_,
                [this](std::shared_ptr<
                       const OccupancyGrid>
                           map)
                {
                    onMap(map);
                });
    }

private:
    void onPose(
        std::shared_ptr<
            const Pose2D>
            pose)
    {
        std::cout
            << "[VIS] pose = "
            << pose->x
            << ", "
            << pose->y
            << ", "
            << pose->theta
            << std::endl;
    }

    void onMap(
        std::shared_ptr<
            const OccupancyGrid>
            map)
    {
        std::cout
            << "[VIS] map updated : "
            << map->width
            << " x "
            << map->height
            << std::endl;
    }

private:
    CallbackGroupPtr group_;

    SubscriptionPtr poseSub_;

    SubscriptionPtr mapSub_;
};

// ======================================================
// MAIN
// ======================================================

int main()
{
    // ==============================================
    // EXECUTOR
    // ==============================================

    Executor::instance().start(4);

    // ==============================================
    // NODE
    // ==============================================

    Node node("robot_system");

    // ==============================================
    // SLAM NODE
    // ==============================================

    SlamNode slam(node);

    // ==============================================
    // VISUALIZATION
    // ==============================================

    VisualizationNode vis(node);

    // ==============================================
    // LIDAR PUBLISHER
    // ==============================================

    auto scanPublisher =
        node.createPublisher<LaserScan>("/scan");

    // ==============================================
    // FAKE LIDAR TIMER
    // ==============================================

    Timer lidarTimer(
        std::chrono::milliseconds(100),
        [&]()
        {
            auto scan =
                std::make_shared<LaserScan>();

            scan->header.timestamp = nowNs();
            scan->header.frameId   = "lidar";

            constexpr int count = 360;

            scan->ranges.resize(count);

            for (int i = 0; i < count; ++i)
            {
                float angle = i * 0.017f;

                scan->ranges[i] =
                    5.0f +
                    std::sin(angle * 4.0f) * 2.0f;
            }

            std::cout
                << "\n[LIDAR] publish scan"
                << std::endl;

            scanPublisher.publish(scan);
        });

    // ==============================================
    // WAIT
    // ==============================================

    std::cout
        << "\nSLAM simulation running..."
        << "\nPress ENTER to exit.\n"
        << std::endl;

    std::cin.get();

    Executor::instance().stop();

    return 0;
}
