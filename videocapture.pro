QMAKE_CXX = g++-4.4
QMAKE_CXXFLAGS = -std=c++0x
QMAKE_CC = gcc-4.4
QMAKE_LFLAGS = -std=c++0x

TEMPLATE = app

DESTDIR = .
TARGET = videocapture

CONFIG += warn_on debug

#insert library paths here
DEPENDPATH +=

INCLUDEPATH += ./src
LIBS += -ljpegsimd


#where to put temporary stuff
MOC_DIR = tmp/
UI_DIR = tmp/
RCC_DIR = tmp/
OBJECTS_DIR = tmp/

#additional defs
DEFINES += 


# Input
HEADERS += ./src/MainWindow.hpp \
           ./src/RawImageDrawThread.hpp \
           ./src/StreamReadThread.hpp

SOURCES += ./src/main.cpp \
           ./src/MainWindow.cpp \
           ./src/RawImageDrawThread.cpp \
           ./src/StreamReadThread.cpp

