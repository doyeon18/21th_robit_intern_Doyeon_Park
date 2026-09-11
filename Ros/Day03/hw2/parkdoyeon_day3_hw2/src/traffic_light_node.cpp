#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include <chrono>
#include <string>

using namespace std::chrono_literals;

class TrafficLightNode : public rclcpp::Node
{
public:
    TrafficLightNode()
    : Node("traffic_light_node"),
      state_("RED")
    {
        this->declare_parameter("red_time", 5.0);
        this->declare_parameter("green_time", 5.0);
        this->declare_parameter("yellow_time", 2.0);

        red_time_ = this->get_parameter("red_time").as_double();
        green_time_ = this->get_parameter("green_time").as_double();
        yellow_time_ = this->get_parameter("yellow_time").as_double();

        publisher_ =
            this->create_publisher<std_msgs::msg::String>(
                "/traffic_light_state", 10);

        last_change_time_ = this->now();

        timer_ =
            this->create_wall_timer(
                100ms,
                std::bind(
                    &TrafficLightNode::timerCallback,
                    this));
    }

private:
    void timerCallback()
    {
        double elapsed =
            (this->now() - last_change_time_).seconds();

        if (state_ == "RED" && elapsed >= red_time_)
        {
            state_ = "GREEN";
            last_change_time_ = this->now();
        }
        else if (state_ == "GREEN" && elapsed >= green_time_)
        {
            state_ = "YELLOW";
            last_change_time_ = this->now();
        }
        else if (state_ == "YELLOW" && elapsed >= yellow_time_)
        {
            state_ = "RED";
            last_change_time_ = this->now();
        }

        std_msgs::msg::String msg;
        msg.data = state_;

        publisher_->publish(msg);
    }

    std::string state_;

    double red_time_;
    double green_time_;
    double yellow_time_;

    rclcpp::Time last_change_time_;

    rclcpp::Publisher<
        std_msgs::msg::String>::SharedPtr publisher_;

    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    rclcpp::spin(
        std::make_shared<TrafficLightNode>());

    rclcpp::shutdown();

    return 0;
}