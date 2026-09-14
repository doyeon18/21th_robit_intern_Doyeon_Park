cmake_minimum_required(VERSION 3.8)
project(parkdoyeon_day4_hw2)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(counter_publisher src/counter_publisher.cpp)
ament_target_dependencies(counter_publisher rclcpp rclcpp_lifecycle std_msgs)

add_executable(counter_subscriber src/counter_subscriber.cpp)
ament_target_dependencies(counter_subscriber rclcpp rclcpp_lifecycle std_msgs)

install(TARGETS counter_publisher counter_subscriber
  DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

ament_package()
