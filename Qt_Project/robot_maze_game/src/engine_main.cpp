#include "robot_maze_game/game_engine_node.h"

#include "rclcpp/rclcpp.hpp"

#include <memory>

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<robot_maze_game::GameEngineNode>());
    rclcpp::shutdown();
    return 0;
}
