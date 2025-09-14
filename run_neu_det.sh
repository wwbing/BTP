#!/bin/bash

# RKNN YOLOv6 推理一键运行脚本
# 用于RK3588平台的neu-det模型推理

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的信息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查必要文件是否存在
check_files() {
    print_info "检查必要文件..."
    
    if [ ! -f "rknn_infer/model/neu-det-new.rknn" ]; then
        print_error "模型文件不存在: rknn_infer/model/neu-det-new.rknn"
        exit 1
    fi
    
    if [ ! -f "rknn_infer/model/neu-det_6_labels_list.txt" ]; then
        print_error "标签文件不存在: rknn_infer/model/neu-det_6_labels_list.txt"
        exit 1
    fi
    
    if [ ! -f "images/inclusion_4.jpg" ]; then
        print_error "测试图像不存在: images/inclusion_4.jpg"
        exit 1
    fi
    
    if [ ! -f "build-linux.sh" ]; then
        print_error "构建脚本不存在: build-linux.sh"
        exit 1
    fi
    
    print_success "所有必要文件检查通过"
}

# 清理之前的构建
clean_build() {
    print_info "清理之前的构建文件..."
    rm -rf build/ install/
    print_success "构建文件清理完成"
}

# 执行编译
build_project() {
    print_info "开始编译项目..."
    if ! ./build-linux.sh; then
        print_error "编译失败"
        exit 1
    fi
    print_success "编译完成"
}

# 执行推理
run_inference() {
    print_info "开始推理测试..."
    
    local install_dir="install/rk3588_linux_aarch64/rknn_yolov6_demo"
    
    # 检查可执行文件
    if [ ! -f "$install_dir/rknn_yolov6_demo" ]; then
        print_error "可执行文件不存在: $install_dir/rknn_yolov6_demo"
        exit 1
    fi
    
    # 复制测试图像
    if [ ! -f "$install_dir/model/inclusion_4.jpg" ]; then
        print_info "复制测试图像..."
        cp images/inclusion_4.jpg "$install_dir/model/"
    fi
    
    # 执行推理
    cd "$install_dir"
    export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH
    
    print_info "执行推理命令..."
    if ! ./rknn_yolov6_demo model/neu-det-new.rknn model/inclusion_4.jpg; then
        print_error "推理失败"
        exit 1
    fi
    
    # 检查输出
    if [ -f "out.png" ]; then
        print_success "推理完成！输出图像: out.png"
        ls -la out.png
    else
        print_error "输出图像未生成"
        exit 1
    fi
}

# 主函数
main() {
    echo "=============================================="
    echo "      RKNN YOLOv6 推理一键运行脚本"
    echo "      平台: RK3588"
    echo "      模型: neu-det-new.rknn"
    echo "=============================================="
    
    # 检查是否在正确的目录
    if [ ! -f "build-linux.sh" ]; then
        print_error "请在项目根目录下运行此脚本"
        exit 1
    fi
    
    # 执行步骤
    check_files
    clean_build
    build_project
    run_inference
    
    echo ""
    print_success "所有步骤执行完成！"
    echo "=============================================="
}

# 运行主函数
main "$@"