#include "parkdoyeon_hw3/turtle_drawer.hpp"

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<TurtleDrawer>();

    node->run();

    rclcpp::shutdown();

    return 0;
}