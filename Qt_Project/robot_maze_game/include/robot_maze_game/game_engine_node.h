#ifndef ROBOT_MAZE_GAME_GAME_ENGINE_NODE_H
#define ROBOT_MAZE_GAME_GAME_ENGINE_NODE_H

#include "robot_maze_game/game_engine.h"
#include "robot_maze_game/msg/game_state.hpp"
#include "robot_maze_game/msg/player_command.hpp"
#include "robot_maze_game/srv/restart_game.hpp"

#include "rclcpp/rclcpp.hpp"

#include <cstdint>

namespace robot_maze_game
{

class GameEngineNode : public rclcpp::Node
{
public:
    explicit GameEngineNode(const rclcpp::NodeOptions &options = rclcpp::NodeOptions());

private:
    GameEngine engine_;
    int tickMilliseconds_ = 100;
    rclcpp::Publisher<msg::GameState>::SharedPtr statePublisher_;
    rclcpp::Subscription<msg::PlayerCommand>::SharedPtr commandSubscription_;
    rclcpp::Service<srv::RestartGame>::SharedPtr restartService_;
    rclcpp::TimerBase::SharedPtr tickTimer_;

    void handleCommand(const msg::PlayerCommand &command);
    void handleRestart(
        const srv::RestartGame::Request::SharedPtr request,
        srv::RestartGame::Response::SharedPtr response);
    void publishState();
    static Difficulty sanitizeDifficulty(std::uint8_t value);
    static std::uint32_t createSeed();
};

} // namespace robot_maze_game

#endif // ROBOT_MAZE_GAME_GAME_ENGINE_NODE_H
