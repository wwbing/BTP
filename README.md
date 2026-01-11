# BTP - RKNN 模型推理项目

基于 Rockchip NPU 的高性能缺陷检测推理实现，专为 RK3588 等嵌入式平台优化。

## 项目概述

本项目包含两个主要组件：

1. **BTP (缺陷检测核心)** - 基于 RKNN 的 YOLOv6 目标检测推理核心
2. **HostPC_DefectRKNN** - 桌面 GUI 应用程序，提供图形化界面进行模型推理

项目支持在 RK3588、RK356x、RK3576 和其他 Rockchip 平台上运行各种计算机视觉模型，当前配置用于缺陷检测应用。

## 特性

- 🚀 **高性能推理** - 利用 Rockchip NPU 硬件加速
- 🔧 **多平台支持** - RK3588、RK356x、RK3576、RV1106 等
- 📦 **完整工具链** - 从模型转换到部署的完整流程
- 🎯 **缺陷检测优化** - 针对 6 类缺陷检测任务优化
- 💻 **GUI 应用** - 提供友好的桌面操作界面
- ⚡ **硬件加速** - 支持 RGA 图像处理加速
- 🏗️ **清晰架构** - 三层分层架构，UI与业务逻辑分离

## 目录结构

```
BTP/
├── rknn_infer/              # 核心推理库
│   ├── src/                 # 源代码
│   │   ├── rknpu2/          # RK3588平台实现
│   │   └── postprocess.cc   # 后处理算法
│   ├── include/             # 头文件
│   └── utils/               # 图像和文件处理工具
├── 3rdparty/                # 第三方依赖库
│   ├── rknpu2/             # RK3588 NPU 运行时
│   ├── librga/             # RGA 图像处理库
│   └── jpeg_turbo/         # JPEG 编解码库
├── HostPC_DefectRKNN/      # 桌面 GUI 应用
│   ├── include/            # 服务类头文件
│   │   ├── inferenceengine.h   # 推理引擎
│   │   ├── imageprocessor.h    # 图像处理
│   │   ├── statisticsservice.h # 统计服务
│   │   └── fileservice.h       # 文件服务
│   ├── src/                # 服务类实现
│   ├── model/              # 模型文件
│   └── resources/          # 资源文件
└── install/                # 构建输出目录
```

## HostPC_DefectRKNN 架构

采用三层分层架构：

```
┌─────────────────────────────────────────────────────────────┐
│                      UI 层                                   │
│  MainWindow (主窗口)  |  CameraWindow (摄像头)  |  StatisticsDialog |
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                     服务层                                   │
│  InferenceEngine  |  ImageProcessor  |  StatisticsService   │
│  FileService      |  DefectColorManager                     │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                    底层 RKNN 库                              │
│  yolov6.cc  |  postprocess.cc  |  image_utils.c             │
└─────────────────────────────────────────────────────────────┘
```

### 服务类职责

| 服务类 | 职责 |
|--------|------|
| `InferenceEngine` | 封装 RKNN 推理引擎，RAII 管理资源 |
| `ImageProcessor` | 图像格式转换、检测框绘制 |
| `StatisticsService` | 批量检测统计数据收集和计算 |
| `FileService` | 文件查找、结果保存 |
| `DefectColorManager` | 6 类缺陷颜色配置 |

## 支持的平台

| 平台 | 架构 | 状态 |
|------|------|------|
| RK3588/RK3588S | aarch64/armhf | ✅ 主要支持 |
| RK3562/RK3566/RK3568 | aarch64/armhf | ✅ 支持 |
| RK3576 | aarch64/armhf | ✅ 支持 |
| RV1126B | armhf | ✅ 支持 |
| RV1103/RV1106 | armhf | ✅ 支持 |
| RV1109/RV1126 | armhf | ✅ 支持 |
| RK1808 | armhf | ✅ 支持 |

## 缺陷检测模型

当前项目配置用于缺陷检测，包含 6 个类别：

| 类别 | 名称 | 颜色 |
|------|------|------|
| cr | 裂纹 | 红色 |
| ic | 夹杂 | 橙色 |
| ps | 压痕 | 黄色 |
| rs | 划痕 | 绿色 |
| sc | 疤痕 | 蓝色 |
| pc | 坑点 | 紫色 |

## 快速开始

### 环境要求

- CMake 3.10+
- 交叉编译工具链 (aarch64-linux-gnu-gcc)
- RKNN 运行时库 (librknnrt.so、librga.so)
- Qt5 开发库

### 构建 GUI 应用

```bash
# 构建桌面应用程序
cd BTP/HostPC_DefectRKNN
mkdir build && cd build
cmake ..
make

# 运行 GUI 应用
cd BTP/HostPC_DefectRKNN/build
export LD_LIBRARY_PATH=../3rdparty/rknpu2/Linux/aarch64:../3rdparty/librga/Linux/aarch64:$LD_LIBRARY_PATH
./HostPC_DefectRKNN
```

### 构建命令行工具

```bash
# 进入 BTP 目录
cd BTP

# 默认构建 (RK3588 + aarch64 + Release)
./build-linux.sh

# 一键运行（构建+推理）
./run_neu_det.sh
```

## GUI 功能

- **单张图片检测** - 选择图片进行缺陷检测
- **批量检测** - 选择文件夹批量处理图片，结果保存到 `results/` 子目录
- **视频推理** - 对视频文件进行实时缺陷检测
- **摄像头检测** - 连接 V4L2 摄像头实时检测
- **统计分析** - 批量检测后查看统计图表

## 自定义配置

### 1. 模型配置

修改 `HostPC_DefectRKNN/src/inferenceengine.cpp`：

```cpp
QString modelPath = appPath + "/../model/your-model.rknn";
```

### 2. 缺陷类别颜色

修改 `HostPC_DefectRKNN/src/defect_colors.cpp`：

```cpp
// 0: cr - 裂纹 (红色)
colorConfigs[0].boxColor = QColor(255, 0, 0);
```

## 开发工具

### 代码跳转

项目已配置生成 `compile_commands.json`，用于 clangd 代码跳转和智能提示：

```
BTP/HostPC_DefectRKNN/build/compile_commands.json
BTP/build/build_rknn_yolov6_demo_*/compile_commands.json
```

### 代码格式化

使用 `.clang-format` 配置文件进行代码格式化，基于 Microsoft 风格。

## 许可证

本项目采用开源许可证，详见 LICENSE 文件。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进项目。

## 致谢

- Rockchip RKNN 技术支持
- RGA 硬件加速库
- Qt5 跨平台 GUI 框架
