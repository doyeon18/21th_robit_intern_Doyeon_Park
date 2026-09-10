#ifndef PARKDOYEON_ROS_RCLCPP_PKG__PUBLISHER_HPP_
#define PARKDOYEON_ROS_RCLCPP_PKG__PUBLISHER_HPP_

#include "rclcpp/rclcpp.hpp"

#include "std_msgs/msg/int32.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/float32.hpp"
#include "std_msgs/msg/bool.hpp"

class PublisherNode : public rclcpp::Node
{
public:
    PublisherNode();

private:
    void timer_callback();

    rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr int_publisher_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr string_publisher_;
    rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr float_publisher_;
    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr bool_publisher_;

    rclcpp::TimerBase::SharedPtr timer_;

    int count_;
};

#endif
