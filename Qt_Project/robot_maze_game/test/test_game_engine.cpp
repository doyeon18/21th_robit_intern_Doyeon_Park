#include "robot_maze_game/game_engine.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace robot_maze_game
{

TEST(GameMapTest, RejectsInvalidDimensions)
{
    EXPECT_THROW(GameMap(8, 9), std::invalid_argument);
    EXPECT_THROW(GameMap(9, 8), std::invalid_argument);
    EXPECT_THROW(GameMap(7, 9), std::invalid_argument);
}

TEST(GameMapTest, GeneratesConnectedReachableMazes)
{
    for (std::uint32_t seed = 1U; seed <= 100U; ++seed) {
        GameMap map;
        ASSERT_TRUE(map.generate(seed)) << "seed=" << seed;
        EXPECT_TRUE(map.hasPath(map.startPosition(), map.exitPosition()))
            << "seed=" << seed;
        EXPECT_GE(map.cycleCount(), 5) << "seed=" << seed;

        for (int x = 0; x < map.columns(); ++x) {
            EXPECT_EQ(map.tileAt(x, 0), TileType::Wall);
            EXPECT_EQ(map.tileAt(x, map.rows() - 1), TileType::Wall);
        }
        for (int y = 0; y < map.rows(); ++y) {
            EXPECT_EQ(map.tileAt(0, y), TileType::Wall);
            EXPECT_EQ(map.tileAt(map.columns() - 1, y), TileType::Wall);
        }
    }
}

TEST(GameEngineTest, SameSeedAndDifficultyAreDeterministic)
{
    GameEngine first;
    GameEngine second;
    ASSERT_TRUE(first.newGame(123456U, Difficulty::Hard));
    ASSERT_TRUE(second.newGame(123456U, Difficulty::Hard));

    EXPECT_EQ(first.player(), second.player());
    EXPECT_EQ(first.keys(), second.keys());
    EXPECT_EQ(first.enemies(), second.enemies());
    for (int y = 0; y < first.map().rows(); ++y) {
        for (int x = 0; x < first.map().columns(); ++x) {
            EXPECT_EQ(first.map().tileAt(x, y), second.map().tileAt(x, y));
        }
    }
}

TEST(GameEngineTest, DifficultyChangesEnemyCount)
{
    GameEngine engine;
    ASSERT_TRUE(engine.newGame(42U, Difficulty::Easy));
    EXPECT_EQ(engine.enemies().size(), 1U);
    ASSERT_TRUE(engine.newGame(42U, Difficulty::Normal));
    EXPECT_EQ(engine.enemies().size(), 2U);
    ASSERT_TRUE(engine.newGame(42U, Difficulty::Hard));
    EXPECT_EQ(engine.enemies().size(), 3U);
}

TEST(GameEngineTest, StartPauseResumeControlTheClock)
{
    GameEngine engine;
    ASSERT_TRUE(engine.newGame(7U, Difficulty::Normal));
    EXPECT_EQ(engine.status(), GameStatus::Ready);
    EXPECT_TRUE(engine.start());
    EXPECT_EQ(engine.status(), GameStatus::Running);

    engine.advanceTime(1000U);
    EXPECT_EQ(engine.elapsedMilliseconds(), 1000U);
    EXPECT_TRUE(engine.pause());
    engine.advanceTime(1000U);
    EXPECT_EQ(engine.elapsedMilliseconds(), 1000U);
    EXPECT_TRUE(engine.resume());
    engine.advanceTime(500U);
    EXPECT_EQ(engine.elapsedMilliseconds(), 1500U);
}

TEST(GameEngineTest, EnemiesUseDifficultySpecificMovementIntervals)
{
    struct IntervalCase
    {
        Difficulty difficulty;
        std::uint32_t interval;
    };
    const std::vector<IntervalCase> cases{
        {Difficulty::Easy, 900U},
        {Difficulty::Normal, 650U},
        {Difficulty::Hard, 450U}
    };

    for (const IntervalCase testCase : cases) {
        GameEngine engine;
        ASSERT_TRUE(engine.newGame(17U, testCase.difficulty));
        ASSERT_TRUE(engine.start());
        const GridPosition player = engine.player();
        const auto initialEnemies = engine.enemies();

        engine.advanceTime(testCase.interval - 1U);
        EXPECT_EQ(engine.enemies(), initialEnemies);
        engine.advanceTime(1U);
        EXPECT_NE(engine.enemies(), initialEnemies);

        for (std::size_t index = 0; index < initialEnemies.size(); ++index) {
            const GridPosition movedEnemy = engine.enemies()[index];
            if (movedEnemy == initialEnemies[index]) {
                continue;
            }
            EXPECT_TRUE(engine.map().isWalkable(movedEnemy.x, movedEnemy.y));
            EXPECT_EQ(std::abs(movedEnemy.x - initialEnemies[index].x)
                          + std::abs(movedEnemy.y - initialEnemies[index].y),
                      1);
            EXPECT_LT(engine.map().shortestPathLength(movedEnemy, player),
                      engine.map().shortestPathLength(initialEnemies[index], player));
        }
    }
}

TEST(GameEngineTest, EnemyDoesNotMoveWhileReadyOrPaused)
{
    GameEngine engine;
    ASSERT_TRUE(engine.newGame(23U, Difficulty::Easy));
    const auto initialEnemies = engine.enemies();

    engine.advanceTime(5000U);
    EXPECT_EQ(engine.enemies(), initialEnemies);

    ASSERT_TRUE(engine.start());
    engine.advanceTime(450U);
    ASSERT_TRUE(engine.pause());
    const auto pausedEnemies = engine.enemies();
    engine.advanceTime(5000U);
    EXPECT_EQ(engine.enemies(), pausedEnemies);

    ASSERT_TRUE(engine.resume());
    engine.advanceTime(449U);
    EXPECT_EQ(engine.enemies(), pausedEnemies);
    engine.advanceTime(1U);
    EXPECT_NE(engine.enemies(), pausedEnemies);
}

TEST(GameEngineTest, EnemiesNeverEnterWallsOrShareATile)
{
    GameEngine engine;
    ASSERT_TRUE(engine.newGame(31U, Difficulty::Hard));
    ASSERT_TRUE(engine.start());

    for (int tick = 0; tick < 40 && engine.status() == GameStatus::Running; ++tick) {
        engine.advanceTime(450U);
        const auto &enemies = engine.enemies();
        for (std::size_t index = 0; index < enemies.size(); ++index) {
            EXPECT_TRUE(engine.map().isWalkable(enemies[index].x, enemies[index].y));
            EXPECT_EQ(std::count(enemies.begin(), enemies.end(), enemies[index]), 1);
        }
    }
}

TEST(GameEngineTest, EnemyReachingPlayerEndsTheMission)
{
    GameEngine engine;
    ASSERT_TRUE(engine.newGame(41U, Difficulty::Easy));
    ASSERT_EQ(engine.enemies().size(), 1U);
    ASSERT_TRUE(engine.start());

    const int initialDistance = engine.map().shortestPathLength(
        engine.enemies().front(), engine.player());
    ASSERT_GT(initialDistance, 0);
    for (int step = 0;
         step < initialDistance && engine.status() == GameStatus::Running;
         ++step) {
        engine.advanceTime(900U);
    }

    EXPECT_EQ(engine.status(), GameStatus::Lost);
    EXPECT_EQ(engine.enemies().front(), engine.player());
    EXPECT_NE(engine.eventText().find("적 로봇"), std::string::npos);
}

TEST(GameEngineTest, WallMovementIsRejected)
{
    GameEngine engine;
    ASSERT_TRUE(engine.newGame(99U, Difficulty::Normal));
    ASSERT_TRUE(engine.start());
    const GridPosition before = engine.player();

    EXPECT_FALSE(engine.move(MoveDirection::Left));
    EXPECT_EQ(engine.player(), before);
    EXPECT_EQ(engine.score(), 0);
    EXPECT_EQ(engine.battery(), 100U);
}

TEST(GameEngineTest, RestartRestoresInitialStateAndPlacements)
{
    GameEngine engine;
    ASSERT_TRUE(engine.newGame(111U, Difficulty::Normal));
    const auto initialKeys = engine.keys();
    const auto initialEnemies = engine.enemies();
    ASSERT_TRUE(engine.start());
    ASSERT_TRUE(engine.move(MoveDirection::Right));
    ASSERT_TRUE(engine.restartSameMap());

    EXPECT_EQ(engine.seed(), 111U);
    EXPECT_EQ(engine.status(), GameStatus::Ready);
    EXPECT_EQ(engine.player(), engine.map().startPosition());
    EXPECT_EQ(engine.keys(), initialKeys);
    EXPECT_EQ(engine.enemies(), initialEnemies);
    EXPECT_EQ(engine.score(), 0);
    EXPECT_EQ(engine.battery(), 100U);

    ASSERT_TRUE(engine.start());
    engine.advanceTime(649U);
    EXPECT_EQ(engine.enemies(), initialEnemies);
    ASSERT_TRUE(engine.restartSameMap());
    ASSERT_TRUE(engine.start());
    engine.advanceTime(1U);
    EXPECT_EQ(engine.enemies(), initialEnemies);
}

TEST(GameEngineTest, TimeLimitEndsTheMission)
{
    GameEngine engine;
    ASSERT_TRUE(engine.newGame(5U, Difficulty::Hard));
    ASSERT_TRUE(engine.start());
    engine.advanceTime(999999U);

    EXPECT_EQ(engine.status(), GameStatus::Lost);
    EXPECT_EQ(engine.remainingMilliseconds(), 0U);
    EXPECT_NE(engine.eventText().find("시간"), std::string::npos);
}

TEST(GameEngineTest, BatteryCanEndTheMission)
{
    GameEngine engine;
    ASSERT_TRUE(engine.newGame(321U, Difficulty::Hard));
    ASSERT_TRUE(engine.start());

    for (int step = 0; step < 220 && engine.status() == GameStatus::Running; ++step) {
        const MoveDirection direction = step % 2 == 0
                                            ? MoveDirection::Right
                                            : MoveDirection::Left;
        ASSERT_TRUE(engine.move(direction));
    }

    EXPECT_EQ(engine.battery(), 0U);
    EXPECT_EQ(engine.status(), GameStatus::Lost);
    EXPECT_NE(engine.eventText().find("배터리"), std::string::npos);
}

} // namespace robot_maze_game
