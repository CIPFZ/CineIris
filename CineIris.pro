QT       += core gui widgets svg

TARGET = CineIris
TEMPLATE = app
CONFIG += c++11

# --- 核心配置：告诉 Qt 哪里去找 FFmpeg ---
# $$PWD 代表当前 .pro 文件所在的目录

# 1. 头文件路径 (代码里 #include <libavcodec/...> 能找到文件)
INCLUDEPATH += $$PWD/ffmpeg/include

# 2. 库文件路径 (链接器能找到 .lib 文件)
LIBS += -L$$PWD/ffmpeg/lib \
        -lavcodec \
        -lavformat \
        -lavutil \
        -lswscale

# 3. 依赖检测 (修改头文件后自动重新编译)
DEPENDPATH += $$PWD/ffmpeg/include

# -------------------------------------

SOURCES += \
    fingerprintworker.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    fingerprintworker.h \
    mainwindow.h \
    taskdata.h

FORMS +=
