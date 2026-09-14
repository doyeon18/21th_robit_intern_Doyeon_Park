#ifndef ROBOT_MAZE_GAME_MAINWINDOW_H
#define ROBOT_MAZE_GAME_MAINWINDOW_H

#include "robot_maze_game/gui_ros_bridge.h"
#include "robot_maze_game/msg/game_state.hpp"

#include <QMainWindow>
#include <QPoint>

#include <cstdint>
#include <memory>
#include <vector>

class QGraphicsItem;
class QGraphicsEllipseItem;
class QGraphicsPolygonItem;
class QGraphicsScene;
class QEvent;
class QKeyEvent;
class QResizeEvent;
class QTimer;
class QVariantAnimation;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace robot_maze_game
{

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(std::shared_ptr<GuiRosBridge> bridge,
                        QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    using GameState = msg::GameState;

    Ui::MainWindow *ui;
    std::shared_ptr<GuiRosBridge> bridge_;
    QGraphicsScene *gameScene_;
    QTimer *movementTimer_;
    QVariantAnimation *playerAnimation_;
    QGraphicsEllipseItem *playerItem_ = nullptr;
    std::vector<QGraphicsPolygonItem *> enemyItems_;
    std::vector<QVariantAnimation *> enemyAnimations_;
    std::vector<QPoint> displayedEnemyPositions_;
    std::vector<QGraphicsItem *> dynamicItems_;
    std::vector<int> heldMovementKeys_;
    int displayedPlayerX_ = -1;
    int displayedPlayerY_ = -1;
    std::uint32_t displayedSeed_ = 0U;
    std::uint16_t displayedColumns_ = 0U;
    std::uint16_t displayedRows_ = 0U;
    std::uint8_t previousStatus_ = 255U;
    std::uint32_t lastEventSequence_ = 0U;
    bool receivedState_ = false;

    void applyGameState(const GameState &state);
    bool rebuildMapIfNeeded(const GameState &state);
    void renderActors(const GameState &state);
    void clearDynamicItems();
    void clearEnemyActors();
    void fitGameScene();
    void setConnectionStatus(const QString &text, const char *state);
    void setGameStatus(const QString &text, const char *state);
    void updateBatteryStyle(std::uint8_t battery);
    void requestRestart(bool newMaze);
    void showResult(const GameState &state);
    void appendEvent(const QString &message);
    bool handleKeyPress(QKeyEvent *event);
    bool handleKeyRelease(QKeyEvent *event);
    void sendHeldMovementCommand();
    std::uint8_t selectedDifficulty() const;

    static int movementCommandForKey(int key);
    static int enemyAnimationDuration(std::uint8_t difficulty);
    static QString formatMilliseconds(std::uint32_t milliseconds);
};

} // namespace robot_maze_game

#endif // ROBOT_MAZE_GAME_MAINWINDOW_H
