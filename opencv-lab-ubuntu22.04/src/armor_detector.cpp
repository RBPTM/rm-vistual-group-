#include "armor_detector.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr float kPi = 3.14159265358979323846F;

float angleFromVertical(const cv::RotatedRect& rect) {
    cv::Point2f points[4];
    rect.points(points);

    cv::Point2f longest_edge{};
    float longest_length_squared = 0.0F;
    for (int i = 0; i < 4; ++i) {
        const cv::Point2f edge = points[(i + 1) % 4] - points[i];
        const float length_squared = edge.dot(edge);
        if (length_squared > longest_length_squared) {
            longest_length_squared = length_squared;
            longest_edge = edge;
        }
    }

    if (longest_length_squared <= 0.0F) {
        return 90.0F;
    }
    return std::atan2(std::abs(longest_edge.x), std::abs(longest_edge.y)) *
           180.0F / kPi;
}

std::array<cv::Point, 4> integerBox(const cv::RotatedRect& rect) {
    cv::Point2f points[4];
    rect.points(points);
    std::array<cv::Point, 4> result{};
    for (int i = 0; i < 4; ++i) {
        result[static_cast<std::size_t>(i)] = cv::Point(cvRound(points[i].x), cvRound(points[i].y));
    }
    return result;
}

float angleDifference(float a, float b) {
    return std::abs(a - b);
}
}  // namespace

ArmorDetector::ArmorDetector(DetectorParams params) : params_(std::move(params)) {
    params_.diff_threshold = std::clamp(params_.diff_threshold, 0, 255);
    params_.brightness_threshold = std::clamp(params_.brightness_threshold, 0, 255);
    params_.min_contour_area = std::max(0.0, params_.min_contour_area);
    params_.min_aspect_ratio = std::max(1.0F, params_.min_aspect_ratio);
    params_.max_vertical_angle = std::clamp(params_.max_vertical_angle, 0.0F, 90.0F);
    params_.morphology_kernel = std::max(1, params_.morphology_kernel);
    if (params_.morphology_kernel % 2 == 0) {
        ++params_.morphology_kernel;
    }
}

std::string toString(TargetColor color) {
    return color == TargetColor::Blue ? "blue" : "red";
}

cv::Mat ArmorDetector::makeMask(const cv::Mat& bgr,
                                cv::Mat& gray,
                                cv::Mat& color_difference,
                                cv::Mat& raw_mask,
                                cv::Mat& opened_mask) const {
    cv::Mat blurred;
    cv::GaussianBlur(bgr, blurred, cv::Size(3, 3), 0.0);
    cv::cvtColor(blurred, gray, cv::COLOR_BGR2GRAY);

    std::vector<cv::Mat> channels;
    cv::split(blurred, channels);
    if (params_.target_color == TargetColor::Blue) {
        // OpenCV stores color images as BGR: blue response minus red response.
        cv::subtract(channels[0], channels[2], color_difference);
    } else {
        // Red response minus blue response.
        cv::subtract(channels[2], channels[0], color_difference);
    }

    cv::Mat color_mask;
    cv::Mat brightness_mask;
    cv::threshold(color_difference, color_mask, params_.diff_threshold, 255, cv::THRESH_BINARY);
    cv::threshold(gray, brightness_mask, params_.brightness_threshold, 255, cv::THRESH_BINARY);
    cv::bitwise_and(color_mask, brightness_mask, raw_mask);

    const int k = params_.morphology_kernel;
    const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(k, k));
    cv::morphologyEx(raw_mask, opened_mask, cv::MORPH_OPEN, kernel);
    cv::Mat closed_mask;
    cv::morphologyEx(opened_mask, closed_mask, cv::MORPH_CLOSE, kernel);
    return closed_mask;
}

std::vector<LightBar> ArmorDetector::findLightBars(const cv::Mat& mask,
                                                   cv::Mat& candidate_visualization) const {
    std::vector<std::vector<cv::Point>> contours;
    cv::Mat contour_input = mask.clone();
    cv::findContours(contour_input, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    candidate_visualization = cv::Mat::zeros(mask.size(), CV_8UC3);
    std::vector<LightBar> bars;
    const double max_area = static_cast<double>(mask.total()) * 0.08;

    for (const auto& contour : contours) {
        if (contour.size() < 3) {
            continue;
        }
        const double area = cv::contourArea(contour);
        if (area < params_.min_contour_area || area > max_area) {
            continue;
        }

        const cv::RotatedRect rect = cv::minAreaRect(contour);
        const float long_side = std::max(rect.size.width, rect.size.height);
        const float short_side = std::min(rect.size.width, rect.size.height);
        if (short_side < 1.0F || long_side < 8.0F) {
            continue;
        }

        const float ratio = long_side / short_side;
        const float angle = angleFromVertical(rect);
        if (ratio < params_.min_aspect_ratio || angle > params_.max_vertical_angle) {
            continue;
        }

        bars.push_back(LightBar{rect, area, long_side, short_side, angle});
    }

    std::sort(bars.begin(), bars.end(), [](const LightBar& a, const LightBar& b) {
        return a.rect.center.x < b.rect.center.x;
    });

    candidate_visualization = cv::Mat::zeros(mask.size(), CV_8UC3);
    for (std::size_t i = 0; i < bars.size(); ++i) {
        const auto box = integerBox(bars[i].rect);
        const std::vector<cv::Point> polygon(box.begin(), box.end());
        const std::vector<std::vector<cv::Point>> polygons{polygon};
        cv::polylines(candidate_visualization, polygons, true,
                      cv::Scalar(0, 215, 255), 2, cv::LINE_AA);
        cv::putText(candidate_visualization, "L" + std::to_string(i),
                    cv::Point(cvRound(bars[i].rect.center.x), cvRound(bars[i].rect.center.y)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
    }
    return bars;
}

bool ArmorDetector::findBestPair(const std::vector<LightBar>& bars,
                                 cv::RotatedRect& armor_rect,
                                 cv::Point2f& center) const {
    if (bars.size() < 2) {
        return false;
    }

    float best_score = std::numeric_limits<float>::max();
    bool found = false;
    for (std::size_t i = 0; i < bars.size(); ++i) {
        for (std::size_t j = i + 1; j < bars.size(); ++j) {
            const LightBar& left = bars[i];
            const LightBar& right = bars[j];
            const float average_height = (left.long_side + right.long_side) * 0.5F;
            if (average_height <= 1.0F) {
                continue;
            }

            const float height_ratio = std::max(left.long_side, right.long_side) /
                                       std::max(1.0F, std::min(left.long_side, right.long_side));
            const float y_difference = std::abs(left.rect.center.y - right.rect.center.y);
            const float x_distance = right.rect.center.x - left.rect.center.x;
            const float center_distance = cv::norm(right.rect.center - left.rect.center);
            const float distance_ratio = center_distance / average_height;
            const float angle_difference_value = angleDifference(left.angle_from_vertical,
                                                                   right.angle_from_vertical);

            // These ratios are starting points for the supplied demo, not universal thresholds.
            if (height_ratio > 1.7F || y_difference > average_height * 0.65F ||
                angle_difference_value > 25.0F ||
                x_distance < average_height * 0.7F ||
                x_distance > average_height * 5.5F ||
                distance_ratio < 1.0F || distance_ratio > 6.0F) {
                continue;
            }

            const float score = std::abs(height_ratio - 1.0F) * 2.0F +
                                y_difference / average_height +
                                angle_difference_value / 45.0F +
                                std::abs(distance_ratio - 2.0F) * 0.12F;
            if (score < best_score) {
                best_score = score;
                center = (left.rect.center + right.rect.center) * 0.5F;
                std::vector<cv::Point2f> corners;
                corners.reserve(8);
                cv::Point2f left_points[4];
                cv::Point2f right_points[4];
                left.rect.points(left_points);
                right.rect.points(right_points);
                corners.insert(corners.end(), left_points, left_points + 4);
                corners.insert(corners.end(), right_points, right_points + 4);
                armor_rect = cv::minAreaRect(corners);
                found = true;
            }
        }
    }
    return found;
}

DetectionResult ArmorDetector::detect(const cv::Mat& bgr) const {
    if (bgr.empty()) {
        throw std::invalid_argument("输入图像为空");
    }
    if (bgr.channels() != 3) {
        throw std::invalid_argument("输入必须是 3 通道 BGR 图像");
    }

    DetectionResult result;
    result.input = bgr.clone();
    result.annotated = bgr.clone();
    result.mask = makeMask(bgr, result.gray, result.color_difference,
                           result.raw_mask, result.opened_mask);
    result.light_bars = findLightBars(result.mask, result.candidate_visualization);

    result.valid = findBestPair(result.light_bars, result.armor_rect, result.center);
    for (const LightBar& bar : result.light_bars) {
        const auto box = integerBox(bar.rect);
        const std::vector<cv::Point> polygon(box.begin(), box.end());
        const std::vector<std::vector<cv::Point>> polygons{polygon};
        cv::polylines(result.annotated, polygons, true,
                      cv::Scalar(0, 215, 255), 2, cv::LINE_AA);
    }

    if (result.valid) {
        const auto armor_box = integerBox(result.armor_rect);
        const std::vector<cv::Point> polygon(armor_box.begin(), armor_box.end());
        const std::vector<std::vector<cv::Point>> polygons{polygon};
        cv::polylines(result.annotated, polygons, true,
                      cv::Scalar(0, 255, 0), 3, cv::LINE_AA);
        cv::circle(result.annotated, result.center, 5, cv::Scalar(0, 255, 255), -1, cv::LINE_AA);
    }

    const std::string status = result.valid
        ? "valid=1 center=(" + std::to_string(cvRound(result.center.x)) + "," +
              std::to_string(cvRound(result.center.y)) + ")"
        : "valid=0 target_not_found";
    cv::putText(result.annotated, status, cv::Point(16, 30),
                cv::FONT_HERSHEY_SIMPLEX, 0.7,
                result.valid ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
    return result;
}
