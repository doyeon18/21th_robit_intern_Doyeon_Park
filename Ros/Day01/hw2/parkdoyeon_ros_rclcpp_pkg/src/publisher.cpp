#include "parkdoyeon_ros_rclcpp_pkg/publisher.hpp"

#include <chrono>
#include <functional>

using namespace std::chrono_literals;

PublisherNode::PublisherNode()
: Node("cpp_publisher"), count_(0)
{
    int_publisher_ =
        this->create_publisher<std_msgs::msg::Int32>(
            "cpp_int_topic", 10);

    string_publisher_ =
        this->create_publisher<std_msgs::msg::String>(
            "cpp_string_topic", 10);

    float_publisher_ =
        this->create_publisher<std_msgs::msg::Float32>(
            "cpp_float_topic", 10);

    bool_publisher_ =
        this->create_publisher<std_msgs::msg::Bool>(
            "cpp_bool_topic", 10);

    timer_ = this->create_wall_timer(
        1s,
        std::bind(&PublisherNode::timer_callback, this));
}

void PublisherNode::timer_callback()
{
    std_msgs::msg::Int32 int_msg;
    std_msgs::msg::String string_msg;
    std_msgs::msg::Float32 float_msg;
    std_msgs::msg::Bool bool_msg;

    int_msg.data = count_;
    string_msg.data = "Hello from C++";
    float_msg.data = 3.14f;
    bool_msg.data = (count_ % 2 == 0);

    int_publisher_->publish(int_msg);
    string_publisher_->publish(string_msg);
    float_publisher_->publish(float_msg);
    bool_publisher_->publish(bool_msg);

    RCLCPP_INFO(
        this->get_logger(),
        "int: %d, string: %s, float: %.2f, bool: %s",
        int_msg.data,
        string_msg.data.c_str(),
        float_msg.data,
        bool_msg.data ? "true" : "false");

    count_++;
}

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<PublisherNode>();

    rclcpp::spin(node);

    rclcpp::shutdown();

    return 0;
}