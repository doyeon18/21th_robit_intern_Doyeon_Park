cmake_minimum_required(VERSION 3.8)
project(parkdoyeon_hw3)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(turtlesim REQUIRED)
find_package(std_srvs REQUIRED)

include_directories(
  include
)

add_executable(turtle_drawer
  src/main.cpp
  src/turtle_drawer.cpp
)

ament_target_dependencies(turtle_drawer
  rclcpp
  geometry_msgs
  turtlesim
  std_srvs
)

install(TARGETS
  turtle_drawer
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()