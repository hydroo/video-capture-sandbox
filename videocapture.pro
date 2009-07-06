#a
QMAKE_CXX = g++-4.4
QMAKE_CXXFLAGS = -std=c++0x
QMAKE_CC = gcc-4.4
QMAKE_CFLAGS = gcc-4.4
QMAKE_LINK = g++-4.4
QMAKE_LFLAGS = -std=c++0x

TEMPLATE = app

DESTDIR = .
TARGET = videocapture

CONFIG += warn_on debug

#insert library paths here
DEPENDPATH +=

INCLUDEPATH += ./src
LIBS += -ljpegsimd -lrt

MOC_DIR = tmp/
UI_DIR = tmp/
RCC_DIR = tmp/
OBJECTS_DIR = tmp/

DEFINES += 

HEADERS += ./src/Camera.hpp \
           ./src/MainWindow.hpp \
           ./src/RawImageDrawThread.hpp \
           ./src/StreamReadThread.hpp

SOURCES += ./src/Camera.cpp \
           ./src/main.cpp \
           ./src/MainWindow.cpp \
           ./src/RawImageDrawThread.cpp \
           ./src/StreamReadThread.cpp

