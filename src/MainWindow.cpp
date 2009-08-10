#include "MainWindow.hpp"

#include <cassert>
#include <functional>
#include <iostream>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QVBoxLayout>
#include <thread>
#include "CaptureDevice.hpp"

using namespace std;


MainWindow::MainWindow(QWidget *parent, CaptureDevice& camera) :
        QMainWindow(parent),
        m_camera(camera),
        m_paintThread(0),
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
    stopPaintThread();
    m_camera.stopCapturing();

}


void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter p(this);

    m_currentCaptureImageMutex.lock();
    p.drawImage(event->rect().topLeft(), m_currentCaptureImage, event->rect());
    m_currentCaptureImageMutex.unlock();

    p.end();

    QWidget::paintEvent(event);
}


void MainWindow::startStopButtonClicked(bool checked)
{
    if (checked) {
        m_camera.startCapturing();
        startPaintThread();
    } else {
        stopPaintThread();
        m_camera.stopCapturing();
    }
}


void MainWindow::paintThread(MainWindow *window)
{
    CaptureDevice *camera = &(window->m_camera);

    struct timespec lastpicture = {numeric_limits<time_t>::min(), 0};

    while (window->m_paintThreadCancellationFlag == false) {
        // cerr << camera->newerBuffersAvailable(lastpicture) << endl;
        
        if (camera->newerBuffersAvailable(lastpicture) > 0) {

            deque<const CaptureDevice::Buffer*> buffers = camera->lockFirstNBuffers(1);
            const CaptureDevice::Buffer* buffer = buffers[0];

            lastpicture = buffer->time;

#if 0
            FILE *fp = fopen("capturedatadump", "wb");
            if (!fp) exit(1);
            fwrite(buffer, camera->bufferSize(), sizeof(unsigned char), fp);
            fclose(fp);
#endif

            /*cerr << camera->captureSize().first << "x" << camera->captureSize().second << " "
                    << camera->bufferSize() << " " << 352*288*3 << endl;*/
            
            window->m_currentCaptureImageMutex.lock();
            window->m_currentCaptureImage = QImage(buffer->buffer, camera->captureSize().first, camera->captureSize().second,
                    QImage::Format_RGB888);
            window->m_currentCaptureImageMutex.unlock();

            camera->unlock(buffers);

            window->update();


        } else {

            struct timespec sleepLength = { 0, 1000 };
            clock_nanosleep(CLOCK_MONOTONIC, 0, &sleepLength, 0);

        }
    }
}


void MainWindow::startPaintThread()
{
    assert(m_paintThread == 0);
    m_paintThread = new std::thread(bind(paintThread, this));
}


void MainWindow::stopPaintThread()
{
    if (m_paintThread != 0) {
        assert(m_paintThread->joinable() == true);

        m_paintThreadCancellationFlag = true;
        m_paintThread->join();
        m_paintThreadCancellationFlag = false;

        delete m_paintThread;
        m_paintThread = 0;
    }
}

