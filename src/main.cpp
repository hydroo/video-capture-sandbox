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

    list<CaptureDevice*> captureDevices;

    /* evaluate arguments start */
    auto it = argList.begin();
    ++it;
    for (; it != argList.end(); ++it) {
        if (*it == "-d") {
            CaptureDevice *newCaptureDevice = new CaptureDevice();

            string deviceFile = *(++it);
            int width = atoi((++it)->c_str());
            int height = atoi((++it)->c_str());

            assert(deviceFile.empty() == false);
            assert(width > 0);
            assert(height > 0);

            newCaptureDevice->setFileName(deviceFile);
            newCaptureDevice->setCaptureSize(width, height);

            bool initialized = newCaptureDevice->init();
            assert(initialized);

            newCaptureDevice->printDeviceInfo();
            newCaptureDevice->printFormats();
            newCaptureDevice->printTimerInformation();
            newCaptureDevice->printControls();
            cout << endl;

            captureDevices.push_back(newCaptureDevice);

        } else if (*it == "-h" || *it == "--help") {
            cout
                << "videocapture [-d ...] [-d ...] [-d ...] ..." << endl
                << endl
                << "  arguments:" << endl
                << "    -d <device file> <res width> <res height>   use this device" << endl
                << "    -h, --help                                  show this message" << endl;
            return 0;
        } else {
            cerr << "unknown argument: \"" << *it << endl;
        }
    }
    /* evaluate arguments end */

    MainWindow mainWindow(0, captureDevices);
    mainWindow.show();

    int ret = app.exec();


    for (auto it = captureDevices.begin(); it != captureDevices.end(); ++it) {
        (*it)->finish();
    }

    return ret;
}

