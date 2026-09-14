#include <atomic>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "std_msgs/msg/int32.hpp"

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class CounterSubscriber : public rclcpp_lifecycle::LifecycleNode
{
public:
  CounterSubscriber() : LifecycleNode("counter_subscriber") {}

private:
  CallbackReturn on_configure(const rclcpp_lifecycle::State &) override
  {
    const auto qos = rclcpp::QoS(rclcpp::KeepLast(10)).reliable().durability_volatile();
    subscription_ = create_subscription<std_msgs::msg::Int32>(
      "count", qos,
      [this](const std_msgs::msg::Int32 & message) {
        // 일반 Subscription은 LifecycleNode의 상태에 따라 자동 중단되지 않는다.
        if (active_.load()) {
          RCLCPP_INFO(get_logger(), "받은 숫자: %d", message.data);
        }
      });
    RCLCPP_INFO(get_logger(), "설정 완료: 활성화 대기 중");
    return CallbackReturn::SUCCESS;
  }

  CallbackReturn on_activate(const rclcpp_lifecycle::State &) override
  {
    active_.store(true);
    RCLCPP_INFO(get_logger(), "활성화 완료: /count 처리 시작");
    return CallbackReturn::SUCCESS;
  }

  CallbackReturn on_deactivate(const rclcpp_lifecycle::State &) override
  {
    active_.store(false);
    RCLCPP_INFO(get_logger(), "비활성화 완료: 수신 처리 중단");
    return CallbackReturn::SUCCESS;
  }

  CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override
  {
    active_.store(false);
    subscription_.reset();
    RCLCPP_INFO(get_logger(), "자원 정리 완료");
    return CallbackReturn::SUCCESS;
  }

  CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override
  {
    active_.store(false);
    subscription_.reset();
    RCLCPP_INFO(get_logger(), "노드 종료");
    return CallbackReturn::SUCCESS;
  }

  std::atomic<bool> active_{false};
  rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr subscription_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CounterSubscriber>()->get_node_base_interface());
  rclcpp::shutdown();
  return 0;
}
