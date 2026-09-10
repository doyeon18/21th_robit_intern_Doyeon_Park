#ifndef PARKDOYEON_DAY2_HW1__VECTOR_SUBSCRIBER_HPP_
#define PARKDOYEON_DAY2_HW1__VECTOR_SUBSCRIBER_HPP_

#include "rclcpp/rclcpp.hpp"
#include "parkdoyeon_day2_interfaces/msg/int_vector.hpp"

class VectorSubscriber : public rclcpp::Node
{
public:
  VectorSubscriber();

private:
  void vector_callback(
    const parkdoyeon_day2_interfaces::msg::IntVector::SharedPtr message);

  rclcpp::Subscription<
    parkdoyeon_day2_interfaces::msg::IntVector>::SharedPtr subscriber_;
};

#endif