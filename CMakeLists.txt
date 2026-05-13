cmake_minimum_required(VERSION 3.14)

project(mini_ros)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FIX: Bật warnings để dễ phát hiện vấn đề khi develop
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# -------------------------------------------------------
# LIB: Header-only interface target
# FIX: Dùng target_include_directories thay vì global
# include_directories(include). Cách này rõ ràng hơn và
# không ảnh hưởng đến các target khác trong cùng project.
# -------------------------------------------------------

add_library(mini_ros_headers INTERFACE)

target_include_directories(
    mini_ros_headers
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# -------------------------------------------------------
# EXECUTABLE: SLAM demo (main.cpp)
# -------------------------------------------------------

add_executable(
    mini_ros
    src/main.cpp
)

target_link_libraries(
    mini_ros
    PRIVATE
        mini_ros_headers
)

# Link threads (cần cho std::thread trên Linux/macOS)
find_package(Threads REQUIRED)

target_link_libraries(mini_ros PRIVATE Threads::Threads)

# -------------------------------------------------------
# EXECUTABLE: Feature demo (main-example-1.cpp)
# -------------------------------------------------------

add_executable(
    mini_ros_example
    src/main-example-1.cpp
)

target_link_libraries(
    mini_ros_example
    PRIVATE
        mini_ros_headers
        Threads::Threads
)
