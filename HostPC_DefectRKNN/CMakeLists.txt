cmake_minimum_required(VERSION 3.16)
project(HostPC_DefectRKNN)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成 compile_commands.json 用于 clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 包含目录
include_directories(include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../rknn_infer/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../rknn_infer/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../rknn_infer/utils)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/rknpu2/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/librga/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/stb_image)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/jpeg_turbo/include)

# 查找Qt5包
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 源文件
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../rknn_infer/src/rknpu2/yolov6.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../rknn_infer/src/postprocess.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../rknn_infer/utils/file_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../rknn_infer/utils/image_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../rknn_infer/utils/image_drawing.c
)

# 头文件
set(HEADERS
    include/mainwindow.h
)

# RKNN库路径
set(RKNN_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../rknn_model_zoo-2.3.2/install/rk3588_linux_aarch64/rknn_yolov6_demo/lib)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 链接Qt库
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core Qt5::Widgets)

# 不使用OpenCV，仅使用Qt的图像处理功能

# 链接RKNN库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/rknpu2/Linux/aarch64/librknnrt.so
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/librga/Linux/aarch64/librga.so
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/jpeg_turbo/Linux/aarch64/libturbojpeg.a
    m
    dl
)

# 安装目标
install(TARGETS ${PROJECT_NAME} DESTINATION bin)