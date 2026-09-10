#include <memory>

#include <QApplication>

#include <rclcpp/rclcpp.hpp>

#include "parkdoyeon_day2_hw3/talker_window.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  QApplication application(argc, argv);

  int result = 0;
  {
    auto node = std::make_shared<rclcpp::Node>("qt_talker");
    TalkerWindow window(node);
    window.show();
    result = application.exec();
  }

  rclcpp::shutdown();
  return result;
}
