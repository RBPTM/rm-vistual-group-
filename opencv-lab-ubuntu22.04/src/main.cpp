#include "armor_detector.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Options {
    std::string input_path;
    std::string output_dir;
    TargetColor color = TargetColor::Blue;
    DetectorParams params;
    bool display = false;
};

void printUsage(const char* executable) {
    std::cerr
        << "用法:\n  " << executable
        << " <图片或视频路径> <blue|red> <输出目录> [选项]\n\n"
        << "选项:\n"
        << "  --diff-threshold N       通道差分阈值，默认 35\n"
        << "  --brightness-threshold N 灰度亮度阈值，默认 25\n"
        << "  --min-area N             最小轮廓面积，默认 12\n"
        << "  --display                弹出调试窗口（需要桌面图形环境）\n\n"
        << "图片示例:\n  " << executable
        << " data/armor_blue.png blue output/blue\n"
        << "视频示例:\n  " << executable
        << " data/your_video.mp4 blue output/video\n";
}

int parseInt(const std::string& value, const std::string& option) {
    try {
        std::size_t consumed = 0;
        const int result = std::stoi(value, &consumed);
        if (consumed != value.size()) {
            throw std::invalid_argument("extra characters");
        }
        return result;
    } catch (const std::exception&) {
        throw std::invalid_argument("选项 " + option + " 需要整数参数，收到：" + value);
    }
}

double parseDouble(const std::string& value, const std::string& option) {
    try {
        std::size_t consumed = 0;
        const double result = std::stod(value, &consumed);
        if (consumed != value.size()) {
            throw std::invalid_argument("extra characters");
        }
        return result;
    } catch (const std::exception&) {
        throw std::invalid_argument("选项 " + option + " 需要数值参数，收到：" + value);
    }
}

Options parseArguments(int argc, char** argv) {
    if (argc < 4) {
        printUsage(argv[0]);
        throw std::invalid_argument("参数不足");
    }

    Options options;
    options.input_path = argv[1];
    const std::string color = argv[2];
    if (color == "blue") {
        options.color = TargetColor::Blue;
    } else if (color == "red") {
        options.color = TargetColor::Red;
    } else {
        throw std::invalid_argument("颜色模式必须是 blue 或 red");
    }
    options.params.target_color = options.color;
    options.output_dir = argv[3];

    for (int i = 4; i < argc; ++i) {
        const std::string argument = argv[i];
        const auto requireValue = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) {
                throw std::invalid_argument("选项 " + name + " 缺少参数");
            }
            ++i;
            return argv[i];
        };

        if (argument == "--display") {
            options.display = true;
        } else if (argument == "--diff-threshold") {
            options.params.diff_threshold = parseInt(requireValue(argument), argument);
        } else if (argument == "--brightness-threshold") {
            options.params.brightness_threshold = parseInt(requireValue(argument), argument);
        } else if (argument == "--min-area") {
            options.params.min_contour_area = parseDouble(requireValue(argument), argument);
        } else {
            throw std::invalid_argument("未知选项：" + argument);
        }
    }
    return options;
}

void writeImage(const fs::path& path, const cv::Mat& image) {
    if (image.empty()) {
        throw std::runtime_error("准备写出的图像为空：" + path.string());
    }
    if (!cv::imwrite(path.string(), image)) {
        throw std::runtime_error("无法写入图片：" + path.string());
    }
}

void saveStages(const fs::path& output_dir, const DetectionResult& result) {
    writeImage(output_dir / "01_input.png", result.input);
    writeImage(output_dir / "02_gray.png", result.gray);
    writeImage(output_dir / "03_color_difference.png", result.color_difference);
    writeImage(output_dir / "04_mask_raw.png", result.raw_mask);
    writeImage(output_dir / "05_mask_open.png", result.opened_mask);
    writeImage(output_dir / "06_mask_closed.png", result.mask);
    writeImage(output_dir / "07_candidates.png", result.candidate_visualization);
    writeImage(output_dir / "08_pairing.png", result.annotated);
}

void writeSummary(const fs::path& path,
                  const Options& options,
                  const cv::Size& size,
                  const DetectionResult& result) {
    std::ofstream summary(path);
    if (!summary) {
        throw std::runtime_error("无法写入报告：" + path.string());
    }
    summary << "RoboMaster OpenCV 实操结果\n"
            << "input=" << options.input_path << '\n'
            << "target_color=" << toString(options.color) << '\n'
            << "width=" << size.width << '\n'
            << "height=" << size.height << '\n'
            << "diff_threshold=" << options.params.diff_threshold << '\n'
            << "brightness_threshold=" << options.params.brightness_threshold << '\n'
            << "candidate_count=" << result.light_bars.size() << '\n'
            << "valid=" << (result.valid ? 1 : 0) << '\n';
    if (result.valid) {
        summary << "center_x=" << std::fixed << std::setprecision(2) << result.center.x << '\n'
                << "center_y=" << std::fixed << std::setprecision(2) << result.center.y << '\n';
    } else {
        summary << "center_x=nan\ncenter_y=nan\n";
    }
}

int runImage(const cv::Mat& image, const Options& options, const ArmorDetector& detector) {
    const fs::path output_dir(options.output_dir);
    fs::create_directories(output_dir);
    const DetectionResult result = detector.detect(image);
    saveStages(output_dir, result);
    writeSummary(output_dir / "result.txt", options, image.size(), result);

    std::cout << "模式：静态图片\n"
              << "输出目录：" << output_dir << '\n'
              << "颜色模式：" << toString(options.color) << '\n'
              << "候选灯条数：" << result.light_bars.size() << '\n'
              << "valid=" << (result.valid ? 1 : 0);
    if (result.valid) {
        std::cout << " center=(" << cvRound(result.center.x) << ", "
                  << cvRound(result.center.y) << ")";
    }
    std::cout << '\n';

    if (options.display) {
        cv::imshow("mask", result.mask);
        cv::imshow("candidates", result.candidate_visualization);
        cv::imshow("detection", result.annotated);
        cv::waitKey(0);
        cv::destroyAllWindows();
    }
    return 0;
}

int runVideo(const Options& options, const ArmorDetector& detector) {
    cv::VideoCapture capture(options.input_path);
    if (!capture.isOpened()) {
        throw std::runtime_error(
            "输入既不是可读图片，也不是可打开的视频：" + options.input_path +
            "\n请检查路径；视频应为 OpenCV/系统编解码器支持的格式。");
    }

    const fs::path output_dir(options.output_dir);
    fs::create_directories(output_dir);
    const double source_fps = capture.get(cv::CAP_PROP_FPS);
    const double output_fps = source_fps > 1.0 && source_fps < 240.0 ? source_fps : 30.0;
    cv::VideoWriter writer;
    std::ofstream csv(output_dir / "detections.csv");
    if (!csv) {
        throw std::runtime_error("无法创建 detections.csv");
    }
    csv << "frame,valid,cx,cy,candidate_count\n";

    cv::Mat frame;
    int frame_index = 0;
    int valid_frames = 0;
    bool saved_first_frame = false;
    bool video_writer_attempted = false;
    while (capture.read(frame)) {
        const DetectionResult result = detector.detect(frame);
        if (!video_writer_attempted) {
            video_writer_attempted = true;
            const fs::path video_path = output_dir / "annotated.avi";
            writer.open(video_path.string(), cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
                        output_fps, frame.size(), true);
            if (!writer.isOpened()) {
                std::cerr << "警告：当前 OpenCV 环境无法创建 MJPG 视频文件；仍会输出逐帧 CSV 和首帧调试图。\n";
            }
        }
        if (writer.isOpened()) {
            writer.write(result.annotated);
        }

        if (!saved_first_frame) {
            saveStages(output_dir, result);
            writeSummary(output_dir / "first_frame_result.txt", options, frame.size(), result);
            saved_first_frame = true;
        }

        if (result.valid) {
            ++valid_frames;
            csv << frame_index << ",1," << result.center.x << "," << result.center.y
                << "," << result.light_bars.size() << '\n';
        } else {
            // 每帧独立写出；失败帧不沿用上一次中心点。
            csv << frame_index << ",0,nan,nan," << result.light_bars.size() << '\n';
        }

        if (options.display) {
            cv::imshow("mask", result.mask);
            cv::imshow("detection", result.annotated);
            if (cv::waitKey(1) == 27) {
                break;
            }
        }
        ++frame_index;
    }

    writer.release();
    capture.release();
    csv.close();
    if (options.display) {
        cv::destroyAllWindows();
    }
    if (frame_index == 0) {
        throw std::runtime_error("视频没有读取到任何帧：" + options.input_path);
    }

    std::cout << "模式：视频逐帧处理\n"
              << "处理帧数：" << frame_index << '\n'
              << "检测有效帧数：" << valid_frames << '\n'
              << "有效率：" << std::fixed << std::setprecision(1)
              << (100.0 * valid_frames / frame_index) << "%\n"
              << "输出目录：" << output_dir << '\n';
    return 0;
}

int main(int argc, char** argv) {
    try {
        const Options options = parseArguments(argc, argv);
        if (!fs::exists(options.input_path)) {
            throw std::runtime_error("输入路径不存在：" + options.input_path);
        }

        ArmorDetector detector(options.params);
        const cv::Mat image = cv::imread(options.input_path, cv::IMREAD_COLOR);
        if (!image.empty()) {
            return runImage(image, options, detector);
        }
        return runVideo(options, detector);
    } catch (const std::invalid_argument& error) {
        std::cerr << "参数错误：" << error.what() << '\n';
        return 2;
    } catch (const cv::Exception& error) {
        std::cerr << "OpenCV 错误：" << error.what() << '\n';
        return 3;
    } catch (const std::exception& error) {
        std::cerr << "错误：" << error.what() << '\n';
        return 1;
    }
}
