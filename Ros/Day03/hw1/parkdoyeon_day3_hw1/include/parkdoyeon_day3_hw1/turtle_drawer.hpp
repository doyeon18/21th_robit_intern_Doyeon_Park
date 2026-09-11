#ifndef TURTLE_DRAWER_HPP
#define TURTLE_DRAWER_HPP

#include <rclcpp/rclcpp.hpp>

#include <geometry_msgs/msg/twist.hpp>

#include <turtlesim/msg/pose.hpp>
#include <turtlesim/srv/set_pen.hpp>
#include <turtlesim/srv/teleport_absolute.hpp>

#include <std_srvs/srv/empty.hpp>

class TurtleDrawer : public rclcpp::Node
{
public:
    TurtleDrawer();

    void run();

private:
    // 거북이 위치 정보 콜백
    void poseCallback(
        const turtlesim::msg::Pose::SharedPtr msg);

    // 정확한 거리만큼 직진
    void moveDistance(
        double distance,
        double speed);

    // 정확한 각도만큼 회전
    void rotateAngle(
        double angle,
        double max_speed);

    // 원 그릴 때 사용
    void moveFor(
        double linear_speed,
        double angular_speed,
        double seconds);

    void stop();

    void setPen(
        int r,
        int g,
        int b,
        int width);

    void teleport(
        float x,
        float y,
        float theta);

    // 1초 후 그림 삭제
    void clearAfterDelay();

    void drawTriangle();
    void drawSquare();
    void drawCircle();

    rclcpp::Publisher<
        geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;

    rclcpp::Subscription<
        turtlesim::msg::Pose>::SharedPtr pose_sub_;

    rclcpp::Client<
        turtlesim::srv::SetPen>::SharedPtr set_pen_client_;

    rclcpp::Client<
        turtlesim::srv::TeleportAbsolute>::SharedPtr teleport_client_;

    rclcpp::Client<
        std_srvs::srv::Empty>::SharedPtr clear_client_;

    turtlesim::msg::Pose current_pose_;

    bool pose_received_;

    int pen_r_;
    int pen_g_;
    int pen_b_;
    int pen_width_;
};

#endif
