#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float64.hpp>
#include <std_msgs/msg/string.hpp>

#include <memory>

class QTimer;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void updateTrafficLight(const std::string &state);
    void updateVehiclePosition(double position);
    void updateVehicleSpeed(double speed);

    Ui::MainWindow *ui;
    rclcpp::Node::SharedPtr rosNode_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr lightSubscription_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr positionSubscription_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr speedSubscription_;
    QTimer *rosTimer_;
};
#endif // MAINWINDOW_H
