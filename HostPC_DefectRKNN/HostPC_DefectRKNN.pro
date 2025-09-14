QT += core widgets
CONFIG += c++17

TARGET = HostPC_DefectRKNN
CONFIG += debug_and_release

TEMPLATE = app

# 定义输出目录
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build
MOC_DIR = $$PWD/build
RCC_DIR = $$PWD/build
UI_DIR = $$PWD/build

# 包含路径
INCLUDEPATH += include
INCLUDEPATH += $$PWD/../rknn_model_zoo-2.3.2/utils
INCLUDEPATH += $$PWD/../rknn_model_zoo-2.3.2/3rdparty/rknpu2/include

# 源文件
SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp

# 头文件
HEADERS += \
    include/mainwindow.h

# RKNN库路径
RKNN_LIB_PATH = $$PWD/../rknn_model_zoo-2.3.2/install/rk3588_linux_aarch64/rknn_yolov6_demo/lib

# 库文件
LIBS += -L$$RKNN_LIB_PATH -lrknnrt -lrga

# 安装
target.path = /usr/local/bin
INSTALLS += target