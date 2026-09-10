#include "parkdoyeon_day2_hw1/vector_publisher.hpp"

#include <sstream>
#include <string>

VectorPublisher::VectorPublisher()
: Node("vector_publisher")
{
  publisher_ =
    this->create_publisher<
    parkdoyeon_day2_interfaces::msg::IntVector>(
    "vector_topic", 10);
}

void VectorPublisher::publish_vector(
  const std::vector<std::int32_t> & values)
{
  parkdoyeon_day2_interfaces::msg::IntVector message;

  message.data = values;
  publisher_->publish(message);

  std::ostringstream output;

  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      output << ", ";
    }

    output << values[i];
  }

  const std::string text = output.str();

  RCLCPP_INFO(
    this->get_logger(),
    "Published: [%s]",
    text.c_str());
}