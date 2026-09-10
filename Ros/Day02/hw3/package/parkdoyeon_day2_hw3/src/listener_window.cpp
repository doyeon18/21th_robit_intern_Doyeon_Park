#include "parkdoyeon_day2_hw3/listener_window.hpp"

#include <functional>

#include <QApplication>
#include <QGroupBox>
#include <QLabel>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace
{
constexpr int kRosTimerIntervalMs = 20;
}

ListenerWindow::ListenerWindow(
  const std::shared_ptr<rclcpp::Node> & node,
  QWidget * parent)
: QMainWindow(parent),
  node_(node),
  received_message_label_(nullptr),
  status_label_(nullptr),
  ros_timer_(nullptr)
{
  createUi();
  createRosInterfaces();

  ros_timer_ = new QTimer(this);
  ros_timer_->setInterval(kRosTimerIntervalMs);
  connect(ros_timer_, &QTimer::timeout, this, &ListenerWindow::processRos);
  ros_timer_->start();
}

void ListenerWindow::createUi()
{
  setWindowTitle(QStringLiteral("ROS 2 Qt Listener"));
  setMinimumSize(560, 300);

  auto * central_widget = new QWidget(this);
  auto * main_layout = new QVBoxLayout(central_widget);
  main_layout->setContentsMargins(24, 24, 24, 24);
  main_layout->setSpacing(16);

  auto * title_label = new QLabel(QStringLiteral("Listener"));
  title_label->setObjectName(QStringLiteral("titleLabel"));

  auto * help_label = new QLabel(
    QStringLiteral("/qt_chat 토픽으로 들어오는 문자열을 기다리고 있습니다."));
  help_label->setObjectName(QStringLiteral("helpLabel"));

  auto * message_group = new QGroupBox(QStringLiteral("최근 수신 메시지"));
  auto * message_layout = new QVBoxLayout(message_group);

  received_message_label_ = new QLabel(QStringLiteral("수신 대기 중..."));
  received_message_label_->setObjectName(QStringLiteral("messageLabel"));
  received_message_label_->setAlignment(Qt::AlignCenter);
  received_message_label_->setWordWrap(true);
  received_message_label_->setMinimumHeight(80);
  message_layout->addWidget(received_message_label_);

  status_label_ = new QLabel(QStringLiteral("수신 횟수: 0회"));
  status_label_->setObjectName(QStringLiteral("statusLabel"));

  main_layout->addWidget(title_label);
  main_layout->addWidget(help_label);
  main_layout->addWidget(message_group);
  main_layout->addWidget(status_label_);
  main_layout->addStretch();
  setCentralWidget(central_widget);

  setStyleSheet(QStringLiteral(
      "QMainWindow { background: #f4f7fb; }"
      "QLabel#titleLabel { color: #17233c; font-size: 26px; font-weight: 700; }"
      "QLabel#helpLabel { color: #55627a; font-size: 13px; }"
      "QLabel#messageLabel { background: #ecfdf3; color: #17633a;"
      " border-radius: 8px; padding: 16px; font-size: 18px; font-weight: 600; }"
      "QLabel#statusLabel { color: #55627a; padding: 4px; }"
      "QGroupBox { background: white; border: 1px solid #dbe2ee;"
      " border-radius: 10px; margin-top: 12px; padding: 14px;"
      " color: #17233c; font-weight: 600; }"
      "QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 5px; }"));
}

void ListenerWindow::createRosInterfaces()
{
  subscription_ = node_->create_subscription<std_msgs::msg::String>(
    "/qt_chat", 10,
    std::bind(&ListenerWindow::messageCallback, this, std::placeholders::_1));
}

void ListenerWindow::messageCallback(
  const std_msgs::msg::String::SharedPtr message)
{
  const QString received_text = QString::fromStdString(message->data);
  received_message_label_->setText(received_text);

  ++receive_count_;
  status_label_->setText(
    QStringLiteral("수신 횟수: %1회").arg(receive_count_));

  RCLCPP_INFO(node_->get_logger(), "Received: '%s'", message->data.c_str());
}

void ListenerWindow::processRos()
{
  if (!rclcpp::ok()) {
    QApplication::quit();
    return;
  }

  rclcpp::spin_some(node_);
}
