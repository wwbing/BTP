#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("缺陷检测上位机");
    app.setApplicationVersion("1.0");

    // 创建主窗口
    MainWindow mainWindow;
    mainWindow.show();

    // 运行应用程序
    return app.exec();
}