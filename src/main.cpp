#include "Prereqs.hpp"

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

    CaptureDevice camera;
    camera.init("/dev/video0", V4L2_PIX_FMT_RGB24, 352, 288);
    camera.printDeviceInfo();
    camera.printFormats();
    camera.printControls();
    camera.printTimerInformation();

    /*pair<double, double> captureRate = camera.determineCapturePeriod();
    cout << "Capture Period: " << captureRate.first
            << "s, StdDeviation: " << fixed << captureRate.second << endl;*/


    MainWindow mainWindow(0, camera);
    mainWindow.show();

    int ret = app.exec();


    camera.finish();


    return ret;
}

