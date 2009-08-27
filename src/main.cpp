/* videocapture is a tool with no special purpose
 *
 * Copyright (C) 2009 Ronny Brendel <ronnybrendel@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "Prereqs.hpp"

#include <cassert>
#include <iostream>
#include <list>
#include <string>
#include <QApplication>
#include "CaptureDevice.hpp"
#include "MainWindow.hpp"


using namespace std;


int main(int argc, char **args)
{
    VT

    QApplication app(argc, args);


    /* transform the arg array into a nice list of strings */
    list<string> argList;
    for( int i = 0; i < argc; ++i ) {
        argList.push_back(string(args[i]));
    }

    /* evaluate arguments start */
    /* evaluate arguments end */

    CaptureDevice camera1;
    CaptureDevice camera2;

    bool initialized = camera1.init("/dev/video0", V4L2_PIX_FMT_RGB24, 352, 288);
    assert(initialized);

    camera1.printDeviceInfo();
    camera1.printFormats();
    camera1.printTimerInformation();
    camera1.printControls();

    bool initialized2 = camera2.init("/dev/video1", V4L2_PIX_FMT_RGB24, 352, 288);
    assert(initialized2);

    camera2.printDeviceInfo();
    camera2.printFormats();
    camera2.printTimerInformation();
    camera2.printControls();


    MainWindow mainWindow(0, camera1, camera2);
    mainWindow.show();

    int ret = app.exec();


    camera1.finish();
    camera2.finish();


    return ret;
}

