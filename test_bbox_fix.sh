#!/bin/bash

# 测试边界框修复效果
echo "开始测试边界框修复效果..."

# 检查是否存在测试图片
if [ ! -f "model/neu-det-inclusion_4.jpg" ]; then
    echo "错误: 测试图片不存在，请检查 model/neu-det-inclusion_4.jpg"
    exit 1
fi

# 编译项目
echo "正在编译项目..."
./build-linux.sh

if [ $? -ne 0 ]; then
    echo "编译失败"
    exit 1
fi

echo "编译成功，开始测试..."

# 进入构建输出目录
cd install/rk3588_linux_aarch64/rknn_yolov6_demo

# 设置库路径
export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH

# 运行测试
echo "运行RKNN推理测试..."
./rknn_yolov6_demo model/neu-det-new.rknn model/neu-det-inclusion_4.jpg

if [ $? -eq 0 ]; then
    echo "测试完成！请检查 out.jpg 文件中的边界框是否正确绘制在图像内容区域，而不是黑边区域。"
    echo "关键检查点："
    echo "1. 边界框应该紧贴缺陷目标，不应该延伸到黑边区域"
    echo "2. 边界框坐标应该对应原始图像的有效内容区域"
    echo "3. 对于不同宽高比的图片，边界框位置都应该准确"

    # 显示输出图片信息
    if [ -f "out.jpg" ]; then
        echo "输出文件: out.jpg"
        ls -la out.jpg
    fi
else
    echo "测试失败"
    exit 1
fi