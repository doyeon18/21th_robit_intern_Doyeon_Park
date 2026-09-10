#ifndef PARKDOYEON_ROS_RCLCPP_PKG__SUBSCRIBER_HPP_
#define PARKDOYEON_ROS_RCLCPP_PKG__SUBSCRIBER_HPP_

#include "rclcpp/rclcpp.hpp"

#include "std_msgs/msg/int32.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/float32.hpp"
#include "std_msgs/msg/bool.hpp"

class SubscriberNode : public rclcpp::Node
{
public:
    SubscriberNode();

private:
    void cpp_int_callback(
        const std_msgs::msg::Int32::SharedPtr msg);

    void cpp_string_callback(
        const std_msgs::msg::String::SharedPtr msg);

    void cpp_float_callback(
        const std_msgs::msg::Float32::SharedPtr msg);

    void cpp_bool_callback(
        const std_msgs::msg::Bool::SharedPtr msg);

    void py_int_callback(
        const std_msgs::msg::Int32::SharedPtr msg);

    void py_string_callback(
        const std_msgs::msg::String::SharedPtr msg);

    void py_float_callback(
        const std_msgs::msg::Float32::SharedPtr msg);

    void py_bool_callback(
        const std_msgs::msg::Bool::SharedPtr msg);

    rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr cpp_int_subscriber_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr cpp_string_subscriber_;
    rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr cpp_float_subscriber_;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr cpp_bool_subscriber_;

    rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr py_int_subscriber_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr py_string_subscriber_;
    rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr py_float_subscriber_;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr py_bool_subscriber_;
};

#endif
