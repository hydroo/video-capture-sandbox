#include "MainWindow.hpp"

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
    m_paintThread->join();
    m_paintThreadCancellationFlag = false;
    delete m_paintThread;
    m_paintThread = 0;

    m_camera.finish();
}


void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter p(this);

    p.drawImage(event->rect().topLeft(), m_currentCaptureImage, event->rect());

    p.end();

    QWidget::paintEvent(event);
}


void MainWindow::startStopButtonClicked(bool checked)
{
    (void) checked;
    // TODO
}


void MainWindow::paintThread(MainWindow *window)
{
    CaptureDevice *camera = &(window->m_camera);
    QImage &image = window->m_currentCaptureImage;

    static char a = 'a' - 1;
    if (a == 'z'+1) exit(0);
    ++a;


    struct timespec lastpicture = {numeric_limits<time_t>::min(), 0};

    while (window->m_paintThreadCancellationFlag == false) {
        // cerr << camera->newerBuffersAvailable(lastpicture) << endl;
        
        if (camera->newerBuffersAvailable(lastpicture) > 0) {

            deque<const CaptureDevice::Buffer*> buffers = camera->lockFirstNBuffers(1);
            const CaptureDevice::Buffer* buffer = buffers[0];

            lastpicture = buffer->time;

            /* do something with the picture */
#if 0
            char filename[128] = ""; 
            sprintf(filename,"%c.jpeg", a);

            FILE *fp = fopen(filename, "wb");
            if (!fp) exit(1);
            fwrite(buffer, camera->bufferSize(), sizeof(unsigned char), fp);
            fclose(fp);
#endif
            // cerr << "read " << a << endl;
            
            /*cerr << camera->captureSize().first << "x" << camera->captureSize().second << " "
                    << camera->bufferSize() << " " << 352*288*3 << endl;*/
            
            image = QImage(buffer->buffer, camera->captureSize().first, camera->captureSize().second,
                    QImage::Format_RGB888);

            window->update();

            camera->unlock(buffers);

        } else {

            struct timespec sleepLength = { 0, 1000 };
            clock_nanosleep(CLOCK_REALTIME, 0, &sleepLength, 0);

        }
    }
}

