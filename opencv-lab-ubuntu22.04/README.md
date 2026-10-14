# Ubuntu 22.04 · RoboMaster OpenCV 专项培训实操代码

本项目为《专项培训 2》配套的、可独立构建的 C++ / OpenCV 入门工程。它面向 Ubuntu 22.04 虚拟机，使用系统软件包中的 OpenCV 4，不依赖 ROS、相机驱动、CUDA、第三方战队工程或私有数据。

> 当前仓库附带的 `data/armor_blue.png` 和 `data/armor_red.png` 是为重复实验制作的合成教学图，不是实车采集图。教材中的 `picture/2-1.png` 至 `picture/2-10.png` 是课程配图，与这两张可运行的练习输入用途不同。

## 1. 在 Ubuntu 22.04 安装工具链

第一次安装依赖时，给虚拟机启用网络（例如 NAT），在终端执行：

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libopencv-dev
```

检查工具是否可用：

```bash
g++ --version
cmake --version
pkg-config --modversion opencv4
pkg-config --cflags --libs opencv4
```

`pkg-config --modversion opencv4` 应输出当前系统安装的 OpenCV 4.x 版本号。后续编译和运行不要求始终联网。

## 2. 构建项目

在终端进入本目录（解压独立代码包后，目录名通常是 `opencv-lab-ubuntu22.04`）：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

成功后会生成两个可执行程序：

- `build/image_toolbox`：一次性生成色彩空间、通道、滤波、阈值、`inRange`、Canny、形态学和轮廓图，适合第 2–4 章。
- `build/armor_lab`：从颜色差分掩膜提取灯条候选并配对；同时支持图片和视频，适合第 1、4–7 章。

## 3. 运行静态图片练习

从项目根目录执行：

```bash
./build/image_toolbox data/armor_blue.png output/toolbox-blue blue
./build/armor_lab data/armor_blue.png blue output/armor-blue

./build/image_toolbox data/armor_red.png output/toolbox-red red
./build/armor_lab data/armor_red.png red output/armor-red
```

所有结果均保存到 `output/`，不要求弹出图形窗口。可以通过文件管理器打开图片，或执行：

```bash
xdg-open output/armor-blue/08_pairing.png
xdg-open output/toolbox-blue/17_inrange_mask.png
```

如果虚拟机桌面支持 OpenCV HighGUI，也可以选择加上 `--display` 看实时窗口：

```bash
./build/armor_lab data/armor_blue.png blue output/armor-blue-window --display
```

没有桌面窗口、通过 SSH 连接，或者虚拟机图形环境不可用时，不要加 `--display`；程序默认只写文件。

## 4. 使用自己的图片或视频

把自己的图片或视频复制进 `data/`，例如 `data/test.png` 或 `data/test.mp4`。按实际相对路径执行：

```bash
./build/armor_lab data/test.png blue output/test-image
./build/armor_lab data/test.mp4 blue output/test-video
```

工程同时附带一段用于教学的合成视频，其中目标会短暂离开画面。可以直接运行：

```bash
./build/armor_lab data/armor_demo.avi blue output/video-demo
```

检查 `output/video-demo/detections.csv`：目标可见区段通常会出现 `valid=1`，目标缺失区段应出现 `valid=0,nan,nan`。实际判断仍以程序输出和调试图为准。

视频模式会逐帧处理，输出：

- `annotated.avi`：逐帧可视化结果（若当前 OpenCV 视频编码器可用）；
- `detections.csv`：每帧的 `valid`、中心坐标和候选数量；未检测到目标时输出 `valid=0` 和 `nan,nan`，不沿用上一帧的中心；
- `01_input.png` 至 `08_pairing.png`：视频第一帧的中间处理结果，便于调试。

如果自己的视频无法打开，请先确认它能在 Ubuntu 播放，并尝试将它转为常见 MP4 / AVI 格式。虚拟机的 OpenCV 编译选项和系统视频编码器会影响可读视频格式。

## 5. 输出文件与教材章节对应

### `image_toolbox` 输出

| 文件 | 观察要点 |
|---|---|
| `01_bgr_input.png` | 输入图像与 BGR 顺序 |
| `02_gray.png`、`03_blue_channel.png`、`04_green_channel.png`、`05_red_channel.png` | 灰度与单通道信息 |
| `06_hue_pseudocolor.png`、`07_saturation.png`、`08_value.png` | HSV 三个通道；H 使用伪彩色显示 |
| `09_lab_L.png`、`10_lab_a.png`、`11_lab_b.png` | Lab 的 L/a/b 通道 |
| `12_gaussian_5x5.png`、`13_median_5.png`、`14_bilateral.png` | 三种平滑结果对比 |
| `15_binary_threshold_90.png`、`16_otsu_binary.png` | 固定阈值与 Otsu 阈值 |
| `17_inrange_mask.png` | HSV 范围阈值；范围只是教学起点，不是普适参数 |
| `18_channel_difference.png`、`19_difference_threshold.png` | B-R 或 R-B 通道差与阈值掩膜 |
| `20_canny_edges.png`、`21_canny_overlay.png` | Canny 边缘与原图叠加 |
| `22_morph_open.png`、`23_morph_close.png` | 掩膜开/闭运算 |
| `24_contours_min_area_rect.png` | 轮廓及旋转外接矩形 |

### `armor_lab` 输出

| 文件 | 观察要点 |
|---|---|
| `01_input.png` | 原始 BGR 输入 |
| `02_gray.png` | 灰度结果 |
| `03_color_difference.png` | 目标色通道差 |
| `04_mask_raw.png` | 阈值组合得到的原始掩膜 |
| `05_mask_open.png` | 开运算后掩膜 |
| `06_mask_closed.png` | 开、闭运算后掩膜 |
| `07_candidates.png` | 通过面积、长宽比和倾角筛选的灯条候选 |
| `08_pairing.png` | 候选、最终装甲板框、中心点以及 `valid` |
| `result.txt` | 输入尺寸、阈值、候选数和中心点 |

## 6. 用命令行调参

程序默认参数只是合成教学图上的起点，不应直接当作实车参数：

```bash
./build/armor_lab data/armor_blue.png blue output/threshold-20 --diff-threshold 20
./build/armor_lab data/armor_blue.png blue output/threshold-60 --diff-threshold 60
./build/armor_lab data/armor_blue.png blue output/min-area-40 --min-area 40
```

每次只改变一个参数，比较 `04_mask_raw.png`、`06_mask_closed.png`、`07_candidates.png` 和 `08_pairing.png`。先确认前景掩膜正确，再调整灯条和配对几何条件。

## 7. 培训练习建议

1. **像素与特征**：比较灰度、B/G/R、HSV、Lab 和通道差分，说明哪种图更突出目标。
2. **阈值与滤波**：对比固定阈值、Otsu、`inRange` 和不同平滑结果，说明阈值变化如何影响前景区域。
3. **掩膜清理**：修改形态学核大小，保存并比较开运算前后结果，记录目标是否断裂或粘连。
4. **几何候选**：阅读 `src/armor_detector.cpp`，找到面积、长宽比、倾角和灯条长度条件，至少修改一项并比较候选框。
5. **灯条配对**：找出高度比、中心 y 差、角度差与中心距离条件，说明它们各自排除了哪类错误组合。
6. **视频状态**：将视频中的目标移出画面，检查 `detections.csv` 是否在丢失帧上输出 `valid=0`，且中心点没有沿用上一帧。
7. **报告**：每位学员提交命令、关键参数、至少两组中间图，以及一个失败案例和排查结论。

## 8. 已知边界

这是为传统视觉概念和 C++ 工程组织方式准备的最小教学程序，不是比赛级自瞄系统。它不提供相机驱动、陀螺仪/云台通信、三维定位、跟踪器、数字分类器或自动曝光控制。合成图片只保证流程可重复，实际素材需要重新评估阈值、过滤条件和配对逻辑。
