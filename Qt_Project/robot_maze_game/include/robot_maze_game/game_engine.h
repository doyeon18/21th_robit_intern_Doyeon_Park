#ifndef ROBOT_MAZE_GAME_GAME_ENGINE_H
#define ROBOT_MAZE_GAME_GAME_ENGINE_H

#include "robot_maze_game/game_map.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace robot_maze_game
{

enum class Difficulty : std::uint8_t
{
    Easy = 0,
    Normal = 1,
    Hard = 2
};

enum class GameStatus : std::uint8_t
{
    Ready = 0,
    Running = 1,
    Paused = 2,
    Won = 3,
    Lost = 4,
    Error = 5
};

enum class MoveDirection
{
    Up,
    Down,
    Left,
    Right
};

class GameEngine
{
public:
    GameEngine();

    bool newGame(std::uint32_t seed, Difficulty difficulty);
    bool restartSameMap();
    bool start();
    bool pause();
    bool resume();
    bool togglePause();
    bool move(MoveDirection direction);
    void advanceTime(std::uint32_t milliseconds);

    const GameMap &map() const;
    std::uint32_t seed() const;
    Difficulty difficulty() const;
    GameStatus status() const;
    GridPosition player() const;
    const std::vector<GridPosition> &keys() const;
    const std::vector<GridPosition> &enemies() const;
    int score() const;
    std::uint32_t elapsedMilliseconds() const;
    std::uint32_t remainingMilliseconds() const;
    std::uint8_t battery() const;
    std::uint8_t collectedKeys() const;
    std::uint8_t totalKeys() const;
    std::uint32_t eventSequence() const;
    const std::string &eventText() const;

private:
    GameMap map_;
    Difficulty difficulty_ = Difficulty::Normal;
    GameStatus status_ = GameStatus::Ready;
    GridPosition player_{1, 1};
    std::vector<GridPosition> keys_;
    std::vector<GridPosition> enemies_;
    int score_ = 0;
    std::uint32_t elapsedMilliseconds_ = 0U;
    std::uint32_t durationMilliseconds_ = 180000U;
    std::uint32_t enemyMoveAccumulatorMilliseconds_ = 0U;
    std::uint32_t successfulMoves_ = 0U;
    std::uint8_t battery_ = 100U;
    std::uint8_t totalKeys_ = 2U;
    std::uint32_t eventSequence_ = 0U;
    std::string eventText_;

    void placeObjects();
    void moveEnemies();
    GridPosition nextEnemyStep(std::size_t enemyIndex) const;
    void collectKeyIfPresent();
    void updateBattery();
    void setLost(const std::string &reason);
    void setEvent(std::string text);
    unsigned int movesPerBatteryUnit() const;
    unsigned int enemyCount() const;
    std::uint32_t enemyMoveIntervalMilliseconds() const;
};

} // namespace robot_maze_game

#endif // ROBOT_MAZE_GAME_GAME_ENGINE_H
