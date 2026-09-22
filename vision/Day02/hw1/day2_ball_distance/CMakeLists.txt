cmake_minimum_required(VERSION 3.16)
project(day2_ball_distance LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(insta360_usb_cam REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(ball_distance_node src/ball_distance_node.cpp)
target_compile_options(ball_distance_node PRIVATE -Wall -Wextra -Wpedantic)
ament_target_dependencies(
  ball_distance_node
  cv_bridge
  insta360_usb_cam
  rclcpp
  sensor_msgs
  std_msgs
)
target_link_libraries(ball_distance_node ${OpenCV_LIBS})

install(TARGETS ball_distance_node DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY config launch DESTINATION share/${PROJECT_NAME})

ament_package()
