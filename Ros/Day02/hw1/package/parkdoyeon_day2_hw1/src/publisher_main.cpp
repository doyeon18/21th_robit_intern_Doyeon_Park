#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "parkdoyeon_day2_hw1/vector_publisher.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<VectorPublisher>();

  std::cout << "공백으로 구분한 정수를 입력하세요." << std::endl;
  std::cout << "예: 10 20 -3" << std::endl;
  std::cout << "종료하려면 q를 입력하세요." << std::endl;

  std::string line;

  while (rclcpp::ok()) {
    std::cout << "> ";
    std::getline(std::cin, line);

    if (line == "q") {
      break;
    }

    std::istringstream input(line);
    std::vector<std::int32_t> values;
    std::int32_t value;

    while (input >> value) {
      values.push_back(value);
    }

    if (!input.eof()) {
      std::cout << "정수만 입력해주세요." << std::endl;
      continue;
    }

    if (values.empty()) {
      std::cout << "한 개 이상의 정수를 입력해주세요." << std::endl;
      continue;
    }

    node->publish_vector(values);
    rclcpp::spin_some(node);
  }

  rclcpp::shutdown();

  return 0;
}