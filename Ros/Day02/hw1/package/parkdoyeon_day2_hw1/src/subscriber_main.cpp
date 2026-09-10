#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "parkdoyeon_day2_hw1/vector_subscriber.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<VectorSubscriber>();

  RCLCPP_INFO(
    node->get_logger(),
    "Waiting for messages on vector_topic...");

  rclcpp::spin(node);

  rclcpp::shutdown();

  return 0;
}
