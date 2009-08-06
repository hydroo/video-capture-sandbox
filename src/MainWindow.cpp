#include "MainWindow.hpp"

#include <functional>
#include <iostream>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <thread>
#include "Camera.hpp"

using namespace std;


MainWindow::MainWindow(QWidget *parent, Camera& camera) :
        QMainWindow(parent),
        m_camera(camera),
        m_paintThreadCancellationFlag(false)
{
    /* *** init ui *** */
    m_mainLayout = new QHBoxLayout();
    m_buttonLayout = new QVBoxLayout();

    m_centralWidget = new QWidget();

    m_rawImageLabel = new QLabel();
    m_rawImageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_startStopButton = new QPushButton(tr("Start"));
    m_startStopButton->setCheckable(true);
    connect(m_startStopButton, SIGNAL(clicked(bool)),
            this, SLOT(startStopButtonClicked(bool)));

    m_buttonLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_buttonLayout->addWidget(m_startStopButton);

    m_mainLayout->addWidget(m_rawImageLabel);
    m_mainLayout->addLayout(m_buttonLayout);

    m_centralWidget->setLayout(m_mainLayout);

    setCentralWidget(m_centralWidget);


    /* *** camera initialization*** */
    m_camera.startCapturing();

    m_paintThread = new std::thread(bind(paintThread, this));
}


MainWindow::~MainWindow()
{
}


QSize MainWindow::sizeHint() const
{
    return QSize(800,600);
}


void MainWindow::closeEvent(QCloseEvent * /*event*/)
{
    m_paintThreadCancellationFlag = true;
    m_camera.finish();
}


void MainWindow::startStopButtonClicked(bool checked)
{
    (void) checked;
    // TODO
}


void MainWindow::paintThread(MainWindow *window)
{
    Camera* camera = &(window->m_camera);


    timespec lastpicture = {numeric_limits<time_t>::max(), 0};

    while (window->m_paintThreadCancellationFlag == false) {
#if 1
        // cerr << camera->newerBuffersAvailable(lastpicture) << endl;
        
        if (camera->newerBuffersAvailable(lastpicture) > 0) {

            deque<const Camera::Buffer*> buffers = camera->lockFirstNBuffers(1);
            const Camera::Buffer* buffer = buffers[0];

            lastpicture = buffer->time;

            /* do something with the picture */

            cerr << "read " << buffer << endl;

            camera->unlock(buffers);

        } else {

            // struct timespec sleepLength = { 1, 0 };
            // clock_nanosleep(CLOCK_REALTIME, 0, &sleepLength, 0);

        }
#endif
    }
}

