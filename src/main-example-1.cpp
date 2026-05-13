#include <iostream>
#include <memory>
#include <thread>

#include "mini_ros/node.h"
#include "mini_ros/messages.h"
#include "mini_ros/service_messages.h"
#include "mini_ros/executor.h"
#include "mini_ros/timer.h"
#include "mini_ros/callback_group.h"

using namespace mini_ros;

// ======================================================
// FREE FUNCTION
// ======================================================

void freeFunctionCallback(
    std::shared_ptr<const LaserScan> scan)
{
    std::cout
        << "[FREE FUNCTION] thread="
        << std::this_thread::get_id()
        << " size="
        << scan->ranges.size()
        << std::endl;
}

// ======================================================
// FUNCTOR
// ======================================================

class FunctorHandler
{
public:
    void operator()(
        std::shared_ptr<const LaserScan> scan)
    {
        std::cout
            << "[FUNCTOR] thread="
            << std::this_thread::get_id()
            << " size="
            << scan->ranges.size()
            << std::endl;
    }
};

// ======================================================
// CLASS
// ======================================================

class Mapper
{
public:
    void onScan(
        std::shared_ptr<const LaserScan> scan)
    {
        std::cout
            << "[MEMBER FUNCTION] thread="
            << std::this_thread::get_id()
            << " size="
            << scan->ranges.size()
            << std::endl;

        // simulate heavy work
        std::this_thread::sleep_for(
            std::chrono::milliseconds(500));
    }

    void onPose(
        std::shared_ptr<const Pose2D> pose)
    {
        std::cout
            << "[POSE] "
            << pose->x
            << ", "
            << pose->y
            << ", "
            << pose->theta
            << std::endl;
    }
};

// ======================================================
// MAIN
// ======================================================

int main()
{
    // ==================================================
    // START EXECUTOR
    // ==================================================

    Executor::instance().start(4);

    // ==================================================
    // NODE
    // ==================================================

    Node node("main_node");

    // ==================================================
    // CALLBACK GROUPS
    // ==================================================

    auto exclusiveGroup =
        node.createCallbackGroup(
            CallbackGroupType::MutuallyExclusive);

    auto reentrantGroup =
        node.createCallbackGroup(
            CallbackGroupType::Reentrant);

    // ==================================================
    // PUBLISHERS
    // ==================================================

    auto scanPublisher =
        node.createPublisher<LaserScan>("/scan");

    auto posePublisher =
        node.createPublisher<Pose2D>("/pose");

    // ==================================================
    // SIMPLE LAMBDA
    // ==================================================

    auto lambdaSub =
        node.createSubscriber<LaserScan>(
            "/scan",
            exclusiveGroup,
            [](std::shared_ptr<const LaserScan> scan)
            {
                std::cout
                    << "[LAMBDA] thread="
                    << std::this_thread::get_id()
                    << " size="
                    << scan->ranges.size()
                    << std::endl;
            });

    // ==================================================
    // CAPTURE LAMBDA
    // ==================================================

    int sensorId = 7;

    auto captureLambdaSub =
        node.createSubscriber<LaserScan>(
            "/scan",
            reentrantGroup,
            [sensorId](std::shared_ptr<const LaserScan> scan)
            {
                std::cout
                    << "[CAPTURE] sensor="
                    << sensorId
                    << " thread="
                    << std::this_thread::get_id()
                    << " size="
                    << scan->ranges.size()
                    << std::endl;
            });

    // ==================================================
    // FREE FUNCTION
    // ==================================================

    auto freeFunctionSub =
        node.createSubscriber<LaserScan>(
            "/scan",
            reentrantGroup,
            freeFunctionCallback);

    // ==================================================
    // FUNCTOR
    // ==================================================

    FunctorHandler functor;

    auto functorSub =
        node.createSubscriber<LaserScan>(
            "/scan",
            reentrantGroup,
            functor);

    // ==================================================
    // MEMBER FUNCTION
    // ==================================================

    auto mapper =
        std::make_shared<Mapper>();

    auto memberSub =
        node.createSubscriber<LaserScan>(
            "/scan",
            &Mapper::onScan,
            mapper);

    auto poseSub =
        node.createSubscriber<Pose2D>(
            "/pose",
            &Mapper::onPose,
            mapper);

    // ==================================================
    // TEMP OBJECT TEST — kiểm tra weak_ptr safety
    // ==================================================

    {
        auto tempMapper =
            std::make_shared<Mapper>();

        auto tempSub =
            node.createSubscriber<LaserScan>(
                "/scan",
                &Mapper::onScan,
                tempMapper);

        std::cout
            << "[TEMP MAPPER CREATED]"
            << std::endl;
    }

    std::cout
        << "[TEMP MAPPER DESTROYED]"
        << std::endl;

    // ==================================================
    // SERVICE SERVER
    // FIX: Giữ serviceHandle để service tồn tại đúng lifetime.
    // Khi serviceHandle bị destroy, service tự unregister.
    // ==================================================

    auto serviceHandle =
        node.createService<
            AddTwoIntsRequest,
            AddTwoIntsResponse>(
            "/add_two_ints",
            [](std::shared_ptr<const AddTwoIntsRequest> req,
               std::shared_ptr<AddTwoIntsResponse> res)
            {
                res->sum = req->a + req->b;

                std::cout
                    << "[SERVICE] "
                    << req->a
                    << " + "
                    << req->b
                    << " = "
                    << res->sum
                    << std::endl;
            });

    // ==================================================
    // CLIENT
    // ==================================================

    auto client =
        node.createClient<
            AddTwoIntsRequest,
            AddTwoIntsResponse>(
            "/add_two_ints");

    // Chờ service sẵn sàng (optional, service đã register ở trên)
    if (!client.waitForService(std::chrono::milliseconds(1000)))
    {
        std::cerr
            << "[CLIENT] Service not available!"
            << std::endl;

        return 1;
    }

    auto request =
        std::make_shared<AddTwoIntsRequest>();

    request->a = 10;
    request->b = 20;

    auto response = client.call(request);

    if (response)
    {
        std::cout
            << "[CLIENT RESPONSE] "
            << response->sum
            << std::endl;
    }

    // ==================================================
    // TIMER : SCAN PUBLISH
    // ==================================================

    Timer scanTimer(
        std::chrono::milliseconds(1000),
        [&]()
        {
            auto scan =
                std::make_shared<LaserScan>();

            scan->header.timestamp = nowNs();
            scan->ranges           = { 1.0f, 2.0f, 3.0f, 4.0f };

            std::cout
                << "\n[PUBLISH] /scan"
                << std::endl;

            scanPublisher.publish(scan);
        });

    // ==================================================
    // TIMER : POSE PUBLISH
    // ==================================================

    Timer poseTimer(
        std::chrono::milliseconds(1500),
        [&]()
        {
            auto pose =
                std::make_shared<Pose2D>();

            pose->header.timestamp = nowNs();
            pose->x                = 1.2f;
            pose->y                = 2.5f;
            pose->theta            = 0.7f;

            std::cout
                << "\n[PUBLISH] /pose"
                << std::endl;

            posePublisher.publish(pose);
        });

    // ==================================================
    // UNSUBSCRIBE TEST
    // ==================================================

    Timer unsubscribeTimer(
        std::chrono::milliseconds(5000),
        [&]()
        {
            std::cout
                << "\n[UNSUBSCRIBE LAMBDA]"
                << std::endl;

            lambdaSub->unsubscribe();
        });

    // ==================================================
    // WAIT
    // ==================================================

    std::cout
        << "\nPress ENTER to exit...\n"
        << std::endl;

    std::cin.get();

    Executor::instance().stop();

    return 0;
}
