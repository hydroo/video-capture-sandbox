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

#include "prereqs.hpp"

#include "basefilter.hpp"
#include "capturedevice.hpp"
#include "mainwindow.hpp"

#include <QApplication>

#include <cassert>
#include <cerrno>
#include <iostream>
#include <list>
#include <set>
#include <string>

#include <dirent.h>
#include <dlfcn.h>
#include <sys/types.h>

using namespace std;


int main(int argc, char **args)
{
    VT

    QApplication app(argc, args);


    string executablePath(args[0]);
    string::size_type positionOfLastSlash = executablePath.find_last_of("/");
    if (positionOfLastSlash != string::npos && positionOfLastSlash+1 < executablePath.length()
            ) { executablePath.resize(positionOfLastSlash); }
    else { /* can this happen? I dont know */ assert(0); }


    /* transform the arg array into a nice list of strings */
    list<string> argList;
    for( int i = 0; i < argc; ++i ) {
        argList.push_back(string(args[i]));
    }

    set<CaptureDevice*> captureDevices;

    /* *** evaluate arguments start *** */
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
            newCaptureDevice->printControls();
            cout << endl;

            assert(captureDevices.find(newCaptureDevice) == captureDevices.end());
            captureDevices.insert(newCaptureDevice);

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
    /* *** evaluate arguments end *** */

    set<pair<CreateFilterFunction, DestroyFilterFunction> > filters;
    set<void*> filterLibraryHandles;
    /* *** load filters *** */
    set<string> filterSearchDirectories;
    filterSearchDirectories.insert(".");
    filterSearchDirectories.insert(executablePath);

    for (auto it = filterSearchDirectories.begin(); it != filterSearchDirectories.end(); ++it) {

        DIR *directoryHandle = opendir(it->c_str());

        if (directoryHandle == 0) {
            cerr << "Cannot open directory \"" << *it << "\" " << errno << " " << strerror(errno) << endl;
            continue;
        }

        for(;;)
        {
            struct dirent *directoryEntry = readdir(directoryHandle);
            if (directoryEntry == 0) break;

            string fileName = *it + '/' + directoryEntry->d_name;

            void *handle = dlopen(fileName.c_str(), RTLD_NOW);
            if (handle == 0) {
                /* it is not a library */
                //cerr << "Cannot open library \"" << fileName << "\" " << dlerror() << endl; 
                continue;
            }

            CreateFilterFunction create = reinterpret_cast<CreateFilterFunction>(dlsym(handle, "create"));
            DestroyFilterFunction destroy = reinterpret_cast<DestroyFilterFunction>(dlsym(handle, "destroy"));

            if (create != 0 && destroy != 0) {

                filterLibraryHandles.insert(handle);
                filters.insert(make_pair(create, destroy));

            } else {
                /* it is a library, but has not the create() function, because it probably is not a filter plugin */
                cerr << "Cannot load filter library symbols \"" << fileName << "\" " << dlerror() << endl;
            }

        }
    }
    /* *** load filters end *** */


    MainWindow mainWindow(0, captureDevices, filters);
    mainWindow.show();

    int ret = app.exec();


    for (auto it = captureDevices.begin(); it != captureDevices.end(); ++it) {
        (*it)->finish();
    }

    for (auto it = filterLibraryHandles.begin(); it != filterLibraryHandles.end(); ++it) {
        int dlcloseRet = dlclose(*it);
        assert(dlcloseRet == 0);
    }

    return ret;
}

