QT += core gui webenginewidgets network

CONFIG += c++11

TARGET = PathPlanning
TEMPLATE = app

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    PathCalculator.cpp

HEADERS += \
    MainWindow.h \
    PathCalculator.h

# 添加编译选项
win32 {
    QMAKE_CXXFLAGS += -std=c++11
}
