#ifndef ROBOT_MAZE_GAME_GAME_MAP_H
#define ROBOT_MAZE_GAME_GAME_MAP_H

#include <cstdint>
#include <random>
#include <vector>

namespace robot_maze_game
{

struct GridPosition
{
    int x = 0;
    int y = 0;

    bool operator==(const GridPosition &other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!=(const GridPosition &other) const
    {
        return !(*this == other);
    }
};

enum class TileType
{
    Wall,
    Floor
};

class GameMap
{
public:
    GameMap(int columns = 31, int rows = 21);

    bool generate(std::uint32_t seed);

    int columns() const;
    int rows() const;
    std::uint32_t seed() const;
    GridPosition startPosition() const;
    GridPosition exitPosition() const;

    TileType tileAt(int x, int y) const;
    bool isWalkable(int x, int y) const;
    bool hasPath(GridPosition from, GridPosition to) const;
    int shortestPathLength(GridPosition from, GridPosition to) const;
    int cycleCount() const;

private:
    int columns_;
    int rows_;
    std::uint32_t seed_ = 0U;
    std::vector<std::vector<TileType>> tiles_;
    GridPosition start_{1, 1};
    GridPosition exit_{1, 1};

    bool isInside(int x, int y) const;
    void fill(TileType tile);
    void carvePerfectMaze(std::mt19937 &randomEngine);
    void carveRooms(std::mt19937 &randomEngine);
    void addLoops(std::mt19937 &randomEngine);
    GridPosition findFarthestFloor(GridPosition origin) const;
};

} // namespace robot_maze_game

#endif // ROBOT_MAZE_GAME_GAME_MAP_H
