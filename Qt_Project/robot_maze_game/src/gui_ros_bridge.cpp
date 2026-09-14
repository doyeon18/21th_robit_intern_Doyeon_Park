#include "robot_maze_game/gui_ros_bridge.h"

#include <memory>
#include <utility>

namespace robot_maze_game
{

GuiRosBridge::GuiRosBridge(const rclcpp::NodeOptions &options)
    : rclcpp::Node("game_gui", options)
{
    commandPublisher_ = create_publisher<msg::PlayerCommand>(
        "player_command", rclcpp::QoS(rclcpp::KeepLast(10)).reliable());

    auto stateQos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    stateSubscription_ = create_subscription<msg::GameState>(
        "game_state", stateQos,
        [this](msg::GameState::ConstSharedPtr state) {
            if (stateCallback_) {
                stateCallback_(*state);
            }
        });

    restartClient_ = create_client<srv::RestartGame>("restart_game");
}

void GuiRosBridge::setStateCallback(StateCallback callback)
{
    stateCallback_ = std::move(callback);
}

void GuiRosBridge::publishCommand(std::uint8_t command, std::uint8_t difficulty)
{
    msg::PlayerCommand message;
    message.command = command;
    message.difficulty = difficulty;
    commandPublisher_->publish(message);
}

void GuiRosBridge::requestRestart(bool newMaze, std::uint8_t difficulty,
                                  RestartCallback callback)
{
    if (!restartClient_->service_is_ready()) {
        callback(false, 0U, "게임 엔진 서비스를 아직 찾지 못했습니다.");
        return;
    }

    auto request = std::make_shared<srv::RestartGame::Request>();
    request->new_maze = newMaze;
    request->difficulty = difficulty;

    restartClient_->async_send_request(
        request,
        [callback = std::move(callback)](
            rclcpp::Client<srv::RestartGame>::SharedFuture future) {
            try {
                const auto response = future.get();
                callback(response->success, response->seed, response->message);
            } catch (const std::exception &error) {
                callback(false, 0U, error.what());
            }
        });
}

} // namespace robot_maze_game
