#include <memory>

#include <QApplication>

#include <rclcpp/rclcpp.hpp>

#include "parkdoyeon_day2_hw2/turtle_gui.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  QApplication application(argc, argv);

  int result = 0;
  {
    auto node = std::make_shared<rclcpp::Node>("turtle_gui");
    TurtleGui window(node);
    window.show();
    result = application.exec();
  }

  rclcpp::shutdown();
  return result;
}
