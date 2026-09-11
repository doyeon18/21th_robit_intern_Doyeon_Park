#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QApplication>
#include <QTimer>

#include <algorithm>

namespace
{
QString lightStyle(const QString &color)
{
    return QString(
        "background-color: %1;"
        "border-radius: 20px;"
        "border: 2px solid black;")
        .arg(color);
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    rosNode_ = std::make_shared<rclcpp::Node>("traffic_sim_qt");

    lightSubscription_ =
        rosNode_->create_subscription<std_msgs::msg::String>(
            "/traffic_light_state",
            10,
            [this](const std_msgs::msg::String::SharedPtr msg) {
                updateTrafficLight(msg->data);
            });

    positionSubscription_ =
        rosNode_->create_subscription<std_msgs::msg::Float64>(
            "/vehicle_position",
            10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                updateVehiclePosition(msg->data);
            });

    speedSubscription_ =
        rosNode_->create_subscription<std_msgs::msg::Float64>(
            "/vehicle_speed",
            10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                updateVehicleSpeed(msg->data);
            });

    rosTimer_ = new QTimer(this);
    connect(rosTimer_, &QTimer::timeout, this, [this]() {
        if (rclcpp::ok()) {
            rclcpp::spin_some(rosNode_);
        } else {
            QApplication::quit();
        }
    });
    rosTimer_->start(10);

    updateTrafficLight("RED");
    updateVehiclePosition(0.0);
    updateVehicleSpeed(0.0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateTrafficLight(const std::string &state)
{
    const QString signal =
        QString::fromStdString(state).trimmed().toUpper();

    ui->signalLabelValue->setText(signal);
    ui->redLight->setStyleSheet(
        lightStyle(signal == "RED" ? "red" : "gray"));
    ui->yellowLight->setStyleSheet(
        lightStyle(signal == "YELLOW" ? "yellow" : "gray"));
    ui->greenLight->setStyleSheet(
        lightStyle(signal == "GREEN" ? "green" : "gray"));
}

void MainWindow::updateVehiclePosition(double position)
{
    const double boundedPosition = std::clamp(position, 0.0, 100.0);
    const int maximumX =
        ui->horizontalRoad->width() - ui->car->width();
    const int x = static_cast<int>(
        (boundedPosition / 100.0) * maximumX);

    ui->positionLabelValue->setText(
        QString::number(position, 'f', 1));
    ui->car->move(x, ui->car->y());
}

void MainWindow::updateVehicleSpeed(double speed)
{
    ui->speedLabelValue->setText(
        QString::number(speed, 'f', 1));
}
