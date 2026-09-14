#include "robot_maze_game/game_map.h"

#include <algorithm>
#include <array>
#include <queue>
#include <stdexcept>

namespace robot_maze_game
{

namespace
{
constexpr std::array<GridPosition, 4> kDirections{{
    {1, 0},
    {-1, 0},
    {0, 1},
    {0, -1}
}};
} // namespace

GameMap::GameMap(int columns, int rows)
    : columns_(columns)
    , rows_(rows)
{
    if (columns_ < 9 || rows_ < 9 || columns_ % 2 == 0 || rows_ % 2 == 0) {
        throw std::invalid_argument("Maze dimensions must be odd and at least 9 x 9.");
    }
    tiles_.assign(rows_, std::vector<TileType>(columns_, TileType::Wall));
}

bool GameMap::generate(std::uint32_t seed)
{
    seed_ = seed;
    std::mt19937 randomEngine(seed_);

    fill(TileType::Wall);
    carvePerfectMaze(randomEngine);
    carveRooms(randomEngine);
    addLoops(randomEngine);

    start_ = {1, 1};
    exit_ = findFarthestFloor(start_);
    return isWalkable(start_.x, start_.y)
           && isWalkable(exit_.x, exit_.y)
           && hasPath(start_, exit_);
}

int GameMap::columns() const { return columns_; }
int GameMap::rows() const { return rows_; }
std::uint32_t GameMap::seed() const { return seed_; }
GridPosition GameMap::startPosition() const { return start_; }
GridPosition GameMap::exitPosition() const { return exit_; }

TileType GameMap::tileAt(int x, int y) const
{
    if (!isInside(x, y)) {
        return TileType::Wall;
    }
    return tiles_[y][x];
}

bool GameMap::isWalkable(int x, int y) const
{
    return tileAt(x, y) == TileType::Floor;
}

bool GameMap::hasPath(GridPosition from, GridPosition to) const
{
    return shortestPathLength(from, to) >= 0;
}

int GameMap::shortestPathLength(GridPosition from, GridPosition to) const
{
    if (!isWalkable(from.x, from.y) || !isWalkable(to.x, to.y)) {
        return -1;
    }

    std::vector<std::vector<int>> distance(rows_, std::vector<int>(columns_, -1));
    std::queue<GridPosition> pending;
    pending.push(from);
    distance[from.y][from.x] = 0;

    while (!pending.empty()) {
        const GridPosition current = pending.front();
        pending.pop();
        if (current == to) {
            return distance[current.y][current.x];
        }

        for (const GridPosition direction : kDirections) {
            const GridPosition next{current.x + direction.x, current.y + direction.y};
            if (!isWalkable(next.x, next.y) || distance[next.y][next.x] >= 0) {
                continue;
            }
            distance[next.y][next.x] = distance[current.y][current.x] + 1;
            pending.push(next);
        }
    }
    return -1;
}

int GameMap::cycleCount() const
{
    int vertices = 0;
    int edges = 0;
    for (int y = 0; y < rows_; ++y) {
        for (int x = 0; x < columns_; ++x) {
            if (!isWalkable(x, y)) {
                continue;
            }
            ++vertices;
            if (isWalkable(x + 1, y)) {
                ++edges;
            }
            if (isWalkable(x, y + 1)) {
                ++edges;
            }
        }
    }
    return vertices == 0 ? 0 : std::max(0, edges - vertices + 1);
}

bool GameMap::isInside(int x, int y) const
{
    return x >= 0 && y >= 0 && x < columns_ && y < rows_;
}

void GameMap::fill(TileType tile)
{
    for (auto &row : tiles_) {
        std::fill(row.begin(), row.end(), tile);
    }
}

void GameMap::carvePerfectMaze(std::mt19937 &randomEngine)
{
    std::vector<GridPosition> stack{{1, 1}};
    tiles_[1][1] = TileType::Floor;

    while (!stack.empty()) {
        const GridPosition current = stack.back();
        std::array<GridPosition, 4> directions = kDirections;
        std::shuffle(directions.begin(), directions.end(), randomEngine);

        bool moved = false;
        for (const GridPosition direction : directions) {
            const GridPosition next{
                current.x + direction.x * 2,
                current.y + direction.y * 2
            };
            if (next.x <= 0 || next.y <= 0
                || next.x >= columns_ - 1 || next.y >= rows_ - 1
                || tiles_[next.y][next.x] == TileType::Floor) {
                continue;
            }

            tiles_[current.y + direction.y][current.x + direction.x] = TileType::Floor;
            tiles_[next.y][next.x] = TileType::Floor;
            stack.push_back(next);
            moved = true;
            break;
        }
        if (!moved) {
            stack.pop_back();
        }
    }
}

void GameMap::carveRooms(std::mt19937 &randomEngine)
{
    std::uniform_int_distribution<int> columnDistribution(2, columns_ - 5);
    std::uniform_int_distribution<int> rowDistribution(2, rows_ - 5);

    constexpr int roomCount = 4;
    for (int room = 0; room < roomCount; ++room) {
        const int left = columnDistribution(randomEngine);
        const int top = rowDistribution(randomEngine);
        const int width = room % 2 == 0 ? 4 : 5;
        const int height = room % 2 == 0 ? 4 : 3;
        for (int y = top; y < std::min(top + height, rows_ - 1); ++y) {
            for (int x = left; x < std::min(left + width, columns_ - 1); ++x) {
                tiles_[y][x] = TileType::Floor;
            }
        }
    }

    for (int y = 1; y <= 3; ++y) {
        for (int x = 1; x <= 3; ++x) {
            tiles_[y][x] = TileType::Floor;
        }
    }
}

void GameMap::addLoops(std::mt19937 &randomEngine)
{
    std::vector<GridPosition> candidates;
    for (int y = 1; y < rows_ - 1; ++y) {
        for (int x = 1; x < columns_ - 1; ++x) {
            if (tiles_[y][x] != TileType::Wall) {
                continue;
            }
            const bool horizontalBridge = isWalkable(x - 1, y) && isWalkable(x + 1, y);
            const bool verticalBridge = isWalkable(x, y - 1) && isWalkable(x, y + 1);
            if (horizontalBridge || verticalBridge) {
                candidates.push_back({x, y});
            }
        }
    }

    std::shuffle(candidates.begin(), candidates.end(), randomEngine);
    const int loopTarget = std::min<int>(
        candidates.size(), std::max(5, columns_ * rows_ / 90));
    for (int index = 0; index < loopTarget; ++index) {
        const GridPosition wall = candidates[index];
        tiles_[wall.y][wall.x] = TileType::Floor;
    }
}

GridPosition GameMap::findFarthestFloor(GridPosition origin) const
{
    std::vector<std::vector<int>> distance(rows_, std::vector<int>(columns_, -1));
    std::queue<GridPosition> pending;
    pending.push(origin);
    distance[origin.y][origin.x] = 0;

    GridPosition farthest = origin;
    while (!pending.empty()) {
        const GridPosition current = pending.front();
        pending.pop();
        if (distance[current.y][current.x] > distance[farthest.y][farthest.x]) {
            farthest = current;
        }
        for (const GridPosition direction : kDirections) {
            const GridPosition next{current.x + direction.x, current.y + direction.y};
            if (!isWalkable(next.x, next.y) || distance[next.y][next.x] >= 0) {
                continue;
            }
            distance[next.y][next.x] = distance[current.y][current.x] + 1;
            pending.push(next);
        }
    }
    return farthest;
}

} // namespace robot_maze_game
