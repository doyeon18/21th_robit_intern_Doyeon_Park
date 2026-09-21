#include "camera_ui_pkg/camera_node/camera_node.hpp"

#include <cv_bridge/cv_bridge.hpp>
#include <std_msgs/msg/header.hpp>

#include <chrono>
#include <functional>
#include <stdexcept>

CameraNode::CameraNode()
: Node("camera_node")
{
  this->declare_parameter<int>("camera_index", 0);
  this->declare_parameter<double>("publish_hz", 30.0);
  this->declare_parameter<std::string>("image_topic", "/camera/image");

  camera_index_ = this->get_parameter("camera_index").as_int();
  publish_hz_ = this->get_parameter("publish_hz").as_double();
  image_topic_ = this->get_parameter("image_topic").as_string();

  image_pub_ =
    this->create_publisher<sensor_msgs::msg::Image>(
            image_topic_, 10);

  cap_.open(camera_index_);

  if (!cap_.isOpened()) {
    throw std::runtime_error("카메라를 열 수 없습니다.");
  }

  auto period =
    std::chrono::duration<double>(1.0 / publish_hz_);

  timer_ = this->create_wall_timer(
        std::chrono::duration_cast<std::chrono::milliseconds>(period),
        std::bind(&CameraNode::timerCallback, this));
}

void CameraNode::timerCallback()
{
  cv::Mat frame;

  cap_ >> frame;

  if (frame.empty()) {
    RCLCPP_WARN(this->get_logger(), "카메라 프레임을 읽지 못했습니다.");
    return;
  }

  auto msg =
    cv_bridge::CvImage(
            std_msgs::msg::Header(),
            "bgr8",
            frame).toImageMsg();

  msg->header.stamp = this->now();

  image_pub_->publish(*msg);
}

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  rclcpp::spin(
        std::make_shared<CameraNode>());

  rclcpp::shutdown();

  return 0;
}
