# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

这是 RKNN 模型推理项目，专门用于缺陷检测等工业应用场景。项目基于 Rockchip NPU 技术，提供完整的 C++ 推理演示和 Qt5 上位机界面，支持在 RK3588、RK356x、RK3576 和其他 Rockchip 平台上运行各种计算机视觉模型。

项目包含两个主要组件：
1. **BTP (缺陷检测项目)**: 基于 Rockchip NPU 的 YOLOv6 目标检测推理实现，专门为 RK3588 平台优化，支持多NPU核心并行处理
2. **HostPC_DefectRKNN**: 桌面应用程序，提供图形化界面进行单张图片和批量图片的缺陷检测，支持文件夹批量处理

## 项目结构

```
BTP/
├── rknn_infer/              # 核心推理代码
│   ├── src/                 # 源代码目录
│   │   ├── main.cc          # 命令行程序入口（支持批量处理）
│   │   ├── postprocess.cc   # 后处理算法
│   │   └── rknpu2/yolov6.cc # RK3588 平台实现（使用多NPU核心）
│   ├── include/            # 头文件
│   │   ├── yolov6.h        # RKNN 模型封装
│   │   └── postprocess.h   # 后处理接口
│   ├── model/              # 模型和数据文件
│   │   ├── neu-det-new.rknn    # RKNN 模型文件
│   │   └── neu-det_6_labels_list.txt  # 标签文件
│   └── utils/              # 工具库
│       ├── image_utils.c   # 图像处理工具
│       ├── image_drawing.c # 可视化工具
│       └── file_utils.c    # 文件操作工具（新增批量处理支持）
├── HostPC_DefectRKNN/        # 桌面GUI应用程序
│   ├── src/main.cpp         # 程序入口
│   ├── src/mainwindow.cpp   # 主窗口实现（新增批量检测功能）
│   ├── include/mainwindow.h # 主窗口头文件
│   ├── model/               # 模型文件副本
│   └── build/               # 构建输出
├── 3rdparty/               # 第三方依赖
│   ├── rknpu2/             # RK3588 NPU 运行时库
│   ├── librga/             # RGA 图像处理库
│   ├── jpeg_turbo/         # JPEG 处理库
│   └── stb_image/          # 图像加载库
├── build-linux.sh          # 构建脚本
├── run_neu_det.sh         # 一键运行脚本
├── .clang-format          # 代码格式化配置
└── compile_commands.json  # 编译数据库
```

## 常用命令

### BTP 项目构建
```bash
# 默认构建 (RK3588 + aarch64 + Release)
./build-linux.sh

# 自定义构建选项
./build-linux.sh -b Debug        # Debug 构建
./build-linux.sh -b Debug -m    # 启用地址 sanitizer
./build-linux.sh -r             # 禁用 RGA
./build-linux.sh -j             # 禁用 libjpeg

# 一键运行（构建+推理）
./run_neu_det.sh

# 清理构建产物
rm -rf build/ install/
```

### 运行推理
```bash
# 导航到构建输出目录
cd install/rk3588_linux_aarch64/rknn_yolov6_demo

# 设置库路径并运行推理
export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH

# 单张图片推理
./rknn_yolov6_demo model/neu-det-new.rknn model/neu-det-inclusion_4.jpg

# 批量处理目录中的所有图片
./rknn_yolov6_demo model/neu-det-new.rknn /path/to/images/
```

### HostPC_DefectRKNN GUI 应用
```bash
# 构建桌面应用程序
cd HostPC_DefectRKNN
mkdir build && cd build
cmake ..
make

# 运行 GUI 应用
./HostPC_DefectRKNN
```

## 核心架构

### 推理管道
1. **输入处理** (`rknn_infer/src/main.cc`): 支持单文件和目录批量处理
2. **模型加载** (`rknn_infer/src/rknpu2/yolov6.cc`): 通过 RKNN API 加载量化模型，使用RKNN_NPU_CORE_0_1_2核心配置
3. **图像预处理** (`rknn_infer/utils/image_utils.c`): Letterbox 缩放、归一化、格式转换
4. **NPU 推理** (`rknn_infer/src/rknpu2/yolov6.cc`): 在 RK3588 NPU 上执行模型推理，支持多核心并行处理
5. **后处理** (`rknn_infer/src/postprocess.cc`): NMS 过滤、边界框解码、置信度阈值
6. **结果可视化** (`rknn_infer/utils/image_drawing.c`): 在输出图像上绘制边界框和标签

### 关键数据结构
- `rknn_app_context_t`: RKNN 模型上下文，包含输入输出张量信息
- `object_detect_result_list`: 检测结果列表，支持多目标检测
- `letterbox_t`: 图像预处理参数，处理不同尺寸输入

### 多NPU核心配置
项目已优化为使用RK3588的所有3个NPU核心：
- **初始化配置**: `RKNN_NPU_CORE_0_1_2` (值7，使用所有核心)
- **核心掩码设置**: 通过 `rknn_set_core_mask()` 函数强制使用多核心
- **性能优化**: 并行处理提高推理速度，适用于工业实时检测场景

### 文件夹批量处理功能
最近新增的功能支持批量处理目录中的所有图片文件：

**主要实现位置**：
- `rknn_infer/src/main.cc`: 主程序逻辑，支持文件和目录两种输入模式
- `rknn_infer/utils/file_utils.c/h`: 文件处理工具库

**核心功能**：
1. **路径检测**: 自动识别输入是文件还是目录
2. **图片过滤**: 支持 `.jpg`, `.jpeg`, `.png`, `.bmp`, `.tiff`, `.tif` 格式
3. **批量处理**: 遍历目录中的所有图片文件进行推理
4. **内存管理**: 动态内存分配和安全的释放机制
5. **错误处理**: 单个文件失败不影响整体处理流程

**使用示例**：
```bash
# 处理单个文件
./rknn_yolov6_demo model.rknn image.jpg

# 处理整个目录
./rknn_yolov6_demo model.rknn /path/to/images/
```

**输出格式**：每个输入文件会生成对应的输出文件，格式为 `<input_filename>_result.jpg`

### 平台适配
- `rknn_infer/src/rknpu2/`: RK3588/RK356x/RK3576 平台实现
- `rknn_infer/src/rknpu1/`: 传统平台支持 (RK1808/RV1126等)
- `rknn_infer/src/rknpu2/yolov6_rv1106_1103.cc`: RV1106/1103 低功耗平台实现

## 平台支持

### 支持的目标
- `rk3588`：RK3588/RK3588S（高性能，主要支持）
- `rk356x`：RK3562/RK3566/RK3568（中端）
- `rk3576`：RK3576（新一代）
- `rv1126b`：RV1126B（低功耗）
- `rv1106`：RV1103/RV1106（超低功耗，使用 DMA 内存）
- `rv1126`：RV1109/RV1126（传统）
- `rk1808`：RK1808（传统）

### 架构选项
- `aarch64`：64 位 ARM（推荐用于性能）
- `armhf`：32 位 ARM（用于内存受限设备）

## 构建系统

### 编译数据库
项目已配置生成 `compile_commands.json`，用于 clangd 代码跳转和智能提示。文件位置：
- `build/build_rknn_yolov6_demo_rk3588_linux_aarch64_Release/compile_commands.json`

### 构建输出
- `install/rk3588_linux_aarch64/rknn_yolov6_demo/`: 构建输出目录
- `install/.../rknn_yolov6_demo`: 可执行文件
- `install/.../lib/`: 依赖库文件
- `install/.../model/`: 模型和标签文件

## 模型配置

### 自定义模型设置
使用自定义模型（如缺陷检测）时，需要修改：

1. **标签文件**：更新 `rknn_infer/src/postprocess.cc` 指向您的标签文件
   ```cpp
   #define LABEL_NALE_TXT_PATH "./model/neu-det_6_labels_list.txt"
   ```

2. **类别数量**：更新 `rknn_infer/include/postprocess.h` 以匹配模型的类别数量
   ```cpp
   #define OBJ_CLASS_NUM 6  // 从默认的 80 修改
   ```

3. **CMakeLists.txt**：在 `rknn_infer/CMakeLists.txt` 中更新文件安装路径
   ```cmake
   install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/model/neu-det_6_labels_list.txt DESTINATION model)
   install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/model/neu-det-inclusion_4.jpg DESTINATION model)
   ```

### 模型输出格式
YOLOv6 模型有 9 个输出张量：
- 3 个尺度的检测输出 (80x80, 40x40, 20x20)
- 每个尺度包括：边界框 (4 通道)、类别分数 (N 个类别) 和物体性 (1 通道)

## 环境设置

### 构建依赖项
- CMake 3.10+
- 交叉编译工具链（aarch64-linux-gnu-gcc）
- RKNN 运行时库（librknnrt.so、librga.so）

### 运行时依赖项
- 目标设备上的 RKNN 运行时库
- 通过 LD_LIBRARY_PATH 设置正确的库路径
- 模型文件和标签文件在正确位置

## 重要说明

- 模型必须使用 RKNN-Toolkit2 转换为 RKNN 格式
- 对于 YOLOv6 模型，输入图像会自动调整大小到 640x640
- 输出格式为 NHWC（批次、高度、宽度、通道）
- 检测结果会以边界框和标签的形式绘制在输出图像上
- 使用自定义模型时，确保 OBJ_CLASS_NUM 与模型的输出类别匹配
- 标签文件应该是纯文本，每行一个类别名称
- 所有场景都使用中文交互

## 构建系统细节

### 编译数据库
项目自动生成 `compile_commands.json` 用于代码智能分析，位置：
- `BTP/build/build_rknn_yolov6_demo_rk3588_linux_aarch64_Release/compile_commands.json`
- `BTP/HostPC_DefectRKNN/build/compile_commands.json`

### 构建输出结构
```
BTP/install/
├── rk3588_linux_aarch64/rknn_yolov6_demo/
│   ├── rknn_yolov6_demo          # 可执行文件
│   ├── lib/                      # 依赖库
│   └── model/                    # 模型和标签文件
└── rk3588_linux_aarch64/         # 开发库文件
```

### 依赖库路径
运行时需要设置库路径：
```bash
export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH
```

## 开发注意事项

1. **模型量化**: 模型必须使用 RKNN-Toolkit2 转换为量化格式
2. **内存管理**: RV1106 平台使用 DMA 内存，其他平台使用普通内存
3. **图像格式**: 支持 RGB/BGR 格式，自动转换为模型所需格式
4. **性能优化**: 使用 RGA 进行硬件加速图像处理，多NPU核心并行处理
5. **错误处理**: 所有 API 调用都有错误检查，注意查看返回值
6. **批量处理**: 单个文件失败不影响整体处理流程，支持多种图像格式

## 调试和故障排除

### 常见问题

#### 视频帧分辨率问题
- **症状**: QVideoProbe 获取的帧分辨率与原始视频分辨率不符
- **原因**: Qt 的 QVideoWidget 在渲染时会对视频进行缩放
- **调试方法**:
  ```cpp
  // 获取原始视频分辨率
  QVariant resolution = mediaPlayer->metaData(QMediaMetaData::Resolution);
  // 检查 QVideoWidget 实际大小
  QSize widgetSize = videoWidget->size();
  ```

#### RKNN 模型推理失败
- **检查点**: 确认模型文件路径正确，标签文件格式正确
- **常见错误**: 类别数量不匹配，模型量化问题
- **调试方法**: 检查控制台输出，确认 RKNN 运行时库路径设置

#### 构建错误
- **依赖问题**: 确认交叉编译工具链和 RKNN 运行时库已安装
- **Qt 相关**: 确认 Qt5 开发库和多媒体模块已安装
- **解决方法**: 检查 `build-linux.sh` 脚本中的路径配置

### 开发工作流

#### 修改后的快速验证
```bash
# 修改代码后快速构建
cd BTP/HostPC_DefectRKNN/build
make

# 或者重新构建整个项目
cd BTP
./build-linux.sh
```

#### 代码调试技巧
- 使用 qDebug() 输出调试信息
- 检查 RKNN API 返回值
- 验证图像格式转换过程
- 确认内存分配和释放配对

### 缺陷检测模型配置
当前项目配置用于缺陷检测，包含6个类别：
- cr: 某种缺陷类型
- ic: 某种缺陷类型
- ps: 某种缺陷类型
- rs: 某种缺陷类型
- sc: 某种缺陷类型
- pc: 某种缺陷类型

### 代码格式化
项目使用 `.clang-format` 配置文件进行代码格式化，基于 Microsoft 风格：
- 缩进使用4个空格
- 大括号使用 Allman 风格
- 列限制设置为0（无限制）

## 最新特性

### 多NPU核心优化
- **配置**: 使用 `RKNN_NPU_CORE_0_1_2` 核心配置
- **实现**: 在 `rknn_infer/src/rknpu2/yolov6.cc` 中添加核心掩码设置
- **性能**: 相比单核心处理，性能显著提升，适用于工业实时检测场景

### 批量处理系统
- **命令行**: 支持目录批量处理，自动过滤图片文件
- **GUI界面**: 添加文件夹选择和批量检测功能，带进度显示
- **错误恢复**: 单个文件失败不影响整体处理流程
- **结果管理**: 自动保存检测结果到指定目录，避免覆盖原始文件

### 双架构设计
- **命令行工具**: 适合自动化部署和集成到生产环境
- **GUI应用**: 提供友好的用户界面，适合人工操作和测试
- **统一后端**: 两个组件共享相同的RKNN推理核心，确保结果一致性

### 当前开发状态
- **视频处理**: 正在调试 QVideoProbe 获取原始视频帧的问题
- **帧分辨率**: QVideoProbe 获取的是渲染后帧(1130x500)而非原始视频帧(1920x1080)
- **调试信息**: 已添加视频分辨率和帧处理调试输出
- **简化工作流**: 实现打开视频→开始推理的简化流程