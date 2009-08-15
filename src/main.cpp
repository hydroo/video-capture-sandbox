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

    bool initialized2 = camera2.init("/dev/video1", V4L2_PIX_FMT_RGB24, 352, 288);
    assert(initialized2);


    MainWindow mainWindow(0, camera1, camera2);
    mainWindow.show();

    int ret = app.exec();


    camera1.finish();
    camera2.finish();


    return ret;
}

