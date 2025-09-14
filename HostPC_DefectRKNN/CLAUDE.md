# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

这是一个基于Qt5的RKNN缺陷检测上位机程序，专门用于工业缺陷检测应用。项目集成了Rockchip NPU推理引擎，提供图形化界面进行图像选择、缺陷检测和结果显示。

## 核心架构

### Qt界面层
- **MainWindow** (`src/mainwindow.cpp`, `include/mainwindow.h`): 主窗口类，负责UI管理和用户交互
- **主要功能**: 图像选择、RKNN推理、结果显示、状态反馈
- **UI组件**: 图像显示标签、打开/检测按钮、状态栏

### RKNN推理层
项目通过静态链接方式集成了rknn_infer项目的核心组件：
- **模型初始化** (`../rknn_infer/src/rknpu2/yolov6.cc`): RKNN模型加载和初始化
- **后处理** (`../rknn_infer/src/postprocess.cc`): NMS过滤和结果解码
- **图像处理** (`../rknn_infer/utils/image_utils.c`): 图像读写和预处理
- **结果绘制** (`../rknn_infer/utils/image_drawing.c`): 原始图像绘制功能

### 关键依赖
- **RKNN运行时**: `librknnrt.so` - Rockchip NPU推理引擎
- **RGA库**: `librga.so` - 硬件加速图像处理
- **图像处理**: `libturbojpeg.a` - JPEG编解码
- **Qt5**: Core和Widgets模块 - GUI框架

## 常用命令

### 构建项目
```bash
cd /home/cat/wwbing/Code/Project/BTP/HostPC_DefectRKNN
mkdir -p build && cd build
cmake ..
make
```

### 运行程序
```bash
# 在有显示界面的环境中运行
cd /home/cat/wwbing/Code/Project/BTP/HostPC_DefectRKNN/build
export LD_LIBRARY_PATH=../3rdparty/rknpu2/Linux/aarch64:../3rdparty/librga/Linux/aarch64:$LD_LIBRARY_PATH
./HostPC_DefectRKNN
```

### 清理构建产物
```bash
cd /home/cat/wwbing/Code/Project/BTP/HostPC_DefectRKNN/build
make clean
rm -rf *
```

## 关键配置点

### 模型配置
- **模型文件**: `model/neu-det-new.rknn` - YOLOv6缺陷检测模型
- **标签文件**: `model/neu-det_6_labels_list.txt` - 6个缺陷类别标签
- **类别定义**: cr(裂纹), ic(夹杂), ps(压痕), rs(划痕), sc(疤痕), pc(坑点)

### 路径管理
- **模型路径**: 在`initializeRKNN()`中使用相对路径动态计算
- **标签路径**: 程序启动时临时切换工作目录确保标签文件加载
- **依赖库**: 所有第三方库使用绝对路径链接

### 显示配置
- **检测框**: 蓝色边框 (COLOR_BLUE: 0xFF0000FF)
- **标签文字**: 红色文字 (COLOR_RED: 0xFFFF0000)
- **标签背景**: 半透明白色背景确保文字清晰可见

## 开发注意事项

### 头文件依赖管理
- RKNN相关结构体在cpp文件中包含，避免头文件依赖
- 使用void*指针管理RKNN上下文，减少编译依赖
- Qt相关头文件在头文件中包含，RKNN相关在cpp文件中包含

### 内存管理
- RKNN应用上下文使用malloc分配，在析构函数中释放
- 图像缓冲区在使用后需要手动释放virt_addr
- 使用RAII原则管理Qt对象

### 错误处理
- 所有关键操作都有返回值检查
- 错误信息通过状态栏和消息框显示
- RKNN初始化失败会禁用检测功能

### 构建系统
- 使用CMake管理项目，支持compile_commands.json生成
- 静态链接rknn_infer源码，避免动态库依赖
- 包含路径使用相对路径，确保可移植性

## 文件结构

```
HostPC_DefectRKNN/
├── CMakeLists.txt          # 构建配置
├── HostPC_DefectRKNN.pro    # Qt项目文件
├── include/
│   └── mainwindow.h       # 主窗口头文件
├── src/
│   ├── main.cpp           # 程序入口
│   └── mainwindow.cpp     # 主窗口实现
├── model/
│   ├── neu-det-new.rknn  # RKNN模型文件
│   └── neu-det_6_labels_list.txt  # 标签文件
├── build/                  # 构建输出目录
└── resources/             # 资源文件目录
```

## 平台支持

- **目标平台**: RK3588 (aarch64)
- **开发环境**: Linux + Qt5
- **运行环境**: 需要显示界面的Linux系统
- **依赖要求**: RKNN运行时库、RGA库、Qt5运行时

## 性能优化

- **图像处理**: 使用Qt的QImage和QPainter进行图像显示
- **内存管理**: 避免不必要的图像拷贝
- **UI响应**: 使用QApplication::processEvents()保持界面响应
- **推理效率**: 直接使用rknn_infer的优化实现