#include "MainWindow.hpp"

#include <cassert>
#include <functional>
#include <iostream>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QtDebug>
#include <QVBoxLayout>
#include <thread>
#include "CaptureDevice.hpp"

using namespace std;


MainWindow::MainWindow(QWidget *parent, CaptureDevice& camera1, CaptureDevice& camera2) :
        QMainWindow(parent),
        m_camera1(camera1),
        m_camera2(camera2),
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
    m_camera1.stopCapturing();
    m_camera2.stopCapturing();
}


void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter p(this);

    m_currentCamera1ImageMutex.lock();
    p.drawImage(event->rect().topLeft(), m_currentCamera1Image, event->rect());
    m_currentCamera1ImageMutex.unlock();

    QPoint camera2TopLeft = QPoint(m_currentCamera1Image.width(), 0);
    QRect camera2Rect = QRect(camera2TopLeft, m_currentCamera2Image.size());

    if (event->rect().intersects(camera2Rect)) {
        m_currentCamera2ImageMutex.lock();
        p.drawImage(camera2TopLeft,
                m_currentCamera2Image,
                QRect(QPoint(0,0), m_currentCamera2Image.size())
                .intersected(event->rect().translated(m_currentCamera1Image.size().width()*-1,
                m_currentCamera1Image.size().height()*-1)));
        m_currentCamera2ImageMutex.unlock();
    }


    p.end();

    QWidget::paintEvent(event);
}


void MainWindow::startStopButtonClicked(bool checked)
{
    if (checked) {
        m_camera1.startCapturing();
        m_camera2.startCapturing();
        startPaintThread();
    } else {
        stopPaintThread();
        m_camera1.stopCapturing();
        m_camera2.stopCapturing();
    }
}


void MainWindow::paintThread(MainWindow *window)
{
    CaptureDevice &m_camera1 = window->m_camera1;
    CaptureDevice &m_camera2 = window->m_camera2;
    QImage& m_currentCamera1Image = window->m_currentCamera1Image;
    QImage& m_currentCamera2Image = window->m_currentCamera2Image;
    std::mutex& m_currentCamera1ImageMutex = window->m_currentCamera1ImageMutex;
    std::mutex& m_currentCamera2ImageMutex = window->m_currentCamera2ImageMutex;
    bool &m_paintThreadCancellationFlag = window->m_paintThreadCancellationFlag;

    struct timespec lastPictureCamera1 = {numeric_limits<time_t>::min(), 0};
    struct timespec lastPictureCamera2 = {numeric_limits<time_t>::min(), 0};

    while (m_paintThreadCancellationFlag == false) {

        bool capture1 = m_camera1.newerBuffersAvailable(lastPictureCamera1) > 0;
        bool capture2 = m_camera2.newerBuffersAvailable(lastPictureCamera2) > 0;

        deque<const CaptureDevice::Buffer*> buffers1;
        deque<const CaptureDevice::Buffer*> buffers2;

        const CaptureDevice::Buffer* buffer1 = buffers1[0];
        const CaptureDevice::Buffer* buffer2 = buffers2[0];
        
        /* minimized time between two buffer locks */
        if (capture1) {
            buffers1 = m_camera1.lockFirstNBuffers(1);
        }
        if (capture2) {
            buffers2 = m_camera2.lockFirstNBuffers(1);
        }

        if (capture1) {
            buffer1 = buffers1[0];

            lastPictureCamera1 = buffer1->time;

            m_currentCamera1ImageMutex.lock();
            m_currentCamera1Image = QImage(buffer1->buffer, m_camera1.captureSize().first,
                    m_camera1.captureSize().second, QImage::Format_RGB888);
            m_currentCamera1ImageMutex.unlock();
            
            m_camera1.unlock(buffers1);
        }

        /* KLUDGE code duplication at its best */
        if (capture2) {
            buffer2 = buffers2[0];

            lastPictureCamera2 = buffer2->time;

            m_currentCamera2ImageMutex.lock();
            m_currentCamera2Image = QImage(buffer2->buffer, m_camera2.captureSize().first,
                    m_camera2.captureSize().second, QImage::Format_RGB888);
            m_currentCamera2ImageMutex.unlock();
            
            m_camera2.unlock(buffers2);
        }


        if (capture1 || capture2) {

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

