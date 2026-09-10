cmake_minimum_required(VERSION 3.8)
project(parkdoyeon_day2_hw3)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(talker
  src/talker_main.cpp
  src/talker_window.cpp
  include/parkdoyeon_day2_hw3/talker_window.hpp
)

target_include_directories(talker PRIVATE include)
ament_target_dependencies(talker rclcpp std_msgs)
target_link_libraries(talker Qt5::Widgets)

add_executable(listener
  src/listener_main.cpp
  src/listener_window.cpp
  include/parkdoyeon_day2_hw3/listener_window.hpp
)

target_include_directories(listener PRIVATE include)
ament_target_dependencies(listener rclcpp std_msgs)
target_link_libraries(listener Qt5::Widgets)

install(TARGETS
  talker
  listener
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
