#include "robot_maze_game/game_engine_node.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <random>

namespace robot_maze_game
{

GameEngineNode::GameEngineNode(const rclcpp::NodeOptions &options)
    : rclcpp::Node("game_engine", options)
{
    const auto tickParameter = declare_parameter<std::int64_t>("tick_ms", 100);
    tickMilliseconds_ = static_cast<int>(
        std::clamp<std::int64_t>(tickParameter, 20, 1000));
    const auto difficultyParameter =
        declare_parameter<std::int64_t>("initial_difficulty", 1);
    const auto initialDifficulty = static_cast<std::uint8_t>(
        std::clamp<std::int64_t>(difficultyParameter, 0, 2));

    auto stateQos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    statePublisher_ = create_publisher<msg::GameState>("game_state", stateQos);
    commandSubscription_ = create_subscription<msg::PlayerCommand>(
        "player_command", rclcpp::QoS(rclcpp::KeepLast(10)).reliable(),
        [this](msg::PlayerCommand::ConstSharedPtr command) {
            handleCommand(*command);
        });
    restartService_ = create_service<srv::RestartGame>(
        "restart_game",
        [this](const srv::RestartGame::Request::SharedPtr request,
               srv::RestartGame::Response::SharedPtr response) {
            handleRestart(request, response);
        });

    const std::uint32_t initialSeed = createSeed();
    engine_.newGame(initialSeed, sanitizeDifficulty(initialDifficulty));
    publishState();

    tickTimer_ = create_wall_timer(
        std::chrono::milliseconds(tickMilliseconds_), [this] {
            if (engine_.status() == GameStatus::Running) {
                engine_.advanceTime(static_cast<std::uint32_t>(tickMilliseconds_));
                publishState();
            }
        });

    RCLCPP_INFO(get_logger(),
                "Game engine ready (seed=%u, topic=game_state, service=restart_game)",
                engine_.seed());
}

void GameEngineNode::handleCommand(const msg::PlayerCommand &command)
{
    bool recognized = true;
    switch (command.command) {
    case msg::PlayerCommand::START:
        if (engine_.status() == GameStatus::Ready
            && engine_.difficulty() != sanitizeDifficulty(command.difficulty)) {
            engine_.newGame(engine_.seed(), sanitizeDifficulty(command.difficulty));
        }
        engine_.start();
        break;
    case msg::PlayerCommand::MOVE_UP:
        engine_.move(MoveDirection::Up);
        break;
    case msg::PlayerCommand::MOVE_DOWN:
        engine_.move(MoveDirection::Down);
        break;
    case msg::PlayerCommand::MOVE_LEFT:
        engine_.move(MoveDirection::Left);
        break;
    case msg::PlayerCommand::MOVE_RIGHT:
        engine_.move(MoveDirection::Right);
        break;
    case msg::PlayerCommand::PAUSE_TOGGLE:
        engine_.togglePause();
        break;
    default:
        recognized = false;
        RCLCPP_WARN(get_logger(), "Ignored unknown player command: %u", command.command);
        break;
    }

    if (recognized) {
        publishState();
    }
}

void GameEngineNode::handleRestart(
    const srv::RestartGame::Request::SharedPtr request,
    srv::RestartGame::Response::SharedPtr response)
{
    const Difficulty difficulty = sanitizeDifficulty(request->difficulty);
    bool success = false;
    if (request->new_maze) {
        success = engine_.newGame(createSeed(), difficulty);
    } else if (difficulty != engine_.difficulty()) {
        success = engine_.newGame(engine_.seed(), difficulty);
    } else {
        success = engine_.restartSameMap();
    }

    response->success = success;
    response->seed = engine_.seed();
    response->message = success
                            ? (request->new_maze ? "새 미로를 생성했습니다."
                                                 : "같은 미로를 다시 시작합니다.")
                            : "미로 초기화에 실패했습니다.";
    publishState();
}

void GameEngineNode::publishState()
{
    msg::GameState state;
    state.seed = engine_.seed();
    state.columns = static_cast<std::uint16_t>(engine_.map().columns());
    state.rows = static_cast<std::uint16_t>(engine_.map().rows());
    state.tiles.reserve(static_cast<std::size_t>(state.columns) * state.rows);
    for (int y = 0; y < engine_.map().rows(); ++y) {
        for (int x = 0; x < engine_.map().columns(); ++x) {
            state.tiles.push_back(engine_.map().tileAt(x, y) == TileType::Wall
                                      ? msg::GameState::TILE_WALL
                                      : msg::GameState::TILE_FLOOR);
        }
    }

    const auto copyPosition = [](GridPosition from, msg::GridPosition &to) {
        to.x = static_cast<std::int16_t>(from.x);
        to.y = static_cast<std::int16_t>(from.y);
    };
    copyPosition(engine_.player(), state.player);
    copyPosition(engine_.map().exitPosition(), state.exit);
    state.keys.resize(engine_.keys().size());
    for (std::size_t index = 0; index < engine_.keys().size(); ++index) {
        copyPosition(engine_.keys()[index], state.keys[index]);
    }
    state.enemies.resize(engine_.enemies().size());
    for (std::size_t index = 0; index < engine_.enemies().size(); ++index) {
        copyPosition(engine_.enemies()[index], state.enemies[index]);
    }

    state.elapsed_ms = engine_.elapsedMilliseconds();
    state.remaining_ms = engine_.remainingMilliseconds();
    state.score = engine_.score();
    state.collected_keys = engine_.collectedKeys();
    state.total_keys = engine_.totalKeys();
    state.battery = engine_.battery();
    state.status = static_cast<std::uint8_t>(engine_.status());
    state.difficulty = static_cast<std::uint8_t>(engine_.difficulty());
    state.event_sequence = engine_.eventSequence();
    state.event_text = engine_.eventText();
    statePublisher_->publish(state);
}

Difficulty GameEngineNode::sanitizeDifficulty(std::uint8_t value)
{
    if (value == static_cast<std::uint8_t>(Difficulty::Easy)) {
        return Difficulty::Easy;
    }
    if (value == static_cast<std::uint8_t>(Difficulty::Hard)) {
        return Difficulty::Hard;
    }
    return Difficulty::Normal;
}

std::uint32_t GameEngineNode::createSeed()
{
    std::random_device randomDevice;
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    std::seed_seq sequence{
        randomDevice(),
        static_cast<unsigned int>(now),
        static_cast<unsigned int>(static_cast<unsigned long long>(now) >> 32U)
    };
    std::uint32_t seed = 0U;
    sequence.generate(&seed, &seed + 1);
    return seed;
}

} // namespace robot_maze_game
