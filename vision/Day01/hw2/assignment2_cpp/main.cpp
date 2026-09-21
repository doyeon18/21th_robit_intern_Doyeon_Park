#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>

struct HsvRange {
    cv::Scalar lower;
    cv::Scalar upper;
};

void drawColorBoxes(const cv::Mat& hsv,
                    cv::Mat& result,
                    const std::vector<HsvRange>& ranges,
                    const cv::Scalar& boxColor,
                    const std::string& label,
                    double minArea) {
    cv::Mat mask = cv::Mat::zeros(hsv.size(), CV_8UC1);

    // 빨간색처럼 범위가 둘 이상이면 각 마스크를 OR로 합친다.
    for (const auto& range : ranges) {
        cv::Mat partialMask;
        cv::inRange(hsv, range.lower, range.upper, partialMask);
        cv::bitwise_or(mask, partialMask, mask);
    }

    // 작은 점 노이즈를 제거하고 객체 내부의 작은 구멍을 메운다.
    const cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours) {
        if (cv::contourArea(contour) < minArea) {
            continue;
        }

        const cv::Rect box = cv::boundingRect(contour);
        cv::rectangle(result, box, boxColor, 4);

        const cv::Point textPosition(box.x, std::max(box.y - 10, 25));
        cv::putText(result, label, textPosition,
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, boxColor, 2);
    }
}

int main(int argc, char* argv[]) {
    const std::string inputPath = argc >= 2 ? argv[1] : "input.png";
    const std::string outputPath = argc >= 3 ? argv[2] : "result.png";
    const bool showWindow = !(argc >= 4 && std::string(argv[3]) == "--no-gui");

    const cv::Mat image = cv::imread(inputPath);
    if (image.empty()) {
        std::cerr << "이미지를 열 수 없습니다: " << inputPath << '\n';
        return 1;
    }

    cv::Mat blurred;
    cv::GaussianBlur(image, blurred, cv::Size(5, 5), 0);

    cv::Mat hsv;
    cv::cvtColor(blurred, hsv, cv::COLOR_BGR2HSV);

    cv::Mat result = image.clone();
    constexpr double minArea = 2000.0;

    // 이 HSV 값은 제공된 사진을 기준으로 조정한 시작값이다.
    drawColorBoxes(
        hsv, result,
        {{{0, 90, 60}, {10, 255, 255}},
         {{170, 90, 60}, {179, 255, 255}}},
        {0, 0, 255}, "RED", minArea);

    drawColorBoxes(
        hsv, result,
        {{{100, 90, 50}, {115, 255, 255}}},
        {255, 0, 0}, "BLUE", minArea);

    drawColorBoxes(
        hsv, result,
        {{{45, 80, 30}, {75, 255, 255}}},
        {0, 255, 0}, "GREEN", minArea);

    if (!cv::imwrite(outputPath, result)) {
        std::cerr << "결과 이미지를 저장할 수 없습니다: " << outputPath << '\n';
        return 1;
    }

    std::cout << "결과 저장 완료: " << outputPath << '\n';
    if (showWindow) {
        cv::imshow("Color object detection", result);
        cv::waitKey(0);
        cv::destroyAllWindows();
    }
    return 0;
}
