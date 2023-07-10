CONFIG -= qt
TARGET = robot
TEMPLATE = lib

CONFIG += dll

CONFIG(debug, debug|release) {
    DESTDIR = output/debug
    LIBS += -Loutput/debug
} else {
    DESTDIR = output/release
    LIBS += -Loutput/release
}

SOURCES += \
    ../robot/main.cpp \
    ../robot/robotapi.cpp
