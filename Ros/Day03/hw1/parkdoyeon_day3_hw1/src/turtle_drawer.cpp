#include "parkdoyeon_day3_hw1/turtle_drawer.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

const double PI = 3.14159265358979323846;

// 각도를 -PI ~ PI 범위로 맞추는 함수
double normalizeAngle(double angle)
{
    while (angle > PI)
    {
        angle -= 2.0 * PI;
    }

    while (angle < -PI)
    {
        angle += 2.0 * PI;
    }

    return angle;
}

TurtleDrawer::TurtleDrawer()
: Node("turtle_drawer"),
  pose_received_(false)
{
    // 거북이 속도 명령 Publisher
    cmd_vel_pub_ =
        this->create_publisher<geometry_msgs::msg::Twist>(
            "/turtle1/cmd_vel", 10);

    // 거북이 현재 위치 Subscriber
    pose_sub_ =
        this->create_subscription<turtlesim::msg::Pose>(
            "/turtle1/pose",
            10,
            std::bind(
                &TurtleDrawer::poseCallback,
                this,
                std::placeholders::_1));

    // 펜 설정 서비스
    set_pen_client_ =
        this->create_client<turtlesim::srv::SetPen>(
            "/turtle1/set_pen");

    // 위치 이동 서비스
    teleport_client_ =
        this->create_client<turtlesim::srv::TeleportAbsolute>(
            "/turtle1/teleport_absolute");

    // 화면 지우기 서비스
    clear_client_ =
        this->create_client<std_srvs::srv::Empty>(
            "/clear");

        // 펜 색상과 굵기 Parameter 선언
        this->declare_parameter("pen_r", 255);
        this->declare_parameter("pen_g", 0);
        this->declare_parameter("pen_b", 0);
        this->declare_parameter("pen_width", 3);

        // Parameter 값 가져오기
        pen_r_ = this->get_parameter("pen_r").as_int();
        pen_g_ = this->get_parameter("pen_g").as_int();
        pen_b_ = this->get_parameter("pen_b").as_int();
        pen_width_ = this->get_parameter("pen_width").as_int();

        std::cout
            << "Pen RGB : "
            << pen_r_ << ", "
            << pen_g_ << ", "
            << pen_b_
            << " / Width : "
            << pen_width_
            << std::endl;
}

void TurtleDrawer::poseCallback(
    const turtlesim::msg::Pose::SharedPtr msg)
{
    current_pose_ = *msg;
    pose_received_ = true;
}

void TurtleDrawer::moveDistance(
    double distance,
    double speed)
{
    // pose를 아직 못 받았다면 받을 때까지 기다림
    while (rclcpp::ok() && !pose_received_)
    {
        rclcpp::spin_some(shared_from_this());
        std::this_thread::sleep_for(10ms);
    }

    double start_x = current_pose_.x;
    double start_y = current_pose_.y;
    double start_theta = current_pose_.theta;

    double target_distance = std::abs(distance);

    while (rclcpp::ok())
    {
        rclcpp::spin_some(shared_from_this());

        double dx = current_pose_.x - start_x;
        double dy = current_pose_.y - start_y;

        double moved =
            std::sqrt(dx * dx + dy * dy);

        double remaining =
            target_distance - moved;

        if (remaining <= 0.02)
        {
            break;
        }

        geometry_msgs::msg::Twist msg;

        double current_speed = speed;

        // 목표 지점에 가까워지면 천천히 이동
        if (remaining < 0.5)
        {
            current_speed = remaining * 2.0;

            if (current_speed < 0.15)
            {
                current_speed = 0.15;
            }
        }

        if (distance < 0.0)
        {
            current_speed = -current_speed;
        }

        msg.linear.x = current_speed;

        // 직진 중 방향이 조금 틀어지면 보정
        double angle_error =
            normalizeAngle(
                start_theta - current_pose_.theta);

        msg.angular.z = angle_error * 2.0;

        cmd_vel_pub_->publish(msg);

        std::this_thread::sleep_for(20ms);
    }

    stop();
}

void TurtleDrawer::rotateAngle(
    double angle,
    double max_speed)
{
    while (rclcpp::ok() && !pose_received_)
    {
        rclcpp::spin_some(shared_from_this());
        std::this_thread::sleep_for(10ms);
    }

    double previous_theta =
        current_pose_.theta;

    double rotated_angle = 0.0;

    while (rclcpp::ok())
    {
        rclcpp::spin_some(shared_from_this());

        double delta =
            normalizeAngle(
                current_pose_.theta - previous_theta);

        rotated_angle += delta;

        previous_theta =
            current_pose_.theta;

        double remaining =
            std::abs(angle) -
            std::abs(rotated_angle);

        if (remaining <= 0.01)
        {
            break;
        }

        geometry_msgs::msg::Twist msg;

        double speed = max_speed;

        // 목표 각도에 가까워지면 천천히 회전
        if (remaining < 0.4)
        {
            speed = remaining * 2.0;

            if (speed < 0.15)
            {
                speed = 0.15;
            }
        }

        if (angle < 0.0)
        {
            speed = -speed;
        }

        msg.angular.z = speed;

        cmd_vel_pub_->publish(msg);

        std::this_thread::sleep_for(20ms);
    }

    stop();
}

void TurtleDrawer::moveFor(
    double linear_speed,
    double angular_speed,
    double seconds)
{
    geometry_msgs::msg::Twist msg;

    msg.linear.x = linear_speed;
    msg.angular.z = angular_speed;

    auto start =
        std::chrono::steady_clock::now();

    while (rclcpp::ok())
    {
        auto now =
            std::chrono::steady_clock::now();

        double elapsed =
            std::chrono::duration<double>(
                now - start).count();

        if (elapsed >= seconds)
        {
            break;
        }

        cmd_vel_pub_->publish(msg);

        rclcpp::spin_some(shared_from_this());

        std::this_thread::sleep_for(20ms);
    }

    stop();
}

void TurtleDrawer::stop()
{
    geometry_msgs::msg::Twist msg;

    msg.linear.x = 0.0;
    msg.angular.z = 0.0;

    // 확실하게 멈추도록 몇 번 전송
    for (int i = 0; i < 3; i++)
    {
        cmd_vel_pub_->publish(msg);
        std::this_thread::sleep_for(20ms);
    }
}

void TurtleDrawer::setPen(
    int r,
    int g,
    int b,
    int width)
{
    while (!set_pen_client_->wait_for_service(1s))
    {
        std::cout
            << "set_pen service 기다리는 중..."
            << std::endl;
    }

    auto request =
        std::make_shared<
            turtlesim::srv::SetPen::Request>();

    request->r = r;
    request->g = g;
    request->b = b;
    request->width = width;
    request->off = 0;

    auto future =
        set_pen_client_->async_send_request(request);

    rclcpp::spin_until_future_complete(
        shared_from_this(),
        future);
}

void TurtleDrawer::teleport(
    float x,
    float y,
    float theta)
{
    // 이동할 때 선이 생기지 않도록 펜 끄기
    while (!set_pen_client_->wait_for_service(1s))
    {
        std::cout
            << "set_pen service 기다리는 중..."
            << std::endl;
    }

    auto pen_request =
        std::make_shared<
            turtlesim::srv::SetPen::Request>();

    pen_request->r = 0;
    pen_request->g = 0;
    pen_request->b = 0;
    pen_request->width = 1;
    pen_request->off = 1;

    auto pen_future =
        set_pen_client_->async_send_request(
            pen_request);

    rclcpp::spin_until_future_complete(
        shared_from_this(),
        pen_future);

    while (!teleport_client_->wait_for_service(1s))
    {
        std::cout
            << "teleport service 기다리는 중..."
            << std::endl;
    }

    auto request =
        std::make_shared<
            turtlesim::srv::TeleportAbsolute::Request>();

    request->x = x;
    request->y = y;
    request->theta = theta;

    auto future =
        teleport_client_->async_send_request(request);

    rclcpp::spin_until_future_complete(
        shared_from_this(),
        future);

    pose_received_ = false;

    // 순간이동 후 새로운 pose 다시 받기
    while (rclcpp::ok() && !pose_received_)
    {
        rclcpp::spin_some(shared_from_this());
        std::this_thread::sleep_for(10ms);
    }
}

void TurtleDrawer::clearAfterDelay()
{
    std::cout
        << "1초 후 그림을 지웁니다."
        << std::endl;

    std::this_thread::sleep_for(1s);

    while (!clear_client_->wait_for_service(1s))
    {
        std::cout
            << "clear service 기다리는 중..."
            << std::endl;
    }

    auto request =
        std::make_shared<
            std_srvs::srv::Empty::Request>();

    auto future =
        clear_client_->async_send_request(request);

    rclcpp::spin_until_future_complete(
        shared_from_this(),
        future);
}

void TurtleDrawer::drawTriangle()
{
    std::cout
        << "삼각형 그리기"
        << std::endl;

    teleport(2.0, 8.0, 0.0);

    // 빨간색, 굵기 3
    setPen(pen_r_, pen_g_, pen_b_, pen_width_);

    for (int i = 0; i < 3; i++)
    {
        moveDistance(2.5, 1.5);

        // 바깥쪽 회전각 120도
        rotateAngle(
            2.0 * PI / 3.0,
            1.2);
    }
}

void TurtleDrawer::drawSquare()
{
    std::cout
        << "사각형 그리기"
        << std::endl;

    teleport(2.0, 2.0, 0.0);

    // 초록색, 굵기 5
    setPen(pen_r_, pen_g_, pen_b_, pen_width_);
    for (int i = 0; i < 4; i++)
    {
        moveDistance(2.5, 1.5);

        // 90도 회전
        rotateAngle(
            PI / 2.0,
            1.2);
    }
}

void TurtleDrawer::drawCircle()
{
    std::cout
        << "원 그리기"
        << std::endl;

    teleport(8.0, 5.5, 0.0);

    // 파란색, 굵기 7
    setPen(pen_r_, pen_g_, pen_b_, pen_width_);;

    // 선속도 1.5, 각속도 1.0
    // 약 2PI초 동안 이동하면 한 바퀴
    moveFor(
        1.5,
        1.0,
        2.0 * PI);
}

void TurtleDrawer::run()
{
    char key;

    while (rclcpp::ok())
    {
        std::cout << std::endl;
        std::cout
            << "============================"
            << std::endl;

        std::cout
            << "W : 삼각형"
            << std::endl;

        std::cout
            << "A : 사각형"
            << std::endl;

        std::cout
            << "S : 원"
            << std::endl;

        std::cout
            << "D : 모든 도형 그리기"
            << std::endl;

        std::cout
            << "Q : 종료"
            << std::endl;

        std::cout
            << "============================"
            << std::endl;

        std::cout << "입력 : ";

        std::cin >> key;

        if (key == 'w' || key == 'W')
        {
            drawTriangle();
            clearAfterDelay();
        }

        else if (key == 'a' || key == 'A')
        {
            drawSquare();
            clearAfterDelay();
        }

        else if (key == 's' || key == 'S')
        {
            drawCircle();
            clearAfterDelay();
        }

        else if (key == 'd' || key == 'D')
        {
            drawTriangle();
            drawSquare();
            drawCircle();

            clearAfterDelay();
        }

        else if (key == 'q' || key == 'Q')
        {
            std::cout
                << "프로그램 종료"
                << std::endl;

            break;
        }

        else
        {
            std::cout
                << "잘못된 키입니다."
                << std::endl;
        }
    }
}
