# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

这是一个基于Qt5的RKNN缺陷检测上位机程序，专门用于工业缺陷检测应用。项目集成了Rockchip NPU推理引擎，提供图形化界面进行图像选择、缺陷检测和结果显示。

项目已完成架构重构，采用**三层分层架构**：
1. **UI层** - 纯界面逻辑，不包含业务逻辑
2. **服务层** - 可复用的业务逻辑封装
3. **底层库** - RKNN推理核心（静态链接）

## 核心架构

### 三层架构

```
┌─────────────────────────────────────────────────────────────┐
│                      UI 层 (Presentation)                   │
│  ┌─────────────────┐  ┌─────────────────┐                   │
│  │   MainWindow    │  │  CameraWindow   │                   │
│  │   (纯界面逻辑)   │  │   (纯界面逻辑)   │                   │
│  └────────┬────────┘  └────────┬────────┘                   │
└───────────┼────────────────────┼─────────────────────────────┘
            │                    │
            ▼                    ▼
┌─────────────────────────────────────────────────────────────┐
│                     服务层 (Services)                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │InferenceEngine│ │ImageProcessor│ │StatisticsService │  │
│  │  推理引擎封装  │  │  图像处理封装  │  │   统计服务封装   │  │
│  └──────────────┘  └──────────────┘  └──────────────────┘  │
│  ┌──────────────┐  ┌─────────────────────────────────────┐ │
│  │  FileService │  │     DefectColorManager              │ │
│  │  文件服务封装  │  │       缺陷颜色配置工具类             │ │
│  └──────────────┘  └─────────────────────────────────────┘ │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                    底层 RKNN 库 (原库)                       │
│  rknn_infer/src/rknpu2/yolov6.cc  - RKNN模型推理            │
│  rknn_infer/src/postprocess.cc    - 后处理 (NMS等)          │
│  rknn_infer/utils/image_utils.c   - 图像格式转换            │
└─────────────────────────────────────────────────────────────┘
```

### 文件结构

```
HostPC_DefectRKNN/
├── include/                    # 头文件
│   ├── mainwindow.h           # 主窗口（UI层）
│   ├── camerawindow.h         # 摄像头窗口（UI层）
│   ├── statisticsdialog.h     # 统计对话框
│   ├── inferenceengine.h      # 推理引擎服务
│   ├── imageprocessor.h       # 图像处理服务
│   ├── statisticsservice.h    # 统计服务
│   ├── fileservice.h          # 文件服务
│   └── defect_colors.h        # 缺陷颜色配置
│
├── src/                       # 源文件
│   ├── main.cpp               # 程序入口
│   ├── mainwindow.cpp         # 主窗口实现
│   ├── camerawindow.cpp       # 摄像头窗口实现
│   ├── statisticsdialog.cpp   # 统计对话框实现
│   ├── inferenceengine.cpp    # 推理引擎实现
│   ├── imageprocessor.cpp     # 图像处理实现
│   ├── statisticsservice.cpp  # 统计服务实现
│   ├── fileservice.cpp        # 文件服务实现
│   └── defect_colors.cpp      # 颜色配置实现
│
├── model/                     # 模型文件
│   ├── neu-det-new.rknn       # RKNN模型
│   └── neu-det_6_labels_list.txt
│
├── resources/                 # 资源文件
│   └── logo.png
│
├── build/                     # 构建目录
└── CMakeLists.txt             # 构建配置
```

### 服务类职责

| 类名 | 职责 | 主要接口 |
|------|------|---------|
| `InferenceEngine` | RKNN推理引擎封装 | `initialize()`, `detect()`, `release()` |
| `ImageProcessor` | 图像处理与绘制 | `drawResults()`, `convertResults()` |
| `StatisticsService` | 统计收集计算 | `collect()`, `getStatistics()` |
| `FileService` | 文件操作 | `findImageFiles()`, `saveResultImage()` |
| `DefectColorManager` | 颜色配置 | `getDefectColorConfig()`, `drawDefectBox()` |

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

### 缺陷类别颜色配置
- **cr** (裂纹): 红色
- **ic** (夹杂): 橙色
- **ps** (压痕): 黄色
- **rs** (划痕): 绿色
- **sc** (疤痕): 蓝色
- **pc** (坑点): 紫色

### 路径管理
- **模型路径**: 在`InferenceEngine::initialize()`中使用相对路径动态计算
- **标签路径**: 程序启动时临时切换工作目录确保标签文件加载
- **依赖库**: 所有第三方库使用绝对路径链接

## 开发注意事项

### 头文件依赖管理
- RKNN相关头文件在cpp文件中包含，避免头文件依赖循环
- 服务类使用pimpl模式减少编译依赖
- UI层头文件不包含底层RKNN头文件

### 内存管理
- `InferenceEngine` 使用RAII模式管理RKNN上下文生命周期
- 图像缓冲区使用malloc/free管理
- 使用std::unique_ptr管理服务类实例

### 错误处理
- 所有关键操作都有返回值检查
- 错误信息通过`getLastError()`获取并显示
- RKNN初始化失败会禁用检测功能

### 重构原则
- UI层只负责界面显示，不包含业务逻辑
- 业务逻辑封装在服务类中，可独立测试
- 服务类不依赖QtWidgets，便于在其他环境复用

## 平台支持

- **目标平台**: RK3588 (aarch64)
- **开发环境**: Linux + Qt5
- **运行环境**: 需要显示界面的Linux系统
- **依赖要求**: RKNN运行时库、RGA库、Qt5运行时

## 性能优化

- **图像处理**: 使用Qt的QImage和QPainter进行图像显示
- **内存管理**: 避免不必要的图像拷贝
- **UI响应**: 使用QApplication::processEvents()保持界面响应
- **推理效率**: 使用多NPU核心并行处理
