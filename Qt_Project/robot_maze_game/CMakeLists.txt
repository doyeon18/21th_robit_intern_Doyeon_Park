cmake_minimum_required(VERSION 3.8)
project(robot_maze_game VERSION 0.1.0 LANGUAGES C CXX)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Widgets)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/GridPosition.msg"
  "msg/PlayerCommand.msg"
  "msg/GameState.msg"
  "srv/RestartGame.srv"
)
rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} "rosidl_typesupport_cpp")

add_library(robot_maze_core STATIC
  src/game_map.cpp
  src/game_engine.cpp
)
target_include_directories(robot_maze_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_options(robot_maze_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(game_engine_node
  src/engine_main.cpp
  src/game_engine_node.cpp
)
target_include_directories(game_engine_node PRIVATE include)
ament_target_dependencies(game_engine_node rclcpp)
target_link_libraries(game_engine_node
  robot_maze_core
  "${cpp_typesupport_target}"
)
target_compile_options(game_engine_node PRIVATE -Wall -Wextra -Wpedantic)

add_executable(game_gui_node
  src/gui_main.cpp
  src/gui_ros_bridge.cpp
  src/mainwindow.cpp
  src/game_result_dialog.cpp
  include/robot_maze_game/mainwindow.h
  include/robot_maze_game/game_result_dialog.h
  ui/mainwindow.ui
  ui/game_result_dialog.ui
)
set_target_properties(game_gui_node PROPERTIES
  AUTOMOC ON
  AUTOUIC ON
  AUTORCC ON
)
target_include_directories(game_gui_node PRIVATE include)
ament_target_dependencies(game_gui_node rclcpp)
target_link_libraries(game_gui_node
  Qt5::Widgets
  "${cpp_typesupport_target}"
)
target_compile_options(game_gui_node PRIVATE -Wall -Wextra -Wpedantic)

install(TARGETS
  game_engine_node
  game_gui_node
  DESTINATION lib/${PROJECT_NAME}
)
install(DIRECTORY launch config docs
  DESTINATION share/${PROJECT_NAME}
)
install(FILES README.md
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(game_engine_test test/test_game_engine.cpp)
  if(TARGET game_engine_test)
    target_link_libraries(game_engine_test robot_maze_core)
    target_compile_options(game_engine_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

ament_export_dependencies(rosidl_default_runtime)
ament_package()
