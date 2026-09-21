#ifndef CAMERA_UI_PKG_UI_NODE_HPP_
#define CAMERA_UI_PKG_UI_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>

#include <opencv2/opencv.hpp>

#include <mutex>
#include <string>

class UiNode : public rclcpp::Node
{
public:
  UiNode();

private:
  void imageCallback(
    const sensor_msgs::msg::Image::SharedPtr msg);

  void timerCallback();

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::TimerBase::SharedPtr timer_;

  cv::Mat latest_image_;
  std::mutex image_mutex_;

  std::string image_topic_;
  double display_hz_;
};

#endif
