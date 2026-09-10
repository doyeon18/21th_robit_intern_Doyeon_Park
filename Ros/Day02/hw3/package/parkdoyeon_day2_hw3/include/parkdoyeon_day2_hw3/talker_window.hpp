#ifndef PARKDOYEON_DAY2_HW3__TALKER_WINDOW_HPP_
#define PARKDOYEON_DAY2_HW3__TALKER_WINDOW_HPP_

#include <cstddef>
#include <memory>

#include <QMainWindow>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

class QLabel;
class QLineEdit;
class QTimer;

class TalkerWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit TalkerWindow(
    const std::shared_ptr<rclcpp::Node> & node,
    QWidget * parent = nullptr);

private slots:
  void publishMessage();
  void processRos();

private:
  void createUi();
  void createRosInterfaces();

  std::shared_ptr<rclcpp::Node> node_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;

  QLineEdit * message_input_;
  QLabel * status_label_;
  QTimer * ros_timer_;
  std::size_t publish_count_{0};
};

#endif  // PARKDOYEON_DAY2_HW3__TALKER_WINDOW_HPP_
