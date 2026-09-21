#ifndef CAMERA_UI_PKG_CAMERA_NODE_HPP_
#define CAMERA_UI_PKG_CAMERA_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>

#include <opencv2/opencv.hpp>

#include <string>

class CameraNode : public rclcpp::Node
{
public:
  CameraNode();

private:
  void timerCallback();

  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_;
  rclcpp::TimerBase::SharedPtr timer_;

  cv::VideoCapture cap_;

  std::string image_topic_;
  double publish_hz_;
  int camera_index_;
};

#endif
