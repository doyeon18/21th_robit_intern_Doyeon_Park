#include "parkdoyeon_ros_rclcpp_pkg/subscriber.hpp"

#include <functional>

SubscriberNode::SubscriberNode()
: Node("cpp_subscriber")
{
    cpp_int_subscriber_ =
        this->create_subscription<std_msgs::msg::Int32>(
            "cpp_int_topic",
            10,
            std::bind(
                &SubscriberNode::cpp_int_callback,
                this,
                std::placeholders::_1));

    cpp_string_subscriber_ =
        this->create_subscription<std_msgs::msg::String>(
            "cpp_string_topic",
            10,
            std::bind(
                &SubscriberNode::cpp_string_callback,
                this,
                std::placeholders::_1));

    cpp_float_subscriber_ =
        this->create_subscription<std_msgs::msg::Float32>(
            "cpp_float_topic",
            10,
            std::bind(
                &SubscriberNode::cpp_float_callback,
                this,
                std::placeholders::_1));

    cpp_bool_subscriber_ =
        this->create_subscription<std_msgs::msg::Bool>(
            "cpp_bool_topic",
            10,
            std::bind(
                &SubscriberNode::cpp_bool_callback,
                this,
                std::placeholders::_1));

    py_int_subscriber_ =
        this->create_subscription<std_msgs::msg::Int32>(
            "py_int_topic",
            10,
            std::bind(
                &SubscriberNode::py_int_callback,
                this,
                std::placeholders::_1));

    py_string_subscriber_ =
        this->create_subscription<std_msgs::msg::String>(
            "py_string_topic",
            10,
            std::bind(
                &SubscriberNode::py_string_callback,
                this,
                std::placeholders::_1));

    py_float_subscriber_ =
        this->create_subscription<std_msgs::msg::Float32>(
            "py_float_topic",
            10,
            std::bind(
                &SubscriberNode::py_float_callback,
                this,
                std::placeholders::_1));

    py_bool_subscriber_ =
        this->create_subscription<std_msgs::msg::Bool>(
            "py_bool_topic",
            10,
            std::bind(
                &SubscriberNode::py_bool_callback,
                this,
                std::placeholders::_1));
}

void SubscriberNode::cpp_int_callback(
    const std_msgs::msg::Int32::SharedPtr msg)
{
    RCLCPP_INFO(
        this->get_logger(),
        "[C++ -> C++] int: %d",
        msg->data);
}

void SubscriberNode::cpp_string_callback(
    const std_msgs::msg::String::SharedPtr msg)
{
    RCLCPP_INFO(
        this->get_logger(),
        "[C++ -> C++] string: %s",
        msg->data.c_str());
}

void SubscriberNode::cpp_float_callback(
    const std_msgs::msg::Float32::SharedPtr msg)
{
    RCLCPP_INFO(
        this->get_logger(),
        "[C++ -> C++] float: %.2f",
        msg->data);
}

void SubscriberNode::cpp_bool_callback(
    const std_msgs::msg::Bool::SharedPtr msg)
{
    RCLCPP_INFO(
        this->get_logger(),
        "[C++ -> C++] bool: %s",
        msg->data ? "true" : "false");
}

void SubscriberNode::py_int_callback(
    const std_msgs::msg::Int32::SharedPtr msg)
{
    RCLCPP_INFO(
        this->get_logger(),
        "[Python -> C++] int: %d",
        msg->data);
}

void SubscriberNode::py_string_callback(
    const std_msgs::msg::String::SharedPtr msg)
{
    RCLCPP_INFO(
        this->get_logger(),
        "[Python -> C++] string: %s",
        msg->data.c_str());
}

void SubscriberNode::py_float_callback(
    const std_msgs::msg::Float32::SharedPtr msg)
{
    RCLCPP_INFO(
        this->get_logger(),
        "[Python -> C++] float: %.2f",
        msg->data);
}

void SubscriberNode::py_bool_callback(
    const std_msgs::msg::Bool::SharedPtr msg)
{
    RCLCPP_INFO(
        this->get_logger(),
        "[Python -> C++] bool: %s",
        msg->data ? "true" : "false");
}

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<SubscriberNode>();

    rclcpp::spin(node);

    rclcpp::shutdown();

    return 0;
}