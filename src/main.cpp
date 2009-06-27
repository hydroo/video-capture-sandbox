#include "Prereqs.hpp"

#include <cassert>
#include <iostream>
#include <list>
#include <string>
#include <thread>
#include <QApplication>
#include "MainWindow.hpp"

using std::cerr;
using std::cout;
using std::list;
using std::string;
using std::thread;


class ReadThread
{
public:
    void operator()()
    {
        cerr << __PRETTY_FUNCTION__;

        for(;;) {
            cerr << "a";
        }
    }
};

void ReadThread2()
{
    cerr << __PRETTY_FUNCTION__;

    for(;;) {
        cerr << "a";
    }
}


int main(int argc, char **args)
{
    QApplication app(argc, args);


    /* transform the arg array into a nice list of strings */
    list<string> argList;
    for( int i = 0; i < argc; ++i ) {
        argList.push_back(string(args[i]));
    }


    thread t(ReadThread2);
    //t.join();

    /* evaluate arguments start */
    /* evaluate arguments end */


    MainWindow mainWindow(0);

    mainWindow.show();

    return app.exec();
}

