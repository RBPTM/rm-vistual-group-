#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

enum class TargetColor { Blue, Red };

void save(const fs::path& path, const cv::Mat& image) {
    if (image.empty() || !cv::imwrite(path.string(), image)) {
        throw std::runtime_error("无法写入图像：" + path.string());
    }
}

cv::Mat makeInRangeMask(const cv::Mat& hsv, TargetColor color) {
    cv::Mat mask;
    if (color == TargetColor::Blue) {
        cv::inRange(hsv, cv::Scalar(90, 70, 35), cv::Scalar(140, 255, 255), mask);
    } else {
        cv::Mat red_low;
        cv::Mat red_high;
        cv::inRange(hsv, cv::Scalar(0, 70, 35), cv::Scalar(10, 255, 255), red_low);
        cv::inRange(hsv, cv::Scalar(170, 70, 35), cv::Scalar(179, 255, 255), red_high);
        cv::bitwise_or(red_low, red_high, mask);
    }
    return mask;
}

int main(int argc, char** argv) {
    try {
        if (argc < 3 || argc > 4) {
            std::cerr << "用法：" << argv[0]
                      << " <输入图片> <输出目录> [blue|red]\n"
                      << "示例：" << argv[0]
                      << " data/armor_blue.png output/toolbox blue\n";
            return 2;
        }

        const std::string input_path = argv[1];
        const fs::path output_dir(argv[2]);
        const std::string color_text = argc == 4 ? argv[3] : "blue";
        if (color_text != "blue" && color_text != "red") {
            throw std::invalid_argument("颜色模式必须是 blue 或 red");
        }
        const TargetColor target_color = color_text == "blue" ? TargetColor::Blue : TargetColor::Red;

        const cv::Mat bgr = cv::imread(input_path, cv::IMREAD_COLOR);
        if (bgr.empty()) {
            throw std::runtime_error("无法读取输入图片：" + input_path);
        }
        fs::create_directories(output_dir);

        save(output_dir / "01_bgr_input.png", bgr);
        cv::Mat gray;
        cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
        save(output_dir / "02_gray.png", gray);

        std::vector<cv::Mat> bgr_channels;
        cv::split(bgr, bgr_channels);
        save(output_dir / "03_blue_channel.png", bgr_channels[0]);
        save(output_dir / "04_green_channel.png", bgr_channels[1]);
        save(output_dir / "05_red_channel.png", bgr_channels[2]);

        cv::Mat hsv;
        cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
        std::vector<cv::Mat> hsv_channels;
        cv::split(hsv, hsv_channels);
        cv::Mat hue_scaled;
        hsv_channels[0].convertTo(hue_scaled, CV_8U, 255.0 / 179.0);
        cv::Mat hue_pseudo_color;
        cv::applyColorMap(hue_scaled, hue_pseudo_color, cv::COLORMAP_HSV);
        save(output_dir / "06_hue_pseudocolor.png", hue_pseudo_color);
        save(output_dir / "07_saturation.png", hsv_channels[1]);
        save(output_dir / "08_value.png", hsv_channels[2]);

        cv::Mat lab;
        cv::cvtColor(bgr, lab, cv::COLOR_BGR2Lab);
        std::vector<cv::Mat> lab_channels;
        cv::split(lab, lab_channels);
        save(output_dir / "09_lab_L.png", lab_channels[0]);
        save(output_dir / "10_lab_a.png", lab_channels[1]);
        save(output_dir / "11_lab_b.png", lab_channels[2]);

        cv::Mat gaussian;
        cv::GaussianBlur(gray, gaussian, cv::Size(5, 5), 0.0);
        save(output_dir / "12_gaussian_5x5.png", gaussian);
        cv::Mat median;
        cv::medianBlur(gray, median, 5);
        save(output_dir / "13_median_5.png", median);
        cv::Mat bilateral;
        cv::bilateralFilter(gray, bilateral, 9, 50.0, 50.0);
        save(output_dir / "14_bilateral.png", bilateral);

        cv::Mat fixed_threshold;
        cv::threshold(gaussian, fixed_threshold, 90, 255, cv::THRESH_BINARY);
        save(output_dir / "15_binary_threshold_90.png", fixed_threshold);
        cv::Mat otsu;
        cv::threshold(gaussian, otsu, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        save(output_dir / "16_otsu_binary.png", otsu);

        const cv::Mat in_range_mask = makeInRangeMask(hsv, target_color);
        save(output_dir / "17_inrange_mask.png", in_range_mask);

        std::vector<cv::Mat> channels;
        cv::split(bgr, channels);
        cv::Mat difference;
        if (target_color == TargetColor::Blue) {
            cv::subtract(channels[0], channels[2], difference);
        } else {
            cv::subtract(channels[2], channels[0], difference);
        }
        save(output_dir / "18_channel_difference.png", difference);
        cv::Mat difference_threshold;
        cv::threshold(difference, difference_threshold, 35, 255, cv::THRESH_BINARY);
        save(output_dir / "19_difference_threshold.png", difference_threshold);

        cv::Mat edges;
        cv::Canny(gaussian, edges, 50, 150);
        save(output_dir / "20_canny_edges.png", edges);
        cv::Mat edges_overlay = bgr.clone();
        cv::Mat edge_color;
        cv::cvtColor(edges, edge_color, cv::COLOR_GRAY2BGR);
        edge_color.setTo(cv::Scalar(80, 255, 80), edges);
        cv::addWeighted(bgr, 0.85, edge_color, 0.6, 0.0, edges_overlay);
        save(output_dir / "21_canny_overlay.png", edges_overlay);

        cv::Mat morph_open;
        cv::Mat morph_close;
        const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::morphologyEx(difference_threshold, morph_open, cv::MORPH_OPEN, kernel);
        cv::morphologyEx(morph_open, morph_close, cv::MORPH_CLOSE, kernel);
        save(output_dir / "22_morph_open.png", morph_open);
        save(output_dir / "23_morph_close.png", morph_close);

        cv::Mat contour_input = morph_close.clone();
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(contour_input, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        cv::Mat contour_visualization = bgr.clone();
        for (const auto& contour : contours) {
            if (cv::contourArea(contour) < 12.0) {
                continue;
            }
            const cv::RotatedRect rect = cv::minAreaRect(contour);
            cv::Point2f points[4];
            rect.points(points);
            std::vector<cv::Point> polygon;
            polygon.reserve(4);
            for (const cv::Point2f& point : points) {
                polygon.emplace_back(cvRound(point.x), cvRound(point.y));
            }
            const std::vector<std::vector<cv::Point>> polygons{polygon};
            cv::polylines(contour_visualization, polygons, true, cv::Scalar(0, 255, 255), 2, cv::LINE_AA);
        }
        save(output_dir / "24_contours_min_area_rect.png", contour_visualization);

        std::ofstream report(output_dir / "README_results.txt");
        report << "OpenCV image toolbox outputs\n"
               << "input=" << input_path << '\n'
               << "mode=" << color_text << '\n'
               << "size=" << bgr.cols << 'x' << bgr.rows << '\n'
               << "contours=" << contours.size() << '\n'
               << "inRange mask: HSV thresholds are teaching defaults, not universal parameters.\n"
               << "channel difference threshold: 35; gray threshold: 90; Canny thresholds: 50,150.\n";
        std::cout << "已生成 24 类处理结果：" << output_dir << '\n'
                  << "颜色模式：" << color_text << "；输入尺寸：" << bgr.cols << 'x' << bgr.rows << '\n';
        return 0;
    } catch (const cv::Exception& error) {
        std::cerr << "OpenCV 错误：" << error.what() << '\n';
        return 3;
    } catch (const std::exception& error) {
        std::cerr << "错误：" << error.what() << '\n';
        return 1;
    }
}
