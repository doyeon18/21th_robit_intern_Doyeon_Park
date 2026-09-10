#ifndef PARKDOYEON_DAY2_HW2__TURTLE_GUI_HPP_
#define PARKDOYEON_DAY2_HW2__TURTLE_GUI_HPP_

#include <deque>
#include <memory>
#include <string>

#include <QMainWindow>

#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_srvs/srv/empty.hpp>
#include <turtlesim/msg/pose.hpp>
#include <turtlesim/srv/set_pen.hpp>
#include <turtlesim/srv/teleport_absolute.hpp>

class QLabel;
class QPushButton;
class QTimer;

class TurtleGui : public QMainWindow
{
  Q_OBJECT

public:
  explicit TurtleGui(
    const std::shared_ptr<rclcpp::Node> & node,
    QWidget * parent = nullptr);
  ~TurtleGui() override;

private slots:
  void startTriangle();
  void startSquare();
  void startCircle();
  void startAllShapes();
  void stopDrawing();
  void clearCanvas();
  void processRosAndCommands();

private:
  enum class CommandType
  {
    SET_PEN,
    TELEPORT,
    MOVE,
    ROTATE,
    ARC
  };

  struct Command
  {
    CommandType type;
    double value1{0.0};
    double value2{0.0};
    double value3{0.0};
    int red{0};
    int green{0};
    int blue{0};
    int width{1};
    bool pen_off{false};
  };

  void createUi();
  void createRosInterfaces();
  void createShortcuts();

  void poseCallback(const turtlesim::msg::Pose::SharedPtr message);
  void publishVelocity(double linear_x, double angular_z);
  void resetProgram(const QString & status_text);
  bool beginProgram(const QString & name);

  void enqueuePen(int red, int green, int blue, int width, bool off);
  void enqueueTeleport(double x, double y, double theta);
  void enqueueMove(double distance, double speed);
  void enqueueRotate(double angle, double maximum_speed);
  void enqueueArc(double linear_speed, double angular_speed, double angle);
  void enqueueTriangle();
  void enqueueSquare();
  void enqueueCircle();

  void startCurrentCommand();
  void updateCurrentCommand();
  void finishServiceCommand(std::size_t program_id);
  void finishMotionCommand();
  static double normalizeAngle(double angle);

  std::shared_ptr<rclcpp::Node> node_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_publisher_;
  rclcpp::Subscription<turtlesim::msg::Pose>::SharedPtr pose_subscription_;
  rclcpp::Client<turtlesim::srv::SetPen>::SharedPtr set_pen_client_;
  rclcpp::Client<turtlesim::srv::TeleportAbsolute>::SharedPtr teleport_client_;
  rclcpp::Client<std_srvs::srv::Empty>::SharedPtr clear_client_;

  QLabel * linear_value_label_;
  QLabel * angular_value_label_;
  QLabel * pose_value_label_;
  QLabel * status_value_label_;
  QTimer * control_timer_;

  turtlesim::msg::Pose current_pose_;
  bool pose_received_{false};
  bool teleport_pose_pending_{false};
  bool program_running_{false};
  bool command_active_{false};
  std::size_t program_id_{0};
  std::deque<Command> command_queue_;
  Command current_command_{};

  double command_start_x_{0.0};
  double command_start_y_{0.0};
  double command_start_theta_{0.0};
  double previous_theta_{0.0};
  double accumulated_angle_{0.0};
  double teleport_target_x_{0.0};
  double teleport_target_y_{0.0};
  double teleport_target_theta_{0.0};
};

#endif  // PARKDOYEON_DAY2_HW2__TURTLE_GUI_HPP_
