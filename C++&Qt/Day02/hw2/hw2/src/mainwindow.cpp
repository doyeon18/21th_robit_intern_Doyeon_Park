#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cmath>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , timer(new QTimer(this))
    , directionDot(nullptr)
    , carX(0)
    , carY(0)
    , carAngle(0)
    , driveMode(Stop)
    , movementSessionActive(false)
{
    ui->setupUi(this);

    // 자동차를 정사각형으로 설정
    ui->car->setFixedSize(50, 50);

    ui->car->setStyleSheet(
        "background-color: lightgray;"
        "border: 2px solid black;"
        );

    carX = ui->car->x();
    carY = ui->car->y();

    // 자동차 앞 방향 표시점
    directionDot = new QFrame(ui->car);
    directionDot->setFixedSize(10, 10);

    directionDot->setStyleSheet(
        "background-color: red;"
        "border-radius: 5px;"
        );

    directionDot->show();

    connect(timer,
            &QTimer::timeout,
            this,
            &MainWindow::updateCar);

    // 20ms마다 자동차 상태 갱신
    timer->start(20);

    updateDirectionDot();
    updateLabels();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startMovement(DriveMode mode)
{
    // 정지 상태에서 새로운 주행을 시작할 때만 저장
    if (!movementSessionActive)
    {
        historyStack.push_back(currentState());

        // Undo 후 새로운 움직임이 발생하면 Redo 기록 삭제
        redoStack.clear();

        movementSessionActive = true;
    }

    driveMode = mode;
}

void MainWindow::finishMovement()
{
    driveMode = Stop;
    movementSessionActive = false;

    updateLabels();
}

void MainWindow::on_forwardButton_pressed()
{
    startMovement(Forward);
}

void MainWindow::on_forwardButton_released()
{
    finishMovement();
}

void MainWindow::on_backwardButton_pressed()
{
    startMovement(Backward);
}

void MainWindow::on_backwardButton_released()
{
    finishMovement();
}

void MainWindow::on_leftButton_pressed()
{
    startMovement(TurnLeft);
}

void MainWindow::on_leftButton_released()
{
    finishMovement();
}

void MainWindow::on_rightButton_pressed()
{
    startMovement(TurnRight);
}

void MainWindow::on_rightButton_released()
{
    finishMovement();
}

void MainWindow::on_stopButton_clicked()
{
    driveMode = Stop;
    movementSessionActive = false;

    updateLabels();
}

void MainWindow::updateCar()
{
    const double moveSpeed = 3.0;
    const double turnSpeed = 2.0;

    if (driveMode == Stop)
        return;

    // 좌우회전 시 방향 계속 변경
    if (driveMode == TurnLeft)
        carAngle -= turnSpeed;

    if (driveMode == TurnRight)
        carAngle += turnSpeed;

    // 각도 0~360 유지
    if (carAngle < 0)
        carAngle += 360;

    if (carAngle >= 360)
        carAngle -= 360;

    constexpr double PI = 3.14159265358979323846;

    double radian = carAngle * PI / 180.0;
    double speed = 0.0;

    if (driveMode == Forward)
        speed = moveSpeed;

    else if (driveMode == Backward)
        speed = -moveSpeed;

    else if (driveMode == TurnLeft ||
             driveMode == TurnRight)
        speed = moveSpeed;

    // 현재 방향 기준 이동
    carX += std::sin(radian) * speed;
    carY -= std::cos(radian) * speed;

    // 필드 밖으로 나가지 못하게 제한
    double maxX =
        ui->carField->width() - ui->car->width();

    double maxY =
        ui->carField->height() - ui->car->height();

    carX = std::clamp(carX, 0.0, maxX);
    carY = std::clamp(carY, 0.0, maxY);

    ui->car->move(
        static_cast<int>(carX),
        static_cast<int>(carY)
        );

    updateDirectionDot();
    updateLabels();
}

void MainWindow::updateDirectionDot()
{
    constexpr double PI = 3.14159265358979323846;

    double radian = carAngle * PI / 180.0;

    double centerX = ui->car->width() / 2.0;
    double centerY = ui->car->height() / 2.0;

    // 자동차 중심에서 방향점까지 거리
    double radius = 17.0;

    double dotX =
        centerX
        + std::sin(radian) * radius
        - directionDot->width() / 2.0;

    double dotY =
        centerY
        - std::cos(radian) * radius
        - directionDot->height() / 2.0;

    directionDot->move(
        static_cast<int>(dotX),
        static_cast<int>(dotY)
        );
}

MainWindow::CarState MainWindow::currentState() const
{
    CarState state;

    state.x = carX;
    state.y = carY;
    state.angle = carAngle;

    return state;
}

void MainWindow::restoreState(const CarState &state)
{
    carX = state.x;
    carY = state.y;
    carAngle = state.angle;

    ui->car->move(
        static_cast<int>(carX),
        static_cast<int>(carY)
        );

    updateDirectionDot();
    updateLabels();
}

void MainWindow::on_undoButton_clicked()
{
    driveMode = Stop;
    movementSessionActive = false;

    if (historyStack.empty())
        return;

    // 현재 상태를 Redo용으로 저장
    redoStack.push_back(currentState());

    CarState previous = historyStack.back();

    historyStack.pop_back();

    restoreState(previous);

    ui->statusLabel->setText(
        "자동차 상태 : 정지 (UNDO)"
        );
}

void MainWindow::on_redoButton_clicked()
{
    driveMode = Stop;
    movementSessionActive = false;

    if (redoStack.empty())
        return;

    // 현재 상태를 다시 History에 저장
    historyStack.push_back(currentState());

    CarState next = redoStack.back();

    redoStack.pop_back();

    restoreState(next);

    ui->statusLabel->setText(
        "자동차 상태 : 정지 (REDO)"
        );
}

void MainWindow::updateLabels()
{
    ui->carCoord->setText(
        QString("자동차 위치 x : %1, y : %2   방향 : %3°")
            .arg(static_cast<int>(carX))
            .arg(static_cast<int>(carY))
            .arg(static_cast<int>(carAngle))
        );

    switch (driveMode)
    {
    case Forward:
        ui->statusLabel->setText("자동차 상태 : 전진");
        break;

    case Backward:
        ui->statusLabel->setText("자동차 상태 : 후진");
        break;

    case TurnLeft:
        ui->statusLabel->setText("자동차 상태 : 좌회전");
        break;

    case TurnRight:
        ui->statusLabel->setText("자동차 상태 : 우회전");
        break;

    case Stop:
        ui->statusLabel->setText("자동차 상태 : 정지");
        break;
    }
}
