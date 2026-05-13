# mini_ros

`mini_ros` is a small C++17 learning project that implements ROS-like runtime
building blocks in a compact, header-oriented codebase.

## Features

- Node, publisher, subscriber, topic manager, and typed messages
- Multithreaded executor with callback groups
- Thread-safe task queue with clean shutdown support
- Service/client RPC-style APIs
- Timer support for periodic callbacks
- SLAM-style demo using fake lidar scans, pose updates, and map publishing

## Project Layout

```text
include/mini_ros/   Core mini_ros headers
src/main.cpp        SLAM simulation demo
src/main-example-1.cpp
                    Feature demo
```

## Requirements

- CMake 3.14 or newer
- A C++17 compiler

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run

Run the SLAM simulation demo:

```sh
./build/mini_ros
```

Run the feature demo:

```sh
./build/mini_ros_example
```

On Windows, the executables may be generated under a configuration directory,
for example:

```powershell
.\build\Debug\mini_ros.exe
.\build\Debug\mini_ros_example.exe
```

