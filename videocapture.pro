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


TARGET = videocapture
TEMPLATE = app
DESTDIR = .

QMAKE_CXX = g++-4.4
QMAKE_CXXFLAGS = -std=c++0x
QMAKE_CC = gcc-4.4
QMAKE_CFLAGS = 
QMAKE_LINK = g++-4.4
QMAKE_LFLAGS = -std=c++0x -Wl,-export-dynamic

CONFIG += warn_on debug

CONFIG += link_pkgconfig
PKGCONFIG += libv4l2


DEFINES += 

INCLUDEPATH += ./src

DEPENDPATH +=
LIBS += -lrt


MOC_DIR = tmp/
UI_DIR = tmp/
RCC_DIR = tmp/
OBJECTS_DIR = tmp/


HEADERS += ./src/basefilter.hpp \
           ./src/capturedevice.hpp \
           ./src/capturedevicesTab.hpp \
           ./src/filtereditorTab.hpp \
           ./src/mainwindow.hpp \
           ./src/viewstab.hpp

SOURCES += ./src/basefilter.cpp \
           ./src/capturedevice.cpp \
           ./src/capturedevicesTab.cpp \
           ./src/filtereditortab.cpp \
           ./src/main.cpp \
           ./src/mainwindow.cpp \
           ./src/viewstab.cpp


include(filters.pri)

#include(enable-vampirtrace.pri)

