#include "parkdoyeon_day2_hw3/talker_window.hpp"

#include <QApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace
{
constexpr int kRosTimerIntervalMs = 20;
}

TalkerWindow::TalkerWindow(
  const std::shared_ptr<rclcpp::Node> & node,
  QWidget * parent)
: QMainWindow(parent),
  node_(node),
  message_input_(nullptr),
  status_label_(nullptr),
  ros_timer_(nullptr)
{
  createUi();
  createRosInterfaces();

  ros_timer_ = new QTimer(this);
  ros_timer_->setInterval(kRosTimerIntervalMs);
  connect(ros_timer_, &QTimer::timeout, this, &TalkerWindow::processRos);
  ros_timer_->start();
}

void TalkerWindow::createUi()
{
  setWindowTitle(QStringLiteral("ROS 2 Qt Talker"));
  setMinimumSize(560, 300);

  auto * central_widget = new QWidget(this);
  auto * main_layout = new QVBoxLayout(central_widget);
  main_layout->setContentsMargins(24, 24, 24, 24);
  main_layout->setSpacing(16);

  auto * title_label = new QLabel(QStringLiteral("Talker"));
  title_label->setObjectName(QStringLiteral("titleLabel"));

  auto * help_label = new QLabel(
    QStringLiteral("전송할 문자열을 입력하고 Publish 버튼을 누르세요."));
  help_label->setObjectName(QStringLiteral("helpLabel"));

  auto * input_group = new QGroupBox(QStringLiteral("메시지 발행"));
  auto * input_layout = new QHBoxLayout(input_group);
  input_layout->setSpacing(10);

  message_input_ = new QLineEdit;
  message_input_->setPlaceholderText(QStringLiteral("메시지를 입력하세요"));
  message_input_->setClearButtonEnabled(true);

  auto * publish_button = new QPushButton(QStringLiteral("Publish"));
  publish_button->setMinimumWidth(110);

  input_layout->addWidget(message_input_);
  input_layout->addWidget(publish_button);

  status_label_ = new QLabel(QStringLiteral("아직 발행한 메시지가 없습니다."));
  status_label_->setObjectName(QStringLiteral("statusLabel"));
  status_label_->setWordWrap(true);

  connect(
    publish_button, &QPushButton::clicked,
    this, &TalkerWindow::publishMessage);
  connect(
    message_input_, &QLineEdit::returnPressed,
    this, &TalkerWindow::publishMessage);

  main_layout->addWidget(title_label);
  main_layout->addWidget(help_label);
  main_layout->addWidget(input_group);
  main_layout->addWidget(status_label_);
  main_layout->addStretch();
  setCentralWidget(central_widget);

  setStyleSheet(QStringLiteral(
      "QMainWindow { background: #f4f7fb; }"
      "QLabel#titleLabel { color: #17233c; font-size: 26px; font-weight: 700; }"
      "QLabel#helpLabel { color: #55627a; font-size: 13px; }"
      "QLabel#statusLabel { background: #e9f2ff; color: #1c4b82;"
      " border-radius: 8px; padding: 12px; }"
      "QGroupBox { background: white; border: 1px solid #dbe2ee;"
      " border-radius: 10px; margin-top: 12px; padding: 14px;"
      " color: #17233c; font-weight: 600; }"
      "QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 5px; }"
      "QLineEdit { border: 1px solid #bcc7d8; border-radius: 7px;"
      " padding: 10px; background: white; color: #17233c; }"
      "QLineEdit:focus { border: 2px solid #4263eb; }"
      "QPushButton { background: #4263eb; color: white; border: none;"
      " border-radius: 8px; padding: 11px; font-weight: 600; }"
      "QPushButton:hover { background: #364fc7; }"
      "QPushButton:pressed { background: #2742a7; }"));

  message_input_->setFocus();
}

void TalkerWindow::createRosInterfaces()
{
  publisher_ = node_->create_publisher<std_msgs::msg::String>("/qt_chat", 10);
}

void TalkerWindow::publishMessage()
{
  const QString input_text = message_input_->text().trimmed();
  if (input_text.isEmpty()) {
    status_label_->setText(QStringLiteral("문자열을 먼저 입력하세요."));
    message_input_->setFocus();
    return;
  }

  std_msgs::msg::String message;
  message.data = input_text.toStdString();
  publisher_->publish(message);

  ++publish_count_;
  status_label_->setText(
    QStringLiteral("발행 %1회: %2")
    .arg(publish_count_)
    .arg(input_text));

  RCLCPP_INFO(node_->get_logger(), "Published: '%s'", message.data.c_str());
  message_input_->clear();
  message_input_->setFocus();
}

void TalkerWindow::processRos()
{
  if (!rclcpp::ok()) {
    QApplication::quit();
    return;
  }

  rclcpp::spin_some(node_);
}
