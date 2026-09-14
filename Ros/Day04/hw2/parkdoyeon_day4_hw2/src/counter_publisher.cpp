#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "std_msgs/msg/int32.hpp"

using namespace std::chrono_literals;
using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class CounterPublisher : public rclcpp_lifecycle::LifecycleNode
{
public:
  CounterPublisher() : LifecycleNode("counter_publisher") {}

private:
  CallbackReturn on_configure(const rclcpp_lifecycle::State &) override
  {
    // 두 노드가 통신할 수 있도록 동일한 QoS를 사용한다.
    const auto qos = rclcpp::QoS(rclcpp::KeepLast(10)).reliable().durability_volatile();
    publisher_ = create_publisher<std_msgs::msg::Int32>("count", qos);
    timer_ = create_wall_timer(1s, std::bind(&CounterPublisher::publish_count, this));
    timer_->cancel();
    count_ = 0;
    RCLCPP_INFO(get_logger(), "설정 완료: 활성화 대기 중");
    return CallbackReturn::SUCCESS;
  }

  CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override
  {
    const auto result = LifecycleNode::on_activate(state);
    if (result == CallbackReturn::SUCCESS) {
      timer_->reset();
      RCLCPP_INFO(get_logger(), "활성화 완료: /count 발행 시작");
    }
    return result;
  }

  CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override
  {
    timer_->cancel();
    RCLCPP_INFO(get_logger(), "비활성화 완료: 발행 중단");
    return LifecycleNode::on_deactivate(state);
  }

  CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override
  {
    timer_.reset();
    publisher_.reset();
    count_ = 0;
    RCLCPP_INFO(get_logger(), "자원 정리 완료");
    return CallbackReturn::SUCCESS;
  }

  CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override
  {
    timer_.reset();
    publisher_.reset();
    RCLCPP_INFO(get_logger(), "노드 종료");
    return CallbackReturn::SUCCESS;
  }

  void publish_count()
  {
    if (!publisher_ || !publisher_->is_activated()) {
      return;
    }
    std_msgs::msg::Int32 message;
    message.data = ++count_;
    publisher_->publish(message);
    RCLCPP_INFO(get_logger(), "발행한 숫자: %d", message.data);
  }

  rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::Int32>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  int32_t count_{0};
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CounterPublisher>()->get_node_base_interface());
  rclcpp::shutdown();
  return 0;
}
