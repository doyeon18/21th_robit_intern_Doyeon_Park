#include "parkdoyeon_day2_hw1/vector_subscriber.hpp"

#include <functional>
#include <sstream>
#include <string>

VectorSubscriber::VectorSubscriber()
: Node("vector_subscriber")
{
  subscriber_ =
    this->create_subscription<
    parkdoyeon_day2_interfaces::msg::IntVector>(
    "vector_topic",
    10,
    std::bind(
      &VectorSubscriber::vector_callback,
      this,
      std::placeholders::_1));
}

void VectorSubscriber::vector_callback(
  const parkdoyeon_day2_interfaces::msg::IntVector::SharedPtr message)
{
  std::ostringstream output;

  for (std::size_t i = 0; i < message->data.size(); ++i) {
    if (i > 0) {
      output << ", ";
    }

    output << message->data[i];
  }

  const std::string text = output.str();

  RCLCPP_INFO(
    this->get_logger(),
    "Received: [%s]",
    text.c_str());
}