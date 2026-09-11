#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/float64.hpp>

#include <chrono>
#include <string>

using namespace std::chrono_literals;

class VehicleNode : public rclcpp::Node
{
public:
    VehicleNode()
    : Node("vehicle_node"),
      light_state_("RED"),
      vehicle_speed_(0.0),
      current_speed_(0.0),
      position_(0.0),
      deceleration_(10.0)
    {
        this->declare_parameter("vehicle_speed", 20.0);

        vehicle_speed_ =
            this->get_parameter("vehicle_speed").as_double();

        light_sub_ =
            this->create_subscription<std_msgs::msg::String>(
                "/traffic_light_state",
                10,
                std::bind(
                    &VehicleNode::lightCallback,
                    this,
                    std::placeholders::_1));

        position_pub_ =
            this->create_publisher<std_msgs::msg::Float64>(
                "/vehicle_position",
                10);

        speed_pub_ =
            this->create_publisher<std_msgs::msg::Float64>(
                "/vehicle_speed",
                10);

        timer_ =
            this->create_wall_timer(
                20ms,
                std::bind(
                    &VehicleNode::timerCallback,
                    this));
    }

private:
    void lightCallback(
        const std_msgs::msg::String::SharedPtr msg)
    {
        light_state_ = msg->data;
    }

    void timerCallback()
    {
        if (light_state_ == "GREEN")
        {
            current_speed_ = vehicle_speed_;
        }
        else if (light_state_ == "YELLOW")
        {
            current_speed_ -= deceleration_ * time_step_;

            if (current_speed_ < 0.0)
            {
                current_speed_ = 0.0;
            }
        }
        else  // RED 또는 잘못된 신호
        {
            current_speed_ = 0.0;
        }

        position_ += current_speed_ * time_step_;

        if (position_ >= 100.0)
        {
            position_ = 0.0;
        }

        std_msgs::msg::Float64 msg;
        msg.data = position_;

        position_pub_->publish(msg);

        std_msgs::msg::Float64 speed_msg;
        speed_msg.data = current_speed_;

        speed_pub_->publish(speed_msg);
    }

    std::string light_state_;

    double vehicle_speed_;
    double current_speed_;
    double position_;
    double deceleration_;
    const double time_step_ = 0.02;

    rclcpp::Subscription<
        std_msgs::msg::String>::SharedPtr light_sub_;

    rclcpp::Publisher<
        std_msgs::msg::Float64>::SharedPtr position_pub_;

    rclcpp::Publisher<
        std_msgs::msg::Float64>::SharedPtr speed_pub_;

    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    rclcpp::spin(
        std::make_shared<VehicleNode>());

    rclcpp::shutdown();

    return 0;
}
