cmake_minimum_required(VERSION 3.8)
project(parkdoyeon_day3_hw2)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(traffic_light_node
  src/traffic_light_node.cpp
)

ament_target_dependencies(traffic_light_node
  rclcpp
  std_msgs
)

add_executable(vehicle_node
  src/vehicle_node.cpp
)

ament_target_dependencies(vehicle_node
  rclcpp
  std_msgs
)

install(TARGETS
  traffic_light_node
  vehicle_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  config
  launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()