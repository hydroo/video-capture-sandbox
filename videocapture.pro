# videocapture is a tool with no special purpose
# 
# Copyright (C) 2009 Ronny Brendel <ronnybrendel@gmail.com>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>


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

#include(enable-vampirtrace.pri)

