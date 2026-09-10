cmake_minimum_required(VERSION 3.8)
project(parkdoyeon_day2_hw2)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(turtlesim REQUIRED)
find_package(std_srvs REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(turtle_gui
  src/main.cpp
  src/turtle_gui.cpp
  include/parkdoyeon_day2_hw2/turtle_gui.hpp
)

target_include_directories(turtle_gui PRIVATE
  include
)

ament_target_dependencies(turtle_gui
  rclcpp
  geometry_msgs
  turtlesim
  std_srvs
)

target_link_libraries(turtle_gui
  Qt5::Widgets
)

install(TARGETS
  turtle_gui
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()