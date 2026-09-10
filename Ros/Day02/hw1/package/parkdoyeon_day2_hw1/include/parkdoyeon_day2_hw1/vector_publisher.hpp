#ifndef PARKDOYEON_DAY2_HW1__VECTOR_PUBLISHER_HPP_
#define PARKDOYEON_DAY2_HW1__VECTOR_PUBLISHER_HPP_

#include <cstdint>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "parkdoyeon_day2_interfaces/msg/int_vector.hpp"

class VectorPublisher : public rclcpp::Node
{
public:
  VectorPublisher();

  void publish_vector(
    const std::vector<std::int32_t> & values);

private:
  rclcpp::Publisher<
    parkdoyeon_day2_interfaces::msg::IntVector>::SharedPtr publisher_;
};

#endif