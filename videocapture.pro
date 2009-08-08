#QMAKE_CXX = vtcxx 
#QMAKE_CXXFLAGS = -vt:cxx g++-4.4 -std=c++0x
#QMAKE_CC = gcc-4.4
#QMAKE_CFLAGS = 
#QMAKE_LINK = vtcxx
#QMAKE_LFLAGS = -vt:cxx g++-4.4 -std=c++0x
QMAKE_CXX = g++-4.4
QMAKE_CXXFLAGS = -std=c++0x
QMAKE_CC = gcc-4.4
QMAKE_CFLAGS = 
QMAKE_LINK = g++-4.4
QMAKE_LFLAGS = -std=c++0x

TEMPLATE = app

DESTDIR = .
TARGET = videocapture

CONFIG += warn_on debug

#insert library paths here
DEPENDPATH +=

INCLUDEPATH += ./src
LIBS += -lrt -lv4l2

MOC_DIR = tmp/
UI_DIR = tmp/
RCC_DIR = tmp/
OBJECTS_DIR = tmp/

DEFINES += 

HEADERS += ./src/CaptureDevice.hpp \
           ./src/MainWindow.hpp

SOURCES += ./src/CaptureDevice.cpp \
           ./src/main.cpp \
           ./src/MainWindow.cpp

