#!/bin/bash

set -e

# 默认值 - 固定 RK3588 平台
TARGET_SOC="rk3588"
TARGET_ARCH="aarch64"
BUILD_TYPE="Release"
ENABLE_ASAN="OFF"
DISABLE_RGA="OFF"
DISABLE_LIBJPEG="OFF"

# 参数解析
while getopts ":hb:m:r:j" opt; do
  case $opt in
    h)
      echo "用法: $0 [-b <build_type>] [-m] [-r] [-j]"
      echo ""
      echo "选项:"
      echo "    -b : 构建类型 (Debug/Release, 默认: Release)"
      echo "    -m : 启用地址 sanitizer"
      echo "    -r : 禁用 RGA"
      echo "    -j : 禁用 libjpeg"
      echo ""
      echo "示例: $0 -b Debug"
      exit 0
      ;;
    b)
      BUILD_TYPE=$OPTARG
      ;;
    m)
      ENABLE_ASAN="ON"
      export ENABLE_ASAN=TRUE
      ;;
    r)
      DISABLE_RGA="ON"
      ;;
    j)
      DISABLE_LIBJPEG="ON"
      ;;
    :)
      echo "Option -$OPTARG requires an argument." 
      exit 1
      ;;
    ?)
      echo "Invalid option: -$OPTARG"
      exit 1
      ;;
  esac
done

# 固定平台，不需要这个检查

# 设置编译器 - 固定使用 aarch64
GCC_COMPILER=aarch64-linux-gnu

export CC=${GCC_COMPILER}-gcc
export CXX=${GCC_COMPILER}-g++

# 检查编译器是否存在
if ! command -v ${CC} >/dev/null 2>&1; then
    echo "错误: ${CC} 不可用"
    exit 1
fi

# 查找演示路径
BUILD_DEMO_PATH="rknn_infer"
if [[ ! -d "${BUILD_DEMO_PATH}" ]]; then
    echo "错误: 找不到源代码目录: ${BUILD_DEMO_PATH}"
    exit 1
fi

# 目标平台映射
case ${TARGET_SOC} in
    rk3562|rk3566|rk3568)
        TARGET_SOC="rk356x"
        ;;
    rv1103)
        TARGET_SOC="rv1106"
        ;;
    rk1808|rv1109)
        ;;
    rk3588|rk3576|rv1106|rv1126|rv1126b)
        ;;
    *)
        echo "错误: 无效的目标平台: ${TARGET_SOC}"
        echo "支持的目标: rk3588, rk356x, rk3576, rv1106, rv1126, rv1126b, rk1808, rv1109"
        exit 1
        ;;
esac

# 设置路径
ROOT_PWD=$( cd "$( dirname $0 )" && pwd )
TARGET_PLATFORM=${TARGET_SOC}_linux_${TARGET_ARCH}
TARGET_SDK="rknn_yolov6_demo"
BUILD_DIR=${ROOT_PWD}/build/build_${TARGET_SDK}_${TARGET_PLATFORM}_${BUILD_TYPE}
INSTALL_DIR=${ROOT_PWD}/install/${TARGET_PLATFORM}/${TARGET_SDK}

# 显示配置信息
echo "==================================="
echo "构建配置"
echo "==================================="
echo "演示路径: ${BUILD_DEMO_PATH}"
echo "目标平台: ${TARGET_SOC}"
echo "架构: ${TARGET_ARCH}"
echo "构建类型: ${BUILD_TYPE}"
echo "地址 sanitizer: ${ENABLE_ASAN}"
echo "禁用 RGA: ${DISABLE_RGA}"
echo "禁用 libjpeg: ${DISABLE_LIBJPEG}"
echo "编译器: ${CC}"
echo "构建目录: ${BUILD_DIR}"
echo "安装目录: ${INSTALL_DIR}"
echo "==================================="

# 创建构建目录
mkdir -p ${BUILD_DIR}

# 清理安装目录
if [[ -d "${INSTALL_DIR}" ]]; then
    rm -rf ${INSTALL_DIR}
fi

# 构建项目
cd ${BUILD_DIR}
cmake ../../${BUILD_DEMO_PATH} \
    -DTARGET_SOC=${TARGET_SOC} \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=${TARGET_ARCH} \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DENABLE_ASAN=${ENABLE_ASAN} \
    -DDISABLE_RGA=${DISABLE_RGA} \
    -DDISABLE_LIBJPEG=${DISABLE_LIBJPEG} \
    -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}

make -j$(nproc)
make install

# 检查 RKNN 模型文件
if ! ls ${INSTALL_DIR}/model/*.rknn 1> /dev/null 2>&1; then
    echo "警告: 在 ${INSTALL_DIR}/model/ 中找不到 RKNN 模型文件"
fi

echo "构建完成！"
echo "编译数据库: ${BUILD_DIR}/compile_commands.json"
echo "安装目录: ${INSTALL_DIR}"