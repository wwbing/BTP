# BTP - RKNN 模型推理项目

基于 Rockchip NPU 的高性能 YOLOv6 目标检测推理实现，专为 RK3588 等嵌入式平台优化。

## 项目概述

本项目包含两个主要组件：

1. **BTP (缺陷检测)** - 基于 RKNN 的 YOLOv6 目标检测推理核心
2. **HostPC_DefectRKNN** - 桌面 GUI 应用程序，提供图形化界面进行模型推理

项目支持在 RK3588、RK356x、RK3576 和其他 Rockchip 平台上运行各种计算机视觉模型，当前配置用于缺陷检测应用。

## 特性

- 🚀 **高性能推理** - 利用 Rockchip NPU 硬件加速
- 🔧 **多平台支持** - RK3588、RK356x、RK3576、RV1106 等
- 📦 **完整工具链** - 从模型转换到部署的完整流程
- 🎯 **缺陷检测优化** - 针对 6 类缺陷检测任务优化
- 💻 **GUI 应用** - 提供友好的桌面操作界面
- ⚡ **硬件加速** - 支持 RGA 图像处理加速

## 目录结构

```
BTP/
├── rknn_infer/              # 核心推理库
│   ├── src/                 # 源代码
│   ├── include/             # 头文件
│   ├── utils/               # 图像和文件处理工具
│   └── model/               # RKNN 模型和标签文件
├── 3rdparty/                # 第三方依赖库
│   ├── rknpu2/             # RK3588 NPU 运行时
│   ├── librga/             # RGA 图像处理库
│   └── opencv/             # OpenCV 库
├── HostPC_DefectRKNN/      # 桌面 GUI 应用
└── install/                # 构建输出目录
```

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

- **cr** - 某种缺陷类型
- **ic** - 某种缺陷类型  
- **ps** - 某种缺陷类型
- **rs** - 某种缺陷类型
- **sc** - 某种缺陷类型
- **pc** - 某种缺陷类型

## 快速开始

### 环境要求

- CMake 3.10+
- 交叉编译工具链 (aarch64-linux-gnu-gcc)
- RKNN 运行时库 (librknnrt.so、librga.so)

### 构建项目

```bash
# 进入 BTP 目录
cd BTP

# 默认构建 (RK3588 + aarch64 + Release)
./build-linux.sh

# 自定义构建选项
./build-linux.sh -b Debug        # Debug 构建
./build-linux.sh -b Debug -m    # 启用地址 sanitizer
./build-linux.sh -r             # 禁用 RGA
./build-linux.sh -j             # 禁用 libjpeg

# 一键运行（构建+推理）
./run_neu_det.sh
```

### 运行推理

```bash
# 导航到构建输出目录
cd BTP/install/rk3588_linux_aarch64/rknn_yolov6_demo

# 设置库路径并运行推理
export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH
./rknn_yolov6_demo model/neu-det-new.rknn model/neu-det-inclusion_4.jpg
```

### 运行 GUI 应用

```bash
# 构建桌面应用程序
cd BTP/HostPC_DefectRKNN
mkdir build && cd build
cmake ..
make

# 运行 GUI 应用
./HostPC_DefectRKNN
```

## 自定义模型配置

### 1. 标签文件配置

修改 `BTP/rknn_infer/src/postprocess.cc`：

```cpp
#define LABEL_NALE_TXT_PATH "./model/your_labels.txt"
```

### 2. 类别数量配置

修改 `BTP/rknn_infer/include/postprocess.h`：

```cpp
#define OBJ_CLASS_NUM N  // 修改为你的类别数量
```

### 3. CMakeLists.txt 配置

在 `BTP/rknn_infer/CMakeLists.txt` 中更新文件安装路径：

```cmake
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/model/your_labels.txt DESTINATION model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/model/your_image.jpg DESTINATION model)
```

## 核心架构

### 推理管道

1. **模型加载** - 通过 RKNN API 加载量化模型
2. **图像预处理** - Letterbox 缩放、归一化、格式转换
3. **NPU 推理** - 在 Rockchip NPU 上执行模型推理
4. **后处理** - NMS 过滤、边界框解码、置信度阈值
5. **结果可视化** - 在输出图像上绘制边界框和标签

### 关键模块

- `rknn_app_context_t` - RKNN 模型上下文管理
- `object_detect_result_list` - 检测结果列表
- `letterbox_t` - 图像预处理参数
- 图像处理工具 (`image_utils.c`, `image_drawing.c`)
- 文件处理工具 (`file_utils.c`)

## 开发工具

### 代码跳转

项目已配置生成 `compile_commands.json`，用于 clangd 代码跳转和智能提示：

```
BTP/build/build_rknn_yolov6_demo_rk3588_linux_aarch64_Release/compile_commands.json
```

### 代码格式化

使用 `.clang-format` 配置文件进行代码格式化，基于 Microsoft 风格：
- 缩进使用 4 个空格
- 大括号使用 Allman 风格
- 列限制设置为 0（无限制）

## 许可证

本项目采用开源许可证，详见 LICENSE 文件。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进项目。

## 致谢

- Rockchip RKNN 技术支持
- OpenCV 图像处理库
- RGA 硬件加速库