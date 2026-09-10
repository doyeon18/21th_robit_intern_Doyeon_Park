#include "parkdoyeon_day2_hw2/turtle_gui.hpp"

#include <algorithm>
#include <cmath>
#include <functional>

#include <QApplication>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kControlPeriodSeconds = 0.02;
}

TurtleGui::TurtleGui(
  const std::shared_ptr<rclcpp::Node> & node,
  QWidget * parent)
: QMainWindow(parent),
  node_(node),
  linear_value_label_(nullptr),
  angular_value_label_(nullptr),
  pose_value_label_(nullptr),
  status_value_label_(nullptr),
  control_timer_(nullptr)
{
  createUi();
  createRosInterfaces();
  createShortcuts();

  control_timer_ = new QTimer(this);
  control_timer_->setInterval(
    static_cast<int>(kControlPeriodSeconds * 1000.0));
  connect(
    control_timer_, &QTimer::timeout,
    this, &TurtleGui::processRosAndCommands);
  control_timer_->start();
}

TurtleGui::~TurtleGui()
{
  ++program_id_;
  command_queue_.clear();
  publishVelocity(0.0, 0.0);
  rclcpp::spin_some(node_);
}

void TurtleGui::createUi()
{
  setWindowTitle(QStringLiteral("ROS 2 turtlesim 조종기"));
  setMinimumSize(620, 430);

  auto * central_widget = new QWidget(this);
  auto * main_layout = new QVBoxLayout(central_widget);
  main_layout->setContentsMargins(24, 24, 24, 24);
  main_layout->setSpacing(16);

  auto * title_label = new QLabel(QStringLiteral("Turtlesim Shape Controller"));
  title_label->setObjectName(QStringLiteral("titleLabel"));
  auto * help_label = new QLabel(
    QStringLiteral("키보드 W/A/S/D 또는 아래 버튼으로 도형을 그립니다."));
  help_label->setObjectName(QStringLiteral("helpLabel"));

  main_layout->addWidget(title_label);
  main_layout->addWidget(help_label);

  auto * button_group = new QGroupBox(QStringLiteral("도형 선택"));
  auto * button_layout = new QGridLayout(button_group);
  button_layout->setSpacing(10);

  auto * triangle_button = new QPushButton(QStringLiteral("W\n삼각형"));
  auto * square_button = new QPushButton(QStringLiteral("A\n사각형"));
  auto * circle_button = new QPushButton(QStringLiteral("S\n원"));
  auto * all_button = new QPushButton(QStringLiteral("D\n전체"));
  auto * stop_button = new QPushButton(QStringLiteral("정지 (Space)"));
  auto * clear_button = new QPushButton(QStringLiteral("화면 지우기"));

  const auto prepare_button = [](QPushButton * button) {
      button->setMinimumHeight(64);
      button->setFocusPolicy(Qt::NoFocus);
    };
  prepare_button(triangle_button);
  prepare_button(square_button);
  prepare_button(circle_button);
  prepare_button(all_button);
  prepare_button(stop_button);
  prepare_button(clear_button);

  button_layout->addWidget(triangle_button, 0, 0);
  button_layout->addWidget(square_button, 0, 1);
  button_layout->addWidget(circle_button, 0, 2);
  button_layout->addWidget(all_button, 0, 3);
  button_layout->addWidget(stop_button, 1, 0, 1, 2);
  button_layout->addWidget(clear_button, 1, 2, 1, 2);

  connect(triangle_button, &QPushButton::clicked, this, &TurtleGui::startTriangle);
  connect(square_button, &QPushButton::clicked, this, &TurtleGui::startSquare);
  connect(circle_button, &QPushButton::clicked, this, &TurtleGui::startCircle);
  connect(all_button, &QPushButton::clicked, this, &TurtleGui::startAllShapes);
  connect(stop_button, &QPushButton::clicked, this, &TurtleGui::stopDrawing);
  connect(clear_button, &QPushButton::clicked, this, &TurtleGui::clearCanvas);

  main_layout->addWidget(button_group);

  auto * value_group = new QGroupBox(QStringLiteral("ROS 2 상태"));
  auto * value_layout = new QGridLayout(value_group);
  value_layout->setHorizontalSpacing(24);
  value_layout->setVerticalSpacing(10);

  linear_value_label_ = new QLabel(QStringLiteral("0.000 m/s"));
  angular_value_label_ = new QLabel(QStringLiteral("0.000 rad/s"));
  pose_value_label_ = new QLabel(QStringLiteral("수신 대기 중"));
  status_value_label_ = new QLabel(QStringLiteral("turtlesim 연결 대기 중"));

  value_layout->addWidget(new QLabel(QStringLiteral("cmd_vel linear.x")), 0, 0);
  value_layout->addWidget(linear_value_label_, 0, 1);
  value_layout->addWidget(new QLabel(QStringLiteral("cmd_vel angular.z")), 1, 0);
  value_layout->addWidget(angular_value_label_, 1, 1);
  value_layout->addWidget(new QLabel(QStringLiteral("현재 pose")), 2, 0);
  value_layout->addWidget(pose_value_label_, 2, 1);
  value_layout->addWidget(new QLabel(QStringLiteral("동작 상태")), 3, 0);
  value_layout->addWidget(status_value_label_, 3, 1);

  main_layout->addWidget(value_group);
  main_layout->addStretch();
  setCentralWidget(central_widget);

  setStyleSheet(QStringLiteral(
      "QMainWindow { background: #f4f7fb; }"
      "QLabel#titleLabel { color: #17233c; font-size: 24px; font-weight: 700; }"
      "QLabel#helpLabel { color: #55627a; font-size: 13px; }"
      "QGroupBox { background: white; border: 1px solid #dbe2ee;"
      " border-radius: 10px; margin-top: 12px; padding: 14px;"
      " color: #17233c; font-weight: 600; }"
      "QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 5px; }"
      "QPushButton { background: #4263eb; color: white; border: none;"
      " border-radius: 8px; padding: 10px; font-size: 14px; font-weight: 600; }"
      "QPushButton:hover { background: #364fc7; }"
      "QPushButton:pressed { background: #2742a7; }"));
}

void TurtleGui::createRosInterfaces()
{
  cmd_vel_publisher_ = node_->create_publisher<geometry_msgs::msg::Twist>(
    "/turtle1/cmd_vel", 10);

  pose_subscription_ = node_->create_subscription<turtlesim::msg::Pose>(
    "/turtle1/pose", 10,
    std::bind(&TurtleGui::poseCallback, this, std::placeholders::_1));

  set_pen_client_ = node_->create_client<turtlesim::srv::SetPen>(
    "/turtle1/set_pen");
  teleport_client_ = node_->create_client<turtlesim::srv::TeleportAbsolute>(
    "/turtle1/teleport_absolute");
  clear_client_ = node_->create_client<std_srvs::srv::Empty>("/clear");
}

void TurtleGui::createShortcuts()
{
  const auto add_shortcut = [this](int key, void (TurtleGui::* slot)()) {
      auto * shortcut = new QShortcut(QKeySequence(key), this);
      shortcut->setContext(Qt::ApplicationShortcut);
      connect(shortcut, &QShortcut::activated, this, slot);
    };

  add_shortcut(Qt::Key_W, &TurtleGui::startTriangle);
  add_shortcut(Qt::Key_A, &TurtleGui::startSquare);
  add_shortcut(Qt::Key_S, &TurtleGui::startCircle);
  add_shortcut(Qt::Key_D, &TurtleGui::startAllShapes);
  add_shortcut(Qt::Key_Space, &TurtleGui::stopDrawing);
}

void TurtleGui::poseCallback(const turtlesim::msg::Pose::SharedPtr message)
{
  current_pose_ = *message;
  pose_value_label_->setText(
    QStringLiteral("x %1   y %2   theta %3")
    .arg(current_pose_.x, 0, 'f', 2)
    .arg(current_pose_.y, 0, 'f', 2)
    .arg(current_pose_.theta, 0, 'f', 2));

  if (teleport_pose_pending_) {
    const double dx = current_pose_.x - teleport_target_x_;
    const double dy = current_pose_.y - teleport_target_y_;
    const double angle_error = normalizeAngle(
      current_pose_.theta - teleport_target_theta_);

    const bool position_matches = std::sqrt(dx * dx + dy * dy) < 0.05;
    const bool angle_matches = std::abs(angle_error) < 0.05;

    if (!position_matches || !angle_matches) {
      pose_received_ = false;
      return;
    }

    teleport_pose_pending_ = false;
  }

  pose_received_ = true;

  if (!program_running_) {
    status_value_label_->setText(QStringLiteral("준비 완료"));
  }
}

void TurtleGui::publishVelocity(double linear_x, double angular_z)
{
  geometry_msgs::msg::Twist message;
  message.linear.x = linear_x;
  message.angular.z = angular_z;
  cmd_vel_publisher_->publish(message);

  linear_value_label_->setText(
    QStringLiteral("%1 m/s").arg(linear_x, 0, 'f', 3));
  angular_value_label_->setText(
    QStringLiteral("%1 rad/s").arg(angular_z, 0, 'f', 3));
}

void TurtleGui::resetProgram(const QString & status_text)
{
  ++program_id_;
  command_queue_.clear();
  program_running_ = false;
  command_active_ = false;
  teleport_pose_pending_ = false;
  publishVelocity(0.0, 0.0);
  status_value_label_->setText(status_text);
}

bool TurtleGui::beginProgram(const QString & name)
{
  if (program_running_) {
    status_value_label_->setText(QStringLiteral("현재 도형이 끝난 후 다시 선택하세요"));
    return false;
  }

  resetProgram(QStringLiteral("준비 중"));
  program_running_ = true;
  status_value_label_->setText(name + QStringLiteral(" 그리는 중"));
  return true;
}

void TurtleGui::enqueuePen(int red, int green, int blue, int width, bool off)
{
  Command command;
  command.type = CommandType::SET_PEN;
  command.red = red;
  command.green = green;
  command.blue = blue;
  command.width = width;
  command.pen_off = off;
  command_queue_.push_back(command);
}

void TurtleGui::enqueueTeleport(double x, double y, double theta)
{
  Command command;
  command.type = CommandType::TELEPORT;
  command.value1 = x;
  command.value2 = y;
  command.value3 = theta;
  command_queue_.push_back(command);
}

void TurtleGui::enqueueMove(double distance, double speed)
{
  Command command;
  command.type = CommandType::MOVE;
  command.value1 = distance;
  command.value2 = speed;
  command_queue_.push_back(command);
}

void TurtleGui::enqueueRotate(double angle, double maximum_speed)
{
  Command command;
  command.type = CommandType::ROTATE;
  command.value1 = angle;
  command.value2 = maximum_speed;
  command_queue_.push_back(command);
}

void TurtleGui::enqueueArc(double linear_speed, double angular_speed, double angle)
{
  Command command;
  command.type = CommandType::ARC;
  command.value1 = linear_speed;
  command.value2 = angular_speed;
  command.value3 = angle;
  command_queue_.push_back(command);
}

void TurtleGui::enqueueTriangle()
{
  enqueuePen(0, 0, 0, 1, true);
  enqueueTeleport(2.0, 8.0, 0.0);
  enqueuePen(255, 0, 0, 3, false);
  for (int side = 0; side < 3; ++side) {
    enqueueMove(2.5, 1.5);
    enqueueRotate(2.0 * kPi / 3.0, 1.2);
  }
}

void TurtleGui::enqueueSquare()
{
  enqueuePen(0, 0, 0, 1, true);
  enqueueTeleport(2.0, 2.0, 0.0);
  enqueuePen(0, 180, 70, 5, false);
  for (int side = 0; side < 4; ++side) {
    enqueueMove(2.5, 1.5);
    enqueueRotate(kPi / 2.0, 1.2);
  }
}

void TurtleGui::enqueueCircle()
{
  enqueuePen(0, 0, 0, 1, true);
  enqueueTeleport(8.0, 4.0, 0.0);
  enqueuePen(30, 100, 255, 7, false);
  enqueueArc(1.2, 0.8, 2.0 * kPi);
}

void TurtleGui::startTriangle()
{
  if (!beginProgram(QStringLiteral("삼각형"))) {
    return;
  }
  enqueueTriangle();
}

void TurtleGui::startSquare()
{
  if (!beginProgram(QStringLiteral("사각형"))) {
    return;
  }
  enqueueSquare();
}

void TurtleGui::startCircle()
{
  if (!beginProgram(QStringLiteral("원"))) {
    return;
  }
  enqueueCircle();
}

void TurtleGui::startAllShapes()
{
  if (!beginProgram(QStringLiteral("전체 도형"))) {
    return;
  }
  enqueueTriangle();
  enqueueSquare();
  enqueueCircle();
}

void TurtleGui::stopDrawing()
{
  resetProgram(QStringLiteral("사용자가 정지함"));
}

void TurtleGui::clearCanvas()
{
  resetProgram(QStringLiteral("화면 지우는 중"));

  if (!clear_client_->service_is_ready()) {
    status_value_label_->setText(QStringLiteral("/clear 서비스를 찾을 수 없음"));
    return;
  }

  const std::size_t request_program_id = program_id_;
  auto request = std::make_shared<std_srvs::srv::Empty::Request>();
  clear_client_->async_send_request(
    request,
    [this, request_program_id](rclcpp::Client<std_srvs::srv::Empty>::SharedFuture) {
      if (request_program_id == program_id_) {
        status_value_label_->setText(QStringLiteral("화면 지우기 완료"));
      }
    });
}

void TurtleGui::processRosAndCommands()
{
  if (!rclcpp::ok()) {
    QApplication::quit();
    return;
  }

  rclcpp::spin_some(node_);

  if (!program_running_) {
    return;
  }

  if (!command_active_) {
    if (command_queue_.empty()) {
      program_running_ = false;
      publishVelocity(0.0, 0.0);
      status_value_label_->setText(QStringLiteral("도형 그리기 완료"));
      return;
    }

    current_command_ = command_queue_.front();
    command_queue_.pop_front();
    command_active_ = true;
    startCurrentCommand();
    return;
  }

  updateCurrentCommand();
}

void TurtleGui::startCurrentCommand()
{
  const std::size_t request_program_id = program_id_;

  if (current_command_.type == CommandType::SET_PEN) {
    if (!set_pen_client_->service_is_ready()) {
      command_queue_.push_front(current_command_);
      command_active_ = false;
      status_value_label_->setText(QStringLiteral("set_pen 서비스 대기 중"));
      return;
    }

    auto request = std::make_shared<turtlesim::srv::SetPen::Request>();
    request->r = static_cast<uint8_t>(std::clamp(current_command_.red, 0, 255));
    request->g = static_cast<uint8_t>(std::clamp(current_command_.green, 0, 255));
    request->b = static_cast<uint8_t>(std::clamp(current_command_.blue, 0, 255));
    request->width = static_cast<uint8_t>(std::clamp(current_command_.width, 1, 255));
    request->off = current_command_.pen_off ? 1 : 0;
    set_pen_client_->async_send_request(
      request,
      [this, request_program_id](rclcpp::Client<turtlesim::srv::SetPen>::SharedFuture) {
        finishServiceCommand(request_program_id);
      });
    return;
  }

  if (current_command_.type == CommandType::TELEPORT) {
    if (!teleport_client_->service_is_ready()) {
      command_queue_.push_front(current_command_);
      command_active_ = false;
      status_value_label_->setText(QStringLiteral("teleport 서비스 대기 중"));
      return;
    }

    auto request = std::make_shared<turtlesim::srv::TeleportAbsolute::Request>();
    request->x = static_cast<float>(current_command_.value1);
    request->y = static_cast<float>(current_command_.value2);
    request->theta = static_cast<float>(current_command_.value3);

    teleport_target_x_ = current_command_.value1;
    teleport_target_y_ = current_command_.value2;
    teleport_target_theta_ = current_command_.value3;
    teleport_pose_pending_ = true;
    pose_received_ = false;
    teleport_client_->async_send_request(
      request,
      [this, request_program_id](
        rclcpp::Client<turtlesim::srv::TeleportAbsolute>::SharedFuture) {
        finishServiceCommand(request_program_id);
      });
    return;
  }

  if (!pose_received_) {
    command_queue_.push_front(current_command_);
    command_active_ = false;
    status_value_label_->setText(QStringLiteral("turtle pose 대기 중"));
    return;
  }

  command_start_x_ = current_pose_.x;
  command_start_y_ = current_pose_.y;
  command_start_theta_ = current_pose_.theta;
  previous_theta_ = current_pose_.theta;
  accumulated_angle_ = 0.0;
}

void TurtleGui::updateCurrentCommand()
{
  if (
    current_command_.type == CommandType::SET_PEN ||
    current_command_.type == CommandType::TELEPORT)
  {
    return;
  }

  if (!pose_received_) {
    publishVelocity(0.0, 0.0);
    return;
  }

  if (current_command_.type == CommandType::MOVE) {
    const double dx = current_pose_.x - command_start_x_;
    const double dy = current_pose_.y - command_start_y_;
    const double moved_distance = std::sqrt(dx * dx + dy * dy);
    const double remaining = std::abs(current_command_.value1) - moved_distance;

    if (remaining <= 0.02) {
      finishMotionCommand();
      return;
    }

    double speed = std::min(std::abs(current_command_.value2), remaining * 2.0);
    speed = std::max(speed, 0.15);
    if (current_command_.value1 < 0.0) {
      speed = -speed;
    }

    const double heading_error = normalizeAngle(
      command_start_theta_ - current_pose_.theta);
    publishVelocity(speed, heading_error * 2.0);
    return;
  }

  const double delta_angle = normalizeAngle(current_pose_.theta - previous_theta_);
  accumulated_angle_ += delta_angle;
  previous_theta_ = current_pose_.theta;

  if (current_command_.type == CommandType::ROTATE) {
    const double remaining =
      std::abs(current_command_.value1) - std::abs(accumulated_angle_);
    if (remaining <= 0.01) {
      finishMotionCommand();
      return;
    }

    double speed = std::min(std::abs(current_command_.value2), remaining * 2.0);
    speed = std::max(speed, 0.15);
    if (current_command_.value1 < 0.0) {
      speed = -speed;
    }
    publishVelocity(0.0, speed);
    return;
  }

  if (current_command_.type == CommandType::ARC) {
    const double remaining =
      std::abs(current_command_.value3) - std::abs(accumulated_angle_);
    if (remaining <= 0.02) {
      finishMotionCommand();
      return;
    }

    const double angular_speed =
      (remaining < 0.25) ? current_command_.value2 * (remaining / 0.25) :
      current_command_.value2;
    const double scale = angular_speed / current_command_.value2;
    publishVelocity(current_command_.value1 * scale, angular_speed);
  }
}

void TurtleGui::finishServiceCommand(std::size_t request_program_id)
{
  if (request_program_id != program_id_) {
    return;
  }
  command_active_ = false;
}

void TurtleGui::finishMotionCommand()
{
  publishVelocity(0.0, 0.0);
  command_active_ = false;
}

double TurtleGui::normalizeAngle(double angle)
{
  while (angle > kPi) {
    angle -= 2.0 * kPi;
  }
  while (angle < -kPi) {
    angle += 2.0 * kPi;
  }
  return angle;
}
