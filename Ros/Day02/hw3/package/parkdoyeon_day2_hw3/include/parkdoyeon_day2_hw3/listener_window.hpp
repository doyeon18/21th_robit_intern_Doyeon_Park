#ifndef PARKDOYEON_DAY2_HW3__LISTENER_WINDOW_HPP_
#define PARKDOYEON_DAY2_HW3__LISTENER_WINDOW_HPP_

#include <cstddef>
#include <memory>

#include <QMainWindow>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

class QLabel;
class QTimer;

class ListenerWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit ListenerWindow(
    const std::shared_ptr<rclcpp::Node> & node,
    QWidget * parent = nullptr);

private slots:
  void processRos();

private:
  void createUi();
  void createRosInterfaces();
  void messageCallback(const std_msgs::msg::String::SharedPtr message);

  std::shared_ptr<rclcpp::Node> node_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;

  QLabel * received_message_label_;
  QLabel * status_label_;
  QTimer * ros_timer_;
  std::size_t receive_count_{0};
};

#endif  // PARKDOYEON_DAY2_HW3__LISTENER_WINDOW_HPP_
