#ifndef ROBOT_MAZE_GAME_GUI_ROS_BRIDGE_H
#define ROBOT_MAZE_GAME_GUI_ROS_BRIDGE_H

#include "rclcpp/rclcpp.hpp"
#include "robot_maze_game/msg/game_state.hpp"
#include "robot_maze_game/msg/player_command.hpp"
#include "robot_maze_game/srv/restart_game.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace robot_maze_game
{

class GuiRosBridge : public rclcpp::Node
{
public:
    using GameState = msg::GameState;
    using StateCallback = std::function<void(const GameState &)>;
    using RestartCallback = std::function<void(bool, std::uint32_t, const std::string &)>;

    explicit GuiRosBridge(const rclcpp::NodeOptions &options = rclcpp::NodeOptions());

    void setStateCallback(StateCallback callback);
    void publishCommand(std::uint8_t command, std::uint8_t difficulty);
    void requestRestart(bool newMaze, std::uint8_t difficulty,
                        RestartCallback callback);

private:
    rclcpp::Publisher<msg::PlayerCommand>::SharedPtr commandPublisher_;
    rclcpp::Subscription<msg::GameState>::SharedPtr stateSubscription_;
    rclcpp::Client<srv::RestartGame>::SharedPtr restartClient_;
    StateCallback stateCallback_;
};

} // namespace robot_maze_game

#endif // ROBOT_MAZE_GAME_GUI_ROS_BRIDGE_H
