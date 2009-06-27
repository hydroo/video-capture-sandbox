#include "Prereqs.hpp"

#include <iostream>
#include <list>
#include <functional>
#include <string>
#include <thread>
#include <QApplication>
#include "MainWindow.hpp"

using std::bind;
using std::cerr;
using std::cout;
using std::endl;
using std::list;
using std::string;
using std::thread;


void ReadThread2(int a)
{
    (void) a;
}


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


    thread t(bind(ReadThread2, 2/* test*/));
    t.join();



    MainWindow mainWindow(0);

    mainWindow.show();

    return app.exec();
}

