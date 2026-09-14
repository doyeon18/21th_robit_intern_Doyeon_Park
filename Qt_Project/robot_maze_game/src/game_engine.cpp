#include "robot_maze_game/game_engine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <utility>

namespace robot_maze_game
{

namespace
{
bool contains(const std::vector<GridPosition> &positions, GridPosition target)
{
    return std::find(positions.begin(), positions.end(), target) != positions.end();
}

constexpr std::array<GridPosition, 4> kEnemyDirections{{
    {0, -1},
    {-1, 0},
    {1, 0},
    {0, 1}
}};
} // namespace

GameEngine::GameEngine()
{
    newGame(1U, Difficulty::Normal);
}

bool GameEngine::newGame(std::uint32_t seed, Difficulty difficulty)
{
    difficulty_ = difficulty;
    if (!map_.generate(seed)) {
        status_ = GameStatus::Error;
        setEvent("미로 생성에 실패했습니다.");
        return false;
    }

    switch (difficulty_) {
    case Difficulty::Easy:
        durationMilliseconds_ = 240000U;
        break;
    case Difficulty::Hard:
        durationMilliseconds_ = 150000U;
        break;
    default:
        durationMilliseconds_ = 180000U;
        break;
    }

    status_ = GameStatus::Ready;
    player_ = map_.startPosition();
    score_ = 0;
    elapsedMilliseconds_ = 0U;
    enemyMoveAccumulatorMilliseconds_ = 0U;
    successfulMoves_ = 0U;
    battery_ = 100U;
    totalKeys_ = 2U;
    placeObjects();
    if (status_ == GameStatus::Error) {
        return false;
    }
    setEvent("새 미로가 준비되었습니다.");
    return true;
}

bool GameEngine::restartSameMap()
{
    return newGame(map_.seed(), difficulty_);
}

bool GameEngine::start()
{
    if (status_ != GameStatus::Ready) {
        return false;
    }
    status_ = GameStatus::Running;
    setEvent("미션을 시작합니다.");
    return true;
}

bool GameEngine::pause()
{
    if (status_ != GameStatus::Running) {
        return false;
    }
    status_ = GameStatus::Paused;
    setEvent("미션을 일시정지했습니다.");
    return true;
}

bool GameEngine::resume()
{
    if (status_ != GameStatus::Paused) {
        return false;
    }
    status_ = GameStatus::Running;
    setEvent("미션을 계속합니다.");
    return true;
}

bool GameEngine::togglePause()
{
    return status_ == GameStatus::Paused ? resume() : pause();
}

bool GameEngine::move(MoveDirection direction)
{
    if (status_ != GameStatus::Running) {
        return false;
    }

    GridPosition offset;
    switch (direction) {
    case MoveDirection::Up:
        offset = {0, -1};
        break;
    case MoveDirection::Down:
        offset = {0, 1};
        break;
    case MoveDirection::Left:
        offset = {-1, 0};
        break;
    case MoveDirection::Right:
        offset = {1, 0};
        break;
    }

    const GridPosition next{player_.x + offset.x, player_.y + offset.y};
    if (!map_.isWalkable(next.x, next.y)) {
        setEvent("벽 때문에 이동할 수 없습니다.");
        return false;
    }

    player_ = next;
    ++successfulMoves_;
    ++score_;
    updateBattery();

    if (status_ == GameStatus::Lost) {
        return true;
    }

    if (contains(enemies_, player_)) {
        setLost("적 로봇과 충돌했습니다.");
        return true;
    }

    collectKeyIfPresent();

    if (player_ == map_.exitPosition()) {
        if (keys_.empty()) {
            status_ = GameStatus::Won;
            score_ += 500 + static_cast<int>(remainingMilliseconds() / 1000U) * 2;
            setEvent("열쇠를 모두 모으고 출구에 도착했습니다.");
        } else {
            setEvent("출구를 열려면 남은 열쇠를 모두 모아야 합니다.");
        }
    }
    return true;
}

void GameEngine::advanceTime(std::uint32_t milliseconds)
{
    if (status_ != GameStatus::Running || milliseconds == 0U) {
        return;
    }

    const std::uint32_t remaining = remainingMilliseconds();
    const std::uint32_t elapsedIncrement = std::min(milliseconds, remaining);
    elapsedMilliseconds_ += elapsedIncrement;
    if (elapsedMilliseconds_ >= durationMilliseconds_) {
        elapsedMilliseconds_ = durationMilliseconds_;
        setLost("제한 시간이 끝났습니다.");
        return;
    }

    enemyMoveAccumulatorMilliseconds_ += elapsedIncrement;
    const std::uint32_t interval = enemyMoveIntervalMilliseconds();
    while (enemyMoveAccumulatorMilliseconds_ >= interval
           && status_ == GameStatus::Running) {
        enemyMoveAccumulatorMilliseconds_ -= interval;
        moveEnemies();
    }
}

const GameMap &GameEngine::map() const { return map_; }
std::uint32_t GameEngine::seed() const { return map_.seed(); }
Difficulty GameEngine::difficulty() const { return difficulty_; }
GameStatus GameEngine::status() const { return status_; }
GridPosition GameEngine::player() const { return player_; }
const std::vector<GridPosition> &GameEngine::keys() const { return keys_; }
const std::vector<GridPosition> &GameEngine::enemies() const { return enemies_; }
int GameEngine::score() const { return score_; }
std::uint32_t GameEngine::elapsedMilliseconds() const { return elapsedMilliseconds_; }

std::uint32_t GameEngine::remainingMilliseconds() const
{
    return elapsedMilliseconds_ >= durationMilliseconds_
               ? 0U
               : durationMilliseconds_ - elapsedMilliseconds_;
}

std::uint8_t GameEngine::battery() const { return battery_; }

std::uint8_t GameEngine::collectedKeys() const
{
    return static_cast<std::uint8_t>(totalKeys_ - keys_.size());
}

std::uint8_t GameEngine::totalKeys() const { return totalKeys_; }
std::uint32_t GameEngine::eventSequence() const { return eventSequence_; }
const std::string &GameEngine::eventText() const { return eventText_; }

void GameEngine::placeObjects()
{
    std::vector<GridPosition> candidates;
    const GridPosition start = map_.startPosition();
    const GridPosition exit = map_.exitPosition();

    for (int y = 1; y < map_.rows() - 1; ++y) {
        for (int x = 1; x < map_.columns() - 1; ++x) {
            const GridPosition position{x, y};
            const int startDistance = std::abs(x - start.x) + std::abs(y - start.y);
            const int exitDistance = std::abs(x - exit.x) + std::abs(y - exit.y);
            if (map_.isWalkable(x, y) && position != start && position != exit
                && startDistance >= 6 && exitDistance >= 3) {
                candidates.push_back(position);
            }
        }
    }

    std::mt19937 randomEngine(map_.seed() ^ 0x9e3779b9U
                              ^ (static_cast<std::uint32_t>(difficulty_) << 24U));
    std::shuffle(candidates.begin(), candidates.end(), randomEngine);

    keys_.clear();
    enemies_.clear();
    const std::size_t required = static_cast<std::size_t>(totalKeys_) + enemyCount();
    if (candidates.size() < required) {
        status_ = GameStatus::Error;
        setEvent("아이템을 배치할 공간이 부족합니다.");
        return;
    }

    for (std::size_t index = 0; index < totalKeys_; ++index) {
        keys_.push_back(candidates[index]);
    }
    for (std::size_t index = totalKeys_; index < required; ++index) {
        enemies_.push_back(candidates[index]);
    }
}

void GameEngine::moveEnemies()
{
    if (contains(enemies_, player_)) {
        setLost("적 로봇과 충돌했습니다.");
        return;
    }

    // Move in the stable placement order so the same seed and input sequence
    // always produce the same enemy positions.
    for (std::size_t index = 0; index < enemies_.size(); ++index) {
        enemies_[index] = nextEnemyStep(index);
        if (enemies_[index] == player_) {
            setLost("적 로봇에게 붙잡혔습니다.");
            return;
        }
    }
}

GridPosition GameEngine::nextEnemyStep(std::size_t enemyIndex) const
{
    const GridPosition current = enemies_[enemyIndex];
    GridPosition best = current;
    int bestDistance = map_.shortestPathLength(current, player_);
    if (bestDistance <= 0) {
        return best;
    }

    for (const GridPosition direction : kEnemyDirections) {
        const GridPosition candidate{
            current.x + direction.x,
            current.y + direction.y
        };
        if (!map_.isWalkable(candidate.x, candidate.y)) {
            continue;
        }

        bool occupiedByAnotherEnemy = false;
        for (std::size_t other = 0; other < enemies_.size(); ++other) {
            if (other != enemyIndex && enemies_[other] == candidate) {
                occupiedByAnotherEnemy = true;
                break;
            }
        }
        if (occupiedByAnotherEnemy) {
            continue;
        }

        const int candidateDistance = map_.shortestPathLength(candidate, player_);
        if (candidateDistance >= 0 && candidateDistance < bestDistance) {
            best = candidate;
            bestDistance = candidateDistance;
        }
    }
    return best;
}

void GameEngine::collectKeyIfPresent()
{
    const auto found = std::find(keys_.begin(), keys_.end(), player_);
    if (found == keys_.end()) {
        return;
    }
    keys_.erase(found);
    score_ += 100;
    setEvent("열쇠를 획득했습니다.");
}

void GameEngine::updateBattery()
{
    const unsigned int consumed = successfulMoves_ / movesPerBatteryUnit();
    battery_ = static_cast<std::uint8_t>(consumed >= 100U ? 0U : 100U - consumed);
    if (battery_ == 0U) {
        setLost("배터리를 모두 사용했습니다.");
    }
}

void GameEngine::setLost(const std::string &reason)
{
    status_ = GameStatus::Lost;
    setEvent(reason);
}

void GameEngine::setEvent(std::string text)
{
    ++eventSequence_;
    eventText_ = std::move(text);
}

unsigned int GameEngine::movesPerBatteryUnit() const
{
    switch (difficulty_) {
    case Difficulty::Easy:
        return 4U;
    case Difficulty::Hard:
        return 2U;
    default:
        return 3U;
    }
}

unsigned int GameEngine::enemyCount() const
{
    switch (difficulty_) {
    case Difficulty::Easy:
        return 1U;
    case Difficulty::Hard:
        return 3U;
    default:
        return 2U;
    }
}

std::uint32_t GameEngine::enemyMoveIntervalMilliseconds() const
{
    switch (difficulty_) {
    case Difficulty::Easy:
        return 900U;
    case Difficulty::Hard:
        return 450U;
    default:
        return 650U;
    }
}

} // namespace robot_maze_game
