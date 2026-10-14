#pragma once

#include <opencv2/core.hpp>

#include <string>
#include <vector>

enum class TargetColor {
    Blue,
    Red,
};

struct DetectorParams {
    TargetColor target_color = TargetColor::Blue;
    int diff_threshold = 35;
    int brightness_threshold = 25;
    double min_contour_area = 12.0;
    float min_aspect_ratio = 2.5F;
    float max_vertical_angle = 40.0F;
    int morphology_kernel = 3;
};

struct LightBar {
    cv::RotatedRect rect;
    double contour_area = 0.0;
    float long_side = 0.0F;
    float short_side = 0.0F;
    float angle_from_vertical = 90.0F;
};

struct DetectionResult {
    cv::Mat input;
    cv::Mat gray;
    cv::Mat color_difference;
    cv::Mat raw_mask;
    cv::Mat opened_mask;
    cv::Mat mask;
    cv::Mat candidate_visualization;
    cv::Mat annotated;
    std::vector<LightBar> light_bars;
    bool valid = false;
    cv::Point2f center{};
    cv::RotatedRect armor_rect{};
};

class ArmorDetector {
public:
    explicit ArmorDetector(DetectorParams params);
    DetectionResult detect(const cv::Mat& bgr) const;

private:
    DetectorParams params_;
    cv::Mat makeMask(const cv::Mat& bgr,
                     cv::Mat& gray,
                     cv::Mat& color_difference,
                     cv::Mat& raw_mask,
                     cv::Mat& opened_mask) const;
    std::vector<LightBar> findLightBars(const cv::Mat& mask,
                                        cv::Mat& candidate_visualization) const;
    bool findBestPair(const std::vector<LightBar>& bars,
                      cv::RotatedRect& armor_rect,
                      cv::Point2f& center) const;
};

std::string toString(TargetColor color);
