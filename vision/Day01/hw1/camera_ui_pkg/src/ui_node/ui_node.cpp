#include "camera_ui_pkg/ui_node/ui_node.hpp"

#include <cv_bridge/cv_bridge.hpp>

#include <chrono>
#include <functional>

UiNode::UiNode()
: Node("ui_node")
{
  this->declare_parameter<double>("display_hz", 30.0);
  this->declare_parameter<std::string>("image_topic", "/camera/image");

  display_hz_ = this->get_parameter("display_hz").as_double();
  image_topic_ = this->get_parameter("image_topic").as_string();

  image_sub_ =
    this->create_subscription<sensor_msgs::msg::Image>(
            image_topic_,
            10,
            std::bind(
                &UiNode::imageCallback,
                this,
                std::placeholders::_1));

  auto period =
    std::chrono::duration<double>(1.0 / display_hz_);

  timer_ = this->create_wall_timer(
        std::chrono::duration_cast<std::chrono::milliseconds>(period),
        std::bind(&UiNode::timerCallback, this));

  cv::namedWindow("Camera", cv::WINDOW_AUTOSIZE);
}

void UiNode::imageCallback(
  const sensor_msgs::msg::Image::SharedPtr msg)
{
  cv::Mat image;

  try {
    image = cv_bridge::toCvCopy(
                    msg,
                    "bgr8")->image;
  } catch (const cv_bridge::Exception & e) {
    RCLCPP_ERROR(
            this->get_logger(),
            "cv_bridge 변환 실패: %s",
            e.what());

    return;
  }

  std::lock_guard<std::mutex> lock(image_mutex_);

  latest_image_ = image.clone();
}

void UiNode::timerCallback()
{
  cv::Mat image;

  {
    std::lock_guard<std::mutex> lock(image_mutex_);

    if (latest_image_.empty()) {
      return;
    }

    image = latest_image_.clone();
  }

  cv::imshow("Camera", image);
  cv::waitKey(1);
}

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  rclcpp::spin(
        std::make_shared<UiNode>());

  rclcpp::shutdown();

  cv::destroyAllWindows();

  return 0;
}
