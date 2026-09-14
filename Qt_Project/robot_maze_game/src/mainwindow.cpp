#include "robot_maze_game/mainwindow.h"

#include "robot_maze_game/game_result_dialog.h"
#include "robot_maze_game/msg/player_command.hpp"
#include "ui_mainwindow.h"

#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QEasingCurve>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QPolygonF>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QStyle>
#include <QTimer>
#include <QVariantAnimation>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <utility>

namespace robot_maze_game
{

namespace
{
constexpr int kTileSize = 40;
constexpr int kMovementIntervalMilliseconds = 150;
constexpr int kPlayerAnimationMilliseconds = 135;
constexpr int kEasyEnemyAnimationMilliseconds = 650;
constexpr int kNormalEnemyAnimationMilliseconds = 450;
constexpr int kHardEnemyAnimationMilliseconds = 300;

void refreshStyle(QWidget *widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}
} // namespace

MainWindow::MainWindow(std::shared_ptr<GuiRosBridge> bridge, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , bridge_(std::move(bridge))
    , gameScene_(new QGraphicsScene(this))
    , movementTimer_(new QTimer(this))
    , playerAnimation_(new QVariantAnimation(this))
{
    ui->setupUi(this);
    ui->gameView->setScene(gameScene_);
    ui->gameView->setRenderHints(QPainter::Antialiasing
                                 | QPainter::SmoothPixmapTransform);
    ui->gameView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->gameView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->gameView->setFocusPolicy(Qt::StrongFocus);
    ui->gameView->installEventFilter(this);
    ui->eventLogEdit->setFocusPolicy(Qt::NoFocus);

    movementTimer_->setInterval(kMovementIntervalMilliseconds);
    movementTimer_->setTimerType(Qt::PreciseTimer);
    connect(movementTimer_, &QTimer::timeout,
            this, &MainWindow::sendHeldMovementCommand);
    playerAnimation_->setDuration(kPlayerAnimationMilliseconds);
    playerAnimation_->setEasingCurve(QEasingCurve::Linear);
    connect(playerAnimation_, &QVariantAnimation::valueChanged,
            this, [this](const QVariant &position) {
                if (playerItem_ != nullptr) {
                    playerItem_->setPos(position.toPointF());
                }
            });

    setConnectionStatus("● ROS 2 CONNECTING", "connecting");
    setGameStatus("WAITING", "ready");
    ui->startButton->setEnabled(false);
    ui->pauseButton->setEnabled(false);
    ui->restartButton->setEnabled(false);
    ui->newMazeButton->setEnabled(false);
    ui->eventLogEdit->setPlainText("[system] Waiting for game engine state...");

    connect(ui->startButton, &QPushButton::clicked, this, [this] {
        bridge_->publishCommand(msg::PlayerCommand::START, selectedDifficulty());
        appendEvent("[GUI → Engine] START command");
        ui->gameView->setFocus();
    });
    connect(ui->pauseButton, &QPushButton::clicked, this, [this] {
        bridge_->publishCommand(msg::PlayerCommand::PAUSE_TOGGLE, selectedDifficulty());
        appendEvent("[GUI → Engine] PAUSE_TOGGLE command");
        ui->gameView->setFocus();
    });
    connect(ui->restartButton, &QPushButton::clicked, this, [this] {
        requestRestart(false);
    });
    connect(ui->newMazeButton, &QPushButton::clicked, this, [this] {
        requestRestart(true);
    });
    connect(ui->helpButton, &QPushButton::clicked, this, [this] {
        QMessageBox::information(
            this,
            "게임 방법",
            "WASD 또는 방향키로 로봇을 이동합니다.\n\n"
            "노란 열쇠 2개를 모두 획득한 뒤 초록색 출구로 이동하세요.\n"
            "빨간 적 로봇과 충돌하거나 시간·배터리가 끝나면 패배합니다.\n\n"
            "화면 명령은 ROS 2 토픽으로 엔진 노드에 전달됩니다.");
        ui->gameView->setFocus();
    });
    connect(ui->difficultyComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                appendEvent(QString("[setting] Difficulty: %1")
                                .arg(ui->difficultyComboBox->currentText()));
                ui->gameView->setFocus();
            });

    bridge_->setStateCallback([this](const GameState &state) {
        applyGameState(state);
    });

    QTimer::singleShot(3000, this, [this] {
        if (!receivedState_) {
            setConnectionStatus("● ROS 2 OFFLINE", "offline");
            appendEvent("[warning] Game engine state was not received.");
        }
    });
    QTimer::singleShot(0, this, [this] { ui->gameView->setFocus(); });
}

MainWindow::~MainWindow()
{
    bridge_->setStateCallback({});
    delete ui;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->gameView && event->type() == QEvent::KeyPress) {
        if (handleKeyPress(static_cast<QKeyEvent *>(event))) {
            return true;
        }
    } else if (watched == ui->gameView && event->type() == QEvent::KeyRelease) {
        if (handleKeyRelease(static_cast<QKeyEvent *>(event))) {
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (!handleKeyPress(event)) {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (!handleKeyRelease(event)) {
        QMainWindow::keyReleaseEvent(event);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    fitGameScene();
}

void MainWindow::applyGameState(const GameState &state)
{
    receivedState_ = true;
    setConnectionStatus("● ROS 2 CONNECTED", "connected");

    const bool rebuilt = rebuildMapIfNeeded(state);
    if (state.columns == 0U || state.rows == 0U
        || state.tiles.size()
               != static_cast<std::size_t>(state.columns) * state.rows) {
        setGameStatus("STATE ERROR", "error");
        appendEvent("[error] Engine sent an invalid map state.");
        return;
    }

    renderActors(state);
    if (rebuilt) {
        fitGameScene();
    }

    ui->timeValueLabel->setText(formatMilliseconds(state.remaining_ms));
    ui->scoreValueLabel->setText(
        QString("%1").arg(state.score, 4, 10, QChar('0')));
    ui->keyValueLabel->setText(
        QString("%1 / %2").arg(state.collected_keys).arg(state.total_keys));
    ui->batteryProgressBar->setValue(state.battery);
    ui->batteryValueLabel->setText(QString("%1%").arg(state.battery));
    updateBatteryStyle(state.battery);

    const QStringList difficulties{"쉬움", "보통", "어려움"};
    const int difficultyIndex = std::min<int>(state.difficulty, 2);
    {
        const QSignalBlocker blocker(ui->difficultyComboBox);
        ui->difficultyComboBox->setCurrentIndex(difficultyIndex);
    }
    ui->stageLabel->setText(
        QString("STAGE 01 · %1").arg(difficulties[difficultyIndex]));

    const bool gameInProgress = state.status == GameState::RUNNING
                                || state.status == GameState::PAUSED;
    if (state.status != GameState::RUNNING) {
        movementTimer_->stop();
        heldMovementKeys_.clear();
    }
    ui->difficultyComboBox->setEnabled(!gameInProgress);
    ui->restartButton->setEnabled(true);
    ui->newMazeButton->setEnabled(true);

    switch (state.status) {
    case GameState::READY:
        setGameStatus("READY", "ready");
        ui->startButton->setEnabled(true);
        ui->pauseButton->setEnabled(false);
        ui->pauseButton->setText("일시정지");
        break;
    case GameState::RUNNING:
        setGameStatus("MISSION ACTIVE", "active");
        ui->startButton->setEnabled(false);
        ui->pauseButton->setEnabled(true);
        ui->pauseButton->setText("일시정지");
        break;
    case GameState::PAUSED:
        setGameStatus("PAUSED", "paused");
        ui->startButton->setEnabled(false);
        ui->pauseButton->setEnabled(true);
        ui->pauseButton->setText("계속하기");
        break;
    case GameState::WON:
        setGameStatus("MISSION COMPLETE", "won");
        ui->startButton->setEnabled(false);
        ui->pauseButton->setEnabled(false);
        break;
    case GameState::LOST:
        setGameStatus("MISSION FAILED", "lost");
        ui->startButton->setEnabled(false);
        ui->pauseButton->setEnabled(false);
        break;
    default:
        setGameStatus("ENGINE ERROR", "error");
        ui->startButton->setEnabled(false);
        ui->pauseButton->setEnabled(false);
        break;
    }

    if (!state.event_text.empty() && state.event_sequence != lastEventSequence_) {
        lastEventSequence_ = state.event_sequence;
        const QString eventText = QString::fromStdString(state.event_text);
        if (eventText == QStringLiteral("벽 때문에 이동할 수 없습니다.")) {
            movementTimer_->stop();
            heldMovementKeys_.clear();
        }
        appendEvent(QString("[Engine → GUI] %1").arg(eventText));
    }

    if ((state.status == GameState::WON || state.status == GameState::LOST)
        && state.status != previousStatus_) {
        showResult(state);
    }
    previousStatus_ = state.status;
}

bool MainWindow::rebuildMapIfNeeded(const GameState &state)
{
    if (state.columns == 0U || state.rows == 0U
        || state.tiles.size()
               != static_cast<std::size_t>(state.columns) * state.rows) {
        return false;
    }

    if (displayedSeed_ == state.seed
        && displayedColumns_ == state.columns
        && displayedRows_ == state.rows) {
        return false;
    }

    playerAnimation_->stop();
    playerItem_ = nullptr;
    displayedPlayerX_ = -1;
    displayedPlayerY_ = -1;
    clearEnemyActors();
    dynamicItems_.clear();
    gameScene_->clear();
    gameScene_->setBackgroundBrush(QColor("#0d171b"));
    gameScene_->setSceneRect(0, 0,
                             state.columns * kTileSize,
                             state.rows * kTileSize);

    const QPen gridPen(QColor("#17272d"));
    for (int x = 0; x <= state.columns; ++x) {
        gameScene_->addLine(x * kTileSize, 0,
                            x * kTileSize, state.rows * kTileSize, gridPen);
    }
    for (int y = 0; y <= state.rows; ++y) {
        gameScene_->addLine(0, y * kTileSize,
                            state.columns * kTileSize, y * kTileSize, gridPen);
    }

    const QBrush wallBrush(QColor("#425861"));
    const QPen wallPen(QColor("#607781"));
    for (int y = 0; y < state.rows; ++y) {
        for (int x = 0; x < state.columns; ++x) {
            const std::size_t index = static_cast<std::size_t>(y) * state.columns + x;
            if (state.tiles[index] == GameState::TILE_WALL) {
                gameScene_->addRect(x * kTileSize, y * kTileSize,
                                    kTileSize, kTileSize, wallPen, wallBrush);
            }
        }
    }

    displayedSeed_ = state.seed;
    displayedColumns_ = state.columns;
    displayedRows_ = state.rows;
    appendEvent(QString("[map] New map received · seed %1").arg(state.seed));
    return true;
}

void MainWindow::renderActors(const GameState &state)
{
    clearDynamicItems();

    auto *exitItem = gameScene_->addRect(
        state.exit.x * kTileSize + 7, state.exit.y * kTileSize + 7, 26, 26,
        QPen(QColor("#63e9a5"), 3), QBrush(QColor("#153a28")));
    dynamicItems_.push_back(exitItem);

    for (const auto &key : state.keys) {
        auto *item = gameScene_->addRect(
            key.x * kTileSize + 12, key.y * kTileSize + 12, 16, 16,
            QPen(QColor("#fff0b3"), 2), QBrush(QColor("#ffd166")));
        dynamicItems_.push_back(item);
    }

    if (enemyItems_.size() != state.enemies.size()) {
        clearEnemyActors();
        enemyItems_.reserve(state.enemies.size());
        enemyAnimations_.reserve(state.enemies.size());
        displayedEnemyPositions_.reserve(state.enemies.size());

        for (std::size_t index = 0; index < state.enemies.size(); ++index) {
            QPolygonF diamond;
            diamond << QPointF(kTileSize / 2.0, kTileSize / 2.0 - 14)
                    << QPointF(kTileSize / 2.0 + 14, kTileSize / 2.0)
                    << QPointF(kTileSize / 2.0, kTileSize / 2.0 + 14)
                    << QPointF(kTileSize / 2.0 - 14, kTileSize / 2.0);
            auto *item = gameScene_->addPolygon(
                diamond, QPen(QColor("#ffc3c7"), 2),
                QBrush(QColor("#ff5b66")));
            item->setZValue(9.0);
            enemyItems_.push_back(item);

            auto *animation = new QVariantAnimation(this);
            animation->setEasingCurve(QEasingCurve::Linear);
            connect(animation, &QVariantAnimation::valueChanged,
                    this, [item](const QVariant &position) {
                        item->setPos(position.toPointF());
                    });
            enemyAnimations_.push_back(animation);
            displayedEnemyPositions_.emplace_back(-1, -1);
        }
    }

    for (std::size_t index = 0; index < state.enemies.size(); ++index) {
        const auto &enemy = state.enemies[index];
        const QPoint nextGridPosition(enemy.x, enemy.y);
        const QPointF targetPosition(enemy.x * kTileSize,
                                     enemy.y * kTileSize);
        const QPoint previousGridPosition = displayedEnemyPositions_[index];

        if (previousGridPosition != nextGridPosition) {
            const int gridDistance =
                std::abs(previousGridPosition.x() - nextGridPosition.x())
                + std::abs(previousGridPosition.y() - nextGridPosition.y());
            auto *animation = enemyAnimations_[index];
            animation->stop();
            if (state.status == GameState::RUNNING && gridDistance == 1) {
                animation->setDuration(enemyAnimationDuration(state.difficulty));
                animation->setStartValue(enemyItems_[index]->pos());
                animation->setEndValue(targetPosition);
                animation->start();
            } else {
                enemyItems_[index]->setPos(targetPosition);
            }
            displayedEnemyPositions_[index] = nextGridPosition;
        }
    }

    const QPointF playerTarget(state.player.x * kTileSize,
                               state.player.y * kTileSize);
    if (playerItem_ == nullptr) {
        playerItem_ = gameScene_->addEllipse(
            7, 7, 26, 26,
            QPen(QColor("#c3e8ff"), 2), QBrush(QColor("#59b8ff")));
        playerItem_->setZValue(10.0);
        playerItem_->setPos(playerTarget);
    } else if (displayedPlayerX_ != state.player.x
               || displayedPlayerY_ != state.player.y) {
        const int gridDistance = std::abs(displayedPlayerX_ - state.player.x)
                                 + std::abs(displayedPlayerY_ - state.player.y);
        playerAnimation_->stop();
        if (gridDistance == 1) {
            playerAnimation_->setStartValue(playerItem_->pos());
            playerAnimation_->setEndValue(playerTarget);
            playerAnimation_->start();
        } else {
            playerItem_->setPos(playerTarget);
        }
    }
    displayedPlayerX_ = state.player.x;
    displayedPlayerY_ = state.player.y;
}

void MainWindow::clearDynamicItems()
{
    for (QGraphicsItem *item : dynamicItems_) {
        gameScene_->removeItem(item);
        delete item;
    }
    dynamicItems_.clear();
}

void MainWindow::clearEnemyActors()
{
    for (QVariantAnimation *animation : enemyAnimations_) {
        animation->stop();
        delete animation;
    }
    enemyAnimations_.clear();

    for (QGraphicsPolygonItem *item : enemyItems_) {
        gameScene_->removeItem(item);
        delete item;
    }
    enemyItems_.clear();
    displayedEnemyPositions_.clear();
}

void MainWindow::fitGameScene()
{
    if (!gameScene_->items().empty() && !ui->gameView->viewport()->size().isEmpty()) {
        ui->gameView->fitInView(gameScene_->sceneRect(), Qt::KeepAspectRatio);
    }
}

void MainWindow::setConnectionStatus(const QString &text, const char *state)
{
    ui->connectionLabel->setText(text);
    ui->connectionLabel->setProperty("state", state);
    refreshStyle(ui->connectionLabel);
}

void MainWindow::setGameStatus(const QString &text, const char *state)
{
    ui->gameStatusLabel->setText(text);
    ui->gameStatusLabel->setProperty("state", state);
    refreshStyle(ui->gameStatusLabel);
}

void MainWindow::updateBatteryStyle(std::uint8_t battery)
{
    const char *level = battery <= 20U ? "critical"
                        : battery <= 45U ? "warning"
                                         : "normal";
    if (ui->batteryProgressBar->property("level").toString() != level) {
        ui->batteryProgressBar->setProperty("level", level);
        refreshStyle(ui->batteryProgressBar);
    }
}

void MainWindow::requestRestart(bool newMaze)
{
    ui->restartButton->setEnabled(false);
    ui->newMazeButton->setEnabled(false);
    appendEvent(newMaze ? "[GUI → Engine] Requesting a new maze..."
                        : "[GUI → Engine] Restarting the same maze...");

    bridge_->requestRestart(
        newMaze, selectedDifficulty(),
        [this](bool success, std::uint32_t seed, const std::string &message) {
            appendEvent(QString("[service] %1 · seed %2")
                            .arg(QString::fromStdString(message))
                            .arg(seed));
            if (!success) {
                ui->restartButton->setEnabled(receivedState_);
                ui->newMazeButton->setEnabled(receivedState_);
            }
            ui->gameView->setFocus();
        });
}

void MainWindow::showResult(const GameState &state)
{
    auto *dialog = new GameResultDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    const bool won = state.status == GameState::WON;
    QString message = QString::fromStdString(state.event_text);
    if (message.isEmpty()) {
        message = won ? "열쇠를 모두 모으고 출구에 도착했습니다."
                      : "임무를 완료하지 못했습니다.";
    }
    dialog->setResult(won, message, formatMilliseconds(state.elapsed_ms), state.score);
    connect(dialog, &QDialog::accepted, this, [this, dialog] {
        if (dialog->selectedAction() == GameResultDialog::Action::Retry) {
            requestRestart(false);
        } else if (dialog->selectedAction() == GameResultDialog::Action::NewMaze) {
            requestRestart(true);
        }
    });
    dialog->open();
}

void MainWindow::appendEvent(const QString &message)
{
    ui->eventLogEdit->appendPlainText(message);
    auto cursor = ui->eventLogEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->eventLogEdit->setTextCursor(cursor);
}

bool MainWindow::handleKeyPress(QKeyEvent *event)
{
    const int command = movementCommandForKey(event->key());
    if (command >= 0) {
        event->accept();
        if (event->isAutoRepeat()) {
            return true;
        }

        const bool movementWasIdle = heldMovementKeys_.empty();
        heldMovementKeys_.erase(
            std::remove(heldMovementKeys_.begin(), heldMovementKeys_.end(),
                        event->key()),
            heldMovementKeys_.end());
        heldMovementKeys_.push_back(event->key());
        if (movementWasIdle) {
            sendHeldMovementCommand();
            movementTimer_->start();
        }
        return true;
    }

    if (event->key() == Qt::Key_P || event->key() == Qt::Key_Space) {
        event->accept();
        if (!event->isAutoRepeat() && receivedState_) {
            bridge_->publishCommand(msg::PlayerCommand::PAUSE_TOGGLE,
                                    selectedDifficulty());
        }
        return true;
    }
    return false;
}

bool MainWindow::handleKeyRelease(QKeyEvent *event)
{
    if (movementCommandForKey(event->key()) < 0) {
        return false;
    }

    event->accept();
    if (event->isAutoRepeat()) {
        return true;
    }

    heldMovementKeys_.erase(
        std::remove(heldMovementKeys_.begin(), heldMovementKeys_.end(),
                    event->key()),
        heldMovementKeys_.end());
    if (heldMovementKeys_.empty()) {
        movementTimer_->stop();
    }
    return true;
}

void MainWindow::sendHeldMovementCommand()
{
    if (!receivedState_ || previousStatus_ != GameState::RUNNING
        || heldMovementKeys_.empty()) {
        return;
    }

    const int command = movementCommandForKey(heldMovementKeys_.back());
    if (command >= 0) {
        bridge_->publishCommand(static_cast<std::uint8_t>(command),
                                selectedDifficulty());
    }
}

std::uint8_t MainWindow::selectedDifficulty() const
{
    return static_cast<std::uint8_t>(ui->difficultyComboBox->currentIndex());
}

int MainWindow::movementCommandForKey(int key)
{
    switch (key) {
    case Qt::Key_W:
    case Qt::Key_Up:
        return msg::PlayerCommand::MOVE_UP;
    case Qt::Key_S:
    case Qt::Key_Down:
        return msg::PlayerCommand::MOVE_DOWN;
    case Qt::Key_A:
    case Qt::Key_Left:
        return msg::PlayerCommand::MOVE_LEFT;
    case Qt::Key_D:
    case Qt::Key_Right:
        return msg::PlayerCommand::MOVE_RIGHT;
    default:
        return -1;
    }
}

int MainWindow::enemyAnimationDuration(std::uint8_t difficulty)
{
    if (difficulty == 0U) {
        return kEasyEnemyAnimationMilliseconds;
    }
    if (difficulty == 2U) {
        return kHardEnemyAnimationMilliseconds;
    }
    return kNormalEnemyAnimationMilliseconds;
}

QString MainWindow::formatMilliseconds(std::uint32_t milliseconds)
{
    const std::uint32_t totalSeconds = (milliseconds + 999U) / 1000U;
    const std::uint32_t minutes = totalSeconds / 60U;
    const std::uint32_t seconds = totalSeconds % 60U;
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

} // namespace robot_maze_game
