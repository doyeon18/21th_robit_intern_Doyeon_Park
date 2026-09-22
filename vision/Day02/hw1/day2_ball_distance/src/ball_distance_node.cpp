#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <cv_bridge/cv_bridge.hpp>
#include <insta360_usb_cam/msg/insta_pan_tilt_msgs.hpp>
#include <insta360_usb_cam/msg/pan_tilt_status_msgs.hpp>
#include <opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>

namespace {

constexpr double kPi = 3.14159265358979323846;

struct Detection {
  cv::Point2f center;
  cv::Point contact;
  float radius_px{};
  double area_px{};
  double circularity{};
  std::vector<cv::Point> contour;
};

double nanValue() { return std::numeric_limits<double>::quiet_NaN(); }

std::string distanceText(const std::string &label, double value) {
  if (!std::isfinite(value)) {
    return label + ": N/A";
  }
  return cv::format("%s: %.3f m", label.c_str(), value);
}

}  // namespace

class BallDistanceNode : public rclcpp::Node {
 public:
  BallDistanceNode() : Node("ball_distance_node") {
    image_topic_ = declare_parameter<std::string>(
        "image_topic", "/camera1/camera/compressed_image");
    camera_info_topic_ =
        declare_parameter<std::string>("camera_info_topic", "/camera1/info");
    debug_topic_ = declare_parameter<std::string>(
        "debug_image_topic", "/ball_distance/debug_image");
    pan_tilt_command_topic_ = declare_parameter<std::string>(
        "pan_tilt_command_topic", "/camera1/pan_tilt");
    pan_tilt_status_topic_ = declare_parameter<std::string>(
        "pan_tilt_status_topic", "/camera1/pan_tilt_status");

    h_min_ = declare_parameter<int>("yellow_h_min", 20);
    h_max_ = declare_parameter<int>("yellow_h_max", 40);
    s_min_ = declare_parameter<int>("yellow_s_min", 100);
    s_max_ = declare_parameter<int>("yellow_s_max", 255);
    v_min_ = declare_parameter<int>("yellow_v_min", 80);
    v_max_ = declare_parameter<int>("yellow_v_max", 255);
    min_area_ = declare_parameter<double>("min_area_px", 500.0);
    min_circularity_ = declare_parameter<double>("min_circularity", 0.55);
    kernel_size_ = declare_parameter<int>("morphology_kernel_size", 5);
    use_undistort_ = declare_parameter<bool>("use_undistort", true);

    ball_diameter_m_ = declare_parameter<double>("ball_diameter_m", 0.0);
    camera_height_m_ = declare_parameter<double>("camera_height_m", 0.0);
    camera_pitch_deg_ = declare_parameter<double>("camera_pitch_deg", 0.0);
    auto_tilt_camera_ = declare_parameter<bool>("auto_tilt_camera", true);
    auto_tracking_ = declare_parameter<bool>("auto_tracking", true);
    tracking_deadband_px_ =
        declare_parameter<int>("tracking_deadband_px", 25);
    tracking_reacquire_px_ =
        declare_parameter<int>("tracking_reacquire_px", 60);
    tracking_max_step_deg_ =
        declare_parameter<double>("tracking_max_step_deg", 1.5);
    tracking_gain_ = declare_parameter<double>("tracking_gain", 0.35);
    tracking_smoothing_alpha_ =
        declare_parameter<double>("tracking_smoothing_alpha", 0.35);
    tracking_command_period_ms_ =
        declare_parameter<int>("tracking_command_period_ms", 200);
    tracking_lock_cycles_required_ =
        declare_parameter<int>("tracking_lock_cycles", 5);
    tracking_lost_timeout_ms_ =
        declare_parameter<int>("tracking_lost_timeout_ms", 750);
    ground_truth_m_ = declare_parameter<double>("ground_truth_m", 0.0);

    validateParameters();

    auto qos = rclcpp::SensorDataQoS();
    image_sub_ = create_subscription<sensor_msgs::msg::Image>(
        image_topic_, qos,
        std::bind(&BallDistanceNode::imageCallback, this, std::placeholders::_1));
    camera_info_sub_ = create_subscription<sensor_msgs::msg::CameraInfo>(
        camera_info_topic_, qos,
        std::bind(&BallDistanceNode::cameraInfoCallback, this,
                  std::placeholders::_1));
    pan_tilt_status_sub_ =
        create_subscription<insta360_usb_cam::msg::PanTiltStatusMsgs>(
            pan_tilt_status_topic_, 10,
            std::bind(&BallDistanceNode::panTiltStatusCallback, this,
                      std::placeholders::_1));

    debug_pub_ = create_publisher<sensor_msgs::msg::Image>(debug_topic_, qos);
    result_pub_ = create_publisher<std_msgs::msg::Float64MultiArray>(
        "/ball_distance/results", 10);
    pan_tilt_command_pub_ =
        create_publisher<insta360_usb_cam::msg::InstaPanTiltMsgs>(
            pan_tilt_command_topic_, 10);

    pan_tilt_timer_ = create_wall_timer(
        std::chrono::milliseconds(tracking_command_period_ms_),
        std::bind(&BallDistanceNode::updatePanTiltControl, this));

    RCLCPP_INFO(get_logger(), "Image topic: %s", image_topic_.c_str());
    RCLCPP_INFO(get_logger(), "CameraInfo topic: %s", camera_info_topic_.c_str());
    if (ball_diameter_m_ <= 0.0) {
      RCLCPP_WARN(
          get_logger(),
          "Set ball_diameter_m to a positive measured value. Method A is disabled.");
    }
    if (camera_height_m_ <= 0.0) {
      RCLCPP_WARN(
          get_logger(),
          "Set camera_height_m after measuring the setup. Method B is disabled "
          "until then.");
    }
    if (auto_tracking_) {
      RCLCPP_INFO(get_logger(),
                  "Yellow-ball tracking enabled; camera_pitch_deg is read from "
                  "the gimbal status");
    }
  }

 private:
  void validateParameters() {
    if (h_min_ < 0 || h_max_ > 179 || h_min_ > h_max_) {
      throw std::invalid_argument("yellow hue must satisfy 0 <= min <= max <= 179");
    }
    if (s_min_ < 0 || s_max_ > 255 || s_min_ > s_max_ || v_min_ < 0 ||
        v_max_ > 255 || v_min_ > v_max_) {
      throw std::invalid_argument("yellow saturation/value range is invalid");
    }
    if (min_area_ <= 0.0 || min_circularity_ < 0.0 ||
        min_circularity_ > 1.0) {
      throw std::invalid_argument("area/circularity parameters are invalid");
    }
    if (kernel_size_ < 1) {
      throw std::invalid_argument("morphology_kernel_size must be positive");
    }
    if (kernel_size_ % 2 == 0) {
      ++kernel_size_;
      RCLCPP_WARN(get_logger(), "Using odd morphology kernel size %d", kernel_size_);
    }
    if (camera_pitch_deg_ < 0.0 || camera_pitch_deg_ > 90.0) {
      throw std::invalid_argument(
          "camera_pitch_deg must be between 0 and 90 degrees downward");
    }
    if (tracking_deadband_px_ < 1 ||
        tracking_reacquire_px_ < tracking_deadband_px_ ||
        tracking_max_step_deg_ <= 0.0 || tracking_gain_ <= 0.0 ||
        tracking_gain_ > 1.0 || tracking_smoothing_alpha_ <= 0.0 ||
        tracking_smoothing_alpha_ > 1.0 ||
        tracking_command_period_ms_ < 100 ||
        tracking_lock_cycles_required_ < 1 || tracking_lost_timeout_ms_ < 100) {
      throw std::invalid_argument("automatic tracking parameters are invalid");
    }
  }

  void panTiltStatusCallback(
      const insta360_usb_cam::msg::PanTiltStatusMsgs::SharedPtr msg) {
    pan_tilt_status_ = msg;
    if (auto_tracking_) {
      if (msg->tilt_range[0] <= msg->tilt &&
          msg->tilt <= msg->tilt_range[1]) {
        camera_pitch_deg_ = -static_cast<double>(msg->tilt) / 3600.0;
      }
      return;
    }
    if (!tilt_command_sent_) {
      return;
    }

    const int tolerance = msg->steps[1] > 0 ? msg->steps[1] : 3600;
    if (std::abs(msg->tilt - target_tilt_raw_) <= tolerance) {
      if (!tilt_ready_) {
        RCLCPP_INFO(get_logger(), "Camera tilt reached %.1f degrees downward",
                    camera_pitch_deg_);
      }
      tilt_ready_ = true;
      pan_tilt_timer_->cancel();
    }
  }

  static int32_t quantizeAndClamp(double value, int step, int32_t minimum,
                                  int32_t maximum) {
    const int safe_step = step > 0 ? step : 3600;
    const double quantized =
        std::round(value / static_cast<double>(safe_step)) * safe_step;
    return static_cast<int32_t>(
        std::clamp(quantized, static_cast<double>(minimum),
                   static_cast<double>(maximum)));
  }

  void publishPanTiltCommand(int32_t pan, int32_t tilt) {
    insta360_usb_cam::msg::InstaPanTiltMsgs command;
    command.header.stamp = now();
    command.header.frame_id = "camera";
    command.pan = pan;
    command.tilt = tilt;
    command.pan_deg = static_cast<int32_t>(
        std::lround(static_cast<double>(pan) / 3600.0));
    command.tilt_deg = static_cast<int32_t>(
        std::lround(static_cast<double>(tilt) / 3600.0));
    pan_tilt_command_pub_->publish(command);
  }

  void updateTracking() {
    if (!pan_tilt_status_) {
      tracking_state_ = "WAITING FOR GIMBAL";
      tracking_locked_ = false;
      tracking_lock_cycles_ = 0;
      return;
    }

    const bool valid_gimbal_status =
        pan_tilt_status_->pan_range[0] <= pan_tilt_status_->pan_range[1] &&
        pan_tilt_status_->tilt_range[0] <= pan_tilt_status_->tilt_range[1] &&
        pan_tilt_status_->pan_range[0] <= pan_tilt_status_->pan &&
        pan_tilt_status_->pan <= pan_tilt_status_->pan_range[1] &&
        pan_tilt_status_->tilt_range[0] <= pan_tilt_status_->tilt &&
        pan_tilt_status_->tilt <= pan_tilt_status_->tilt_range[1];
    if (!valid_gimbal_status) {
      tracking_state_ = "INVALID GIMBAL STATUS";
      tracking_locked_ = false;
      tracking_lock_cycles_ = 0;
      RCLCPP_WARN_THROTTLE(
          get_logger(), *get_clock(), 3000,
          "Ignoring out-of-range pan/tilt status: pan=%d tilt=%d",
          pan_tilt_status_->pan, pan_tilt_status_->tilt);
      return;
    }

    const auto now_steady = std::chrono::steady_clock::now();
    const bool detection_fresh = latest_ball_center_.has_value() &&
        (now_steady - last_detection_time_) <=
            std::chrono::milliseconds(tracking_lost_timeout_ms_);
    if (!detection_fresh || latest_fx_ <= 0.0 || latest_fy_ <= 0.0) {
      tracking_state_ = "SEARCHING";
      tracking_locked_ = false;
      tracking_lock_cycles_ = 0;
      return;
    }

    const double error_x = latest_ball_center_->x - latest_cx_;
    const double error_y = latest_ball_center_->y - latest_cy_;
    // Hysteresis prevents a locked camera from repeatedly starting and
    // stopping near the edge of the center box.
    const int active_deadband =
        tracking_locked_ ? tracking_reacquire_px_ : tracking_deadband_px_;
    const bool centered = std::abs(error_x) <= active_deadband &&
                          std::abs(error_y) <= active_deadband;
    if (centered) {
      ++tracking_lock_cycles_;
      if (tracking_lock_cycles_ >= tracking_lock_cycles_required_) {
        tracking_locked_ = true;
        tracking_state_ = "LOCKED";
      } else {
        tracking_locked_ = false;
        tracking_state_ = "STABILIZING";
      }
      return;
    }

    tracking_locked_ = false;
    tracking_lock_cycles_ = 0;
    tracking_state_ = "TRACKING";

    const double max_step_rad = tracking_max_step_deg_ * kPi / 180.0;
    const double yaw_delta = std::clamp(
        tracking_gain_ * std::atan(error_x / latest_fx_), -max_step_rad,
        max_step_rad);
    const double pitch_delta = std::clamp(
        tracking_gain_ * std::atan(error_y / latest_fy_), -max_step_rad,
        max_step_rad);

    const int pan_step = pan_tilt_status_->steps[0] > 0
                             ? pan_tilt_status_->steps[0]
                             : 3600;
    const int tilt_step = pan_tilt_status_->steps[1] > 0
                              ? pan_tilt_status_->steps[1]
                              : 3600;
    const int32_t current_pan = pan_tilt_status_->pan;
    const int32_t current_tilt = pan_tilt_status_->tilt;

    // V4L2 uses positive pan to the right and positive tilt upward.
    const int32_t target_pan = quantizeAndClamp(
        static_cast<double>(current_pan) + yaw_delta * 180.0 / kPi * 3600.0,
        pan_step, pan_tilt_status_->pan_range[0],
        pan_tilt_status_->pan_range[1]);
    const int32_t target_tilt = quantizeAndClamp(
        static_cast<double>(current_tilt) - pitch_delta * 180.0 / kPi * 3600.0,
        tilt_step, pan_tilt_status_->tilt_range[0],
        pan_tilt_status_->tilt_range[1]);

    if (target_pan != current_pan || target_tilt != current_tilt) {
      publishPanTiltCommand(target_pan, target_tilt);
    }
  }

  void updatePanTiltControl() {
    if (auto_tracking_) {
      updateTracking();
      return;
    }
    sendAutomaticTilt();
  }

  void sendAutomaticTilt() {
    if (!auto_tilt_camera_) {
      tilt_ready_ = true;
      pan_tilt_timer_->cancel();
      return;
    }
    if (!pan_tilt_status_) {
      return;
    }

    const int step = pan_tilt_status_->steps[1] > 0
                         ? pan_tilt_status_->steps[1]
                         : 3600;
    const double requested_raw = -camera_pitch_deg_ * 3600.0;
    int32_t target = static_cast<int32_t>(
        std::lround(requested_raw / static_cast<double>(step)) * step);
    target = std::clamp(target, pan_tilt_status_->tilt_range[0],
                        pan_tilt_status_->tilt_range[1]);

    target_tilt_raw_ = target;
    camera_pitch_deg_ = -static_cast<double>(target) / 3600.0;

    publishPanTiltCommand(0, target_tilt_raw_);
    tilt_command_sent_ = true;
    ++tilt_command_attempts_;

    if (tilt_command_attempts_ == 1) {
      RCLCPP_INFO(get_logger(),
                  "Commanding camera to %.1f degrees downward (raw tilt %d)",
                  camera_pitch_deg_, target_tilt_raw_);
    }
    if (tilt_command_attempts_ >= 10 && !tilt_ready_) {
      RCLCPP_WARN(get_logger(),
                  "Camera did not confirm the requested tilt; Method B remains disabled");
      pan_tilt_timer_->cancel();
    }
  }

  void cameraInfoCallback(const sensor_msgs::msg::CameraInfo::SharedPtr msg) {
    if (msg->k[0] <= 0.0 || msg->k[4] <= 0.0 || msg->width == 0 ||
        msg->height == 0) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 3000,
                           "Ignoring invalid CameraInfo");
      return;
    }

    camera_info_ = msg;
    const double center_error_x =
        std::abs(msg->k[2] - static_cast<double>(msg->width) / 2.0);
    const double center_error_y =
        std::abs(msg->k[5] - static_cast<double>(msg->height) / 2.0);
    if (center_error_x > msg->width * 0.20 ||
        center_error_y > msg->height * 0.20) {
      RCLCPP_WARN_THROTTLE(
          get_logger(), *get_clock(), 10000,
          "CameraInfo principal point is far from the image center. The "
          "calibration may have been made at a different resolution.");
    }
  }

  std::pair<cv::Mat, cv::Mat> intrinsicsForFrame(int width, int height) const {
    cv::Mat k = (cv::Mat_<double>(3, 3) <<
        camera_info_->k[0], camera_info_->k[1], camera_info_->k[2],
        camera_info_->k[3], camera_info_->k[4], camera_info_->k[5],
        camera_info_->k[6], camera_info_->k[7], camera_info_->k[8]);

    cv::Mat d(1, static_cast<int>(camera_info_->d.size()), CV_64F);
    for (std::size_t i = 0; i < camera_info_->d.size(); ++i) {
      d.at<double>(0, static_cast<int>(i)) = camera_info_->d[i];
    }

    if (static_cast<int>(camera_info_->width) != width ||
        static_cast<int>(camera_info_->height) != height) {
      const double sx = static_cast<double>(width) / camera_info_->width;
      const double sy = static_cast<double>(height) / camera_info_->height;
      k.at<double>(0, 0) *= sx;
      k.at<double>(0, 2) *= sx;
      k.at<double>(1, 1) *= sy;
      k.at<double>(1, 2) *= sy;

      RCLCPP_WARN_THROTTLE(
          get_logger(), *get_clock(), 5000,
          "Image is %dx%d but CameraInfo is %ux%u; scaling intrinsics. "
          "Recalibration at the actual output resolution is recommended.",
          width, height, camera_info_->width, camera_info_->height);
    }
    return {k, d};
  }

  std::optional<Detection> detectBall(const cv::Mat &bgr, cv::Mat &mask) const {
    cv::Mat blurred;
    cv::GaussianBlur(bgr, blurred, cv::Size(5, 5), 0.0);

    cv::Mat hsv;
    cv::cvtColor(blurred, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(h_min_, s_min_, v_min_),
                cv::Scalar(h_max_, s_max_, v_max_), mask);

    const cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_ELLIPSE, cv::Size(kernel_size_, kernel_size_));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask.clone(), contours, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);

    std::optional<Detection> best;
    for (const auto &contour : contours) {
      const double area = cv::contourArea(contour);
      const double perimeter = cv::arcLength(contour, true);
      if (area < min_area_ || perimeter <= 0.0) {
        continue;
      }

      const double circularity = 4.0 * kPi * area / (perimeter * perimeter);
      if (circularity < min_circularity_) {
        continue;
      }

      cv::Point2f center;
      float radius = 0.0F;
      cv::minEnclosingCircle(contour, center, radius);
      const auto bottom = std::max_element(
          contour.begin(), contour.end(),
          [](const cv::Point &a, const cv::Point &b) { return a.y < b.y; });

      Detection current{center, *bottom, radius, area, circularity, contour};
      if (!best || current.area_px > best->area_px) {
        best = std::move(current);
      }
    }
    return best;
  }

  double sizeDistance(const Detection &detection, const cv::Mat &k) const {
    const double diameter_px = 2.0 * detection.radius_px;
    if (ball_diameter_m_ <= 0.0 || diameter_px <= 0.0) {
      return nanValue();
    }
    return k.at<double>(0, 0) * ball_diameter_m_ / diameter_px;
  }

  double groundDistance(const Detection &detection, const cv::Mat &k) const {
    if (camera_height_m_ <= 0.0 ||
        (auto_tracking_ && !tracking_locked_) ||
        (!auto_tracking_ && auto_tilt_camera_ && !tilt_ready_)) {
      return nanValue();
    }

    const double fy = k.at<double>(1, 1);
    const double cy = k.at<double>(1, 2);
    const double normalized_y = (detection.contact.y - cy) / fy;
    const double ray_down_angle = std::atan(normalized_y);
    const double pitch = camera_pitch_deg_ * kPi / 180.0;
    const double ground_angle = pitch + ray_down_angle;

    if (ground_angle <= 1e-6 || ground_angle >= kPi / 2.0) {
      return nanValue();
    }
    return camera_height_m_ / std::tan(ground_angle);
  }

  void publishResults(double method_a, double method_b) {
    std_msgs::msg::Float64MultiArray result;
    const double truth = ground_truth_m_ > 0.0 ? ground_truth_m_ : nanValue();
    const double error_a = std::isfinite(method_a) && std::isfinite(truth)
                               ? std::abs(method_a - truth)
                               : nanValue();
    const double error_b = std::isfinite(method_b) && std::isfinite(truth)
                               ? std::abs(method_b - truth)
                               : nanValue();
    result.data = {method_a, method_b, truth, error_a, error_b};
    result_pub_->publish(result);
  }

  void drawOverlay(cv::Mat &debug, const Detection &detection, double method_a,
                   double method_b) const {
    if (auto_tracking_) {
      const cv::Point image_center(
          static_cast<int>(std::lround(latest_cx_)),
          static_cast<int>(std::lround(latest_cy_)));
      cv::rectangle(
          debug,
          cv::Rect(image_center.x - tracking_deadband_px_,
                   image_center.y - tracking_deadband_px_,
                   tracking_deadband_px_ * 2, tracking_deadband_px_ * 2),
          tracking_locked_ ? cv::Scalar(0, 255, 0)
                           : cv::Scalar(0, 165, 255),
          2);
      cv::drawMarker(debug, image_center, cv::Scalar(255, 255, 0),
                     cv::MARKER_CROSS, 24, 2);
    }
    cv::drawContours(debug, std::vector<std::vector<cv::Point>>{detection.contour},
                     -1, cv::Scalar(0, 255, 0), 2);
    cv::circle(debug, detection.center,
               static_cast<int>(std::lround(detection.radius_px)),
               cv::Scalar(255, 0, 255), 2);
    cv::circle(debug, detection.contact, 5, cv::Scalar(0, 0, 255), -1);

    cv::putText(debug, distanceText("A(size)", method_a), cv::Point(20, 35),
                cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(255, 255, 255), 2);
    cv::putText(debug, distanceText("B(ground)", method_b), cv::Point(20, 70),
                cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(255, 255, 255), 2);
    cv::putText(debug,
                cv::format("diameter: %.1f px, circularity: %.2f",
                           2.0 * detection.radius_px, detection.circularity),
                cv::Point(20, 105), cv::FONT_HERSHEY_SIMPLEX, 0.65,
                cv::Scalar(255, 255, 255), 2);
    cv::putText(debug,
                cv::format("tilt: %.1f deg down%s", camera_pitch_deg_,
                           auto_tracking_ ? " (measured)" :
                           (auto_tilt_camera_ ? " (auto)" : "")),
                cv::Point(20, 140), cv::FONT_HERSHEY_SIMPLEX, 0.65,
                cv::Scalar(255, 255, 255), 2);
    if (auto_tracking_) {
      cv::putText(debug,
                  cv::format("tracking: %s", tracking_state_.c_str()),
                  cv::Point(20, 175), cv::FONT_HERSHEY_SIMPLEX, 0.65,
                  tracking_locked_ ? cv::Scalar(0, 255, 0)
                                   : cv::Scalar(0, 165, 255),
                  2);
    }

    if (ground_truth_m_ > 0.0) {
      cv::putText(debug, cv::format("truth: %.3f m", ground_truth_m_),
                  cv::Point(20, 210), cv::FONT_HERSHEY_SIMPLEX, 0.65,
                  cv::Scalar(0, 255, 255), 2);
    }
  }

  void imageCallback(const sensor_msgs::msg::Image::ConstSharedPtr msg) {
    cv::Mat frame;
    try {
      frame = cv_bridge::toCvShare(msg, "bgr8")->image;
    } catch (const cv_bridge::Exception &e) {
      RCLCPP_ERROR_THROTTLE(get_logger(), *get_clock(), 3000,
                            "cv_bridge conversion failed: %s", e.what());
      return;
    }

    if (!camera_info_) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 3000,
                           "Waiting for CameraInfo on %s",
                           camera_info_topic_.c_str());
      return;
    }

    auto [k, d] = intrinsicsForFrame(frame.cols, frame.rows);
    cv::Mat working;
    if (use_undistort_ && !d.empty()) {
      cv::undistort(frame, working, k, d, k);
    } else {
      working = frame.clone();
    }

    cv::Mat mask;
    auto detection = detectBall(working, mask);
    cv::Mat debug = working.clone();
    double method_a = nanValue();
    double method_b = nanValue();

    if (detection) {
      if (latest_ball_center_) {
        latest_ball_center_->x = static_cast<float>(
            (1.0 - tracking_smoothing_alpha_) * latest_ball_center_->x +
            tracking_smoothing_alpha_ * detection->center.x);
        latest_ball_center_->y = static_cast<float>(
            (1.0 - tracking_smoothing_alpha_) * latest_ball_center_->y +
            tracking_smoothing_alpha_ * detection->center.y);
      } else {
        latest_ball_center_ = detection->center;
      }
      latest_fx_ = k.at<double>(0, 0);
      latest_fy_ = k.at<double>(1, 1);
      latest_cx_ = k.at<double>(0, 2);
      latest_cy_ = k.at<double>(1, 2);
      last_detection_time_ = std::chrono::steady_clock::now();
      method_a = sizeDistance(*detection, k);
      method_b = groundDistance(*detection, k);
      drawOverlay(debug, *detection, method_a, method_b);
    } else {
      latest_ball_center_.reset();
      cv::putText(debug, "yellow ball: not detected", cv::Point(20, 35),
                  cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(0, 0, 255), 2);
      if (auto_tracking_) {
        cv::putText(debug,
                    cv::format("tracking: %s", tracking_state_.c_str()),
                    cv::Point(20, 70), cv::FONT_HERSHEY_SIMPLEX, 0.65,
                    cv::Scalar(0, 165, 255), 2);
      }
    }

    publishResults(method_a, method_b);
    auto debug_msg = cv_bridge::CvImage(msg->header, "bgr8", debug).toImageMsg();
    debug_pub_->publish(*debug_msg);
  }

  std::string image_topic_;
  std::string camera_info_topic_;
  std::string debug_topic_;
  std::string pan_tilt_command_topic_;
  std::string pan_tilt_status_topic_;
  int h_min_{};
  int h_max_{};
  int s_min_{};
  int s_max_{};
  int v_min_{};
  int v_max_{};
  int kernel_size_{};
  double min_area_{};
  double min_circularity_{};
  bool use_undistort_{};
  bool auto_tilt_camera_{};
  bool auto_tracking_{};
  int tracking_deadband_px_{};
  int tracking_reacquire_px_{};
  double tracking_max_step_deg_{};
  double tracking_gain_{};
  double tracking_smoothing_alpha_{};
  int tracking_command_period_ms_{};
  int tracking_lock_cycles_required_{};
  int tracking_lost_timeout_ms_{};
  int tracking_lock_cycles_{};
  bool tracking_locked_{};
  std::string tracking_state_{"WAITING FOR GIMBAL"};
  std::optional<cv::Point2f> latest_ball_center_;
  double latest_fx_{};
  double latest_fy_{};
  double latest_cx_{};
  double latest_cy_{};
  std::chrono::steady_clock::time_point last_detection_time_{};
  double ball_diameter_m_{};
  double camera_height_m_{};
  double camera_pitch_deg_{};
  double ground_truth_m_{};
  int32_t target_tilt_raw_{};
  int tilt_command_attempts_{};
  bool tilt_command_sent_{};
  bool tilt_ready_{};

  sensor_msgs::msg::CameraInfo::SharedPtr camera_info_;
  insta360_usb_cam::msg::PanTiltStatusMsgs::SharedPtr pan_tilt_status_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr camera_info_sub_;
  rclcpp::Subscription<insta360_usb_cam::msg::PanTiltStatusMsgs>::SharedPtr
      pan_tilt_status_sub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr debug_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr result_pub_;
  rclcpp::Publisher<insta360_usb_cam::msg::InstaPanTiltMsgs>::SharedPtr
      pan_tilt_command_pub_;
  rclcpp::TimerBase::SharedPtr pan_tilt_timer_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  try {
    rclcpp::spin(std::make_shared<BallDistanceNode>());
  } catch (const std::exception &e) {
    std::cerr << "ball_distance_node failed: " << e.what() << '\n';
    rclcpp::shutdown();
    return 1;
  }
  rclcpp::shutdown();
  return 0;
}
