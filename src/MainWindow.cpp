#include "MainWindow.hpp"

#include <cassert>
#include <functional>
#include <iostream>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QSlider>
#include <QtDebug>
#include <QVBoxLayout>
#include <sstream>
#include <thread>

using namespace std;


template <typename T> static string anythingToString(T t)
{
    ostringstream os;
    os << t;
    return os.str();
}


MainWindow::MainWindow(QWidget *parent, CaptureDevice& camera1, CaptureDevice& camera2) :
        QMainWindow(parent),
        m_camera1(camera1),
        m_camera2(camera2),
        m_paintThread(0),
        m_paintThreadCancellationFlag(false)
{
    /* *** init ui *** */
    m_mainLayout = new QHBoxLayout();
    m_globalButtonsLayout = new QVBoxLayout();
    m_globalButtonsLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_centralWidget = new QWidget(this);
    m_startStopButton = new QPushButton(tr("Start"), m_centralWidget);
    m_startStopButton->setCheckable(true);
    connect(m_startStopButton, SIGNAL(clicked(bool)), this, SLOT(startStopButtonClicked(bool)));
    m_updateControlValuesButton = new QPushButton(tr("Update Control Values"), m_centralWidget);
    connect(m_updateControlValuesButton, SIGNAL(clicked(bool)), this, SLOT(updateControlValuesButtonClicked(bool)));
    m_camera1GroupBox = new QGroupBox(tr("Camera 1"), m_centralWidget);
        m_camera1Layout = new QVBoxLayout();
        m_camera1Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        m_camera1InfoLabel = new QLabel(m_camera1GroupBox);
        m_camera1Layout ->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        m_camera1ImageLabel = new QLabel(m_camera1GroupBox);
        m_camera1ImageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_camera2GroupBox = new QGroupBox(tr("Camera 2"), m_centralWidget);
        m_camera2Layout = new QVBoxLayout();
        m_camera2Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        m_camera2InfoLabel = new QLabel(m_camera1GroupBox);
        m_camera2Layout ->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        m_camera2ImageLabel = new QLabel(m_camera2GroupBox);
        m_camera2ImageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_globalButtonsLayout->addWidget(m_startStopButton);
    m_globalButtonsLayout->addWidget(m_updateControlValuesButton);
    m_camera1Layout->addWidget(m_camera1InfoLabel);
    m_camera2Layout->addWidget(m_camera2InfoLabel);
    m_camera1Layout->addWidget(m_camera1ImageLabel);
    m_camera2Layout->addWidget(m_camera2ImageLabel);
    m_camera1GroupBox->setLayout(m_camera1Layout);
    m_camera2GroupBox->setLayout(m_camera2Layout);
    m_mainLayout->addWidget(m_camera1GroupBox);
    m_mainLayout->addWidget(m_camera2GroupBox);
    m_mainLayout->addLayout(m_globalButtonsLayout);
    m_centralWidget->setLayout(m_mainLayout);
    setCentralWidget(m_centralWidget);

    createCaptureDeviceControlWidgets(m_camera1, qobject_cast<QWidget*>(m_camera1GroupBox));
    createCaptureDeviceControlWidgets(m_camera2, qobject_cast<QWidget*>(m_camera2GroupBox));

    updateControlValuesButtonClicked(false);
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
    /* KLUDGE
     * pixmaps can only be used from the gui thread.
     *
     * Here we are using Qt's queue to get a decent spot when to update the label's images.
     * Normally one would try to get hold of the GUI thread in a different manner,
     * but for now this is ok
     */

    m_camera1ImageLabel->setPixmap(QPixmap::fromImage(m_currentCamera1Image));
    m_camera2ImageLabel->setPixmap(QPixmap::fromImage(m_currentCamera2Image));


    /* update info label text */
    ostringstream camera1LabelText;
    for (auto it = m_camera1InfoLabelContents.begin(); it != m_camera1InfoLabelContents.end(); ++it) {
        camera1LabelText<< it->first << " : " << it->second << endl;
    }
    m_camera1InfoLabel->setText(camera1LabelText.str().c_str());

    ostringstream camera2LabelText;
    for (auto it = m_camera2InfoLabelContents.begin(); it != m_camera2InfoLabelContents.end(); ++it) {
        camera2LabelText << it->first << " : " << it->second << endl;
    }
    m_camera2InfoLabel->setText(camera2LabelText.str().c_str());


    QWidget::paintEvent(event);
}


void MainWindow::startStopButtonClicked(bool checked)
{
    if (checked == true ) {
        if (m_camera1.isCapturing() == false) m_camera1.startCapturing();
        else m_camera1.pauseCapturing(false);
        if (m_camera2.isCapturing() == false) m_camera2.startCapturing();
        else m_camera2.pauseCapturing(false);
        if (isPainting() == false) startPaintThread();
        else pausePaintThread(false);
    } else {
        assert(m_camera1.isCapturing() == true);
        assert(m_camera2.isCapturing() == true);
        assert(isPainting() == true);

        pausePaintThread(true);
        m_camera1.pauseCapturing(true);
        m_camera2.pauseCapturing(true);
    }
}


void MainWindow::updateControlValuesButtonClicked(bool checked)
{
    Q_UNUSED(checked);

    bool isCamera1Capturing = m_camera1.isCapturing();
    bool isCamera2Capturing = m_camera2.isCapturing();


    if (isCamera1Capturing) m_camera1.pauseCapturing(true);
    if (isCamera2Capturing) m_camera2.pauseCapturing(true);

    m_updateControlValuesButton->setEnabled(false);

    for (auto it = m_senderWidgetToControl.begin(); it != m_senderWidgetToControl.end(); ++it) {

        struct v4l2_control currentValue;
        currentValue.id = it->second.id;
        if (it->second.camera->control(currentValue) == true) {
            /* currentValue->value = xxx */
        } else {
            cerr << __PRETTY_FUNCTION__ << "error getting control value. Using default." << endl;
            currentValue.value = it->second.default_value;
        }
        
        switch (it->second.type) {
        case V4L2_CTRL_TYPE_INTEGER: {
            QSlider *widget = qobject_cast<QSlider*>(it->first);
            assert(widget != 0);
            widget->setValue(currentValue.value);
            break; }
        case V4L2_CTRL_TYPE_BOOLEAN: {
            QCheckBox *widget = qobject_cast<QCheckBox*>(it->first);
            assert(widget != 0);
            widget->setCheckState(currentValue.value == 0 ? Qt::Unchecked : Qt::Checked);
            break; }
        case V4L2_CTRL_TYPE_MENU: { /* never tested this - ronny 090820 */
            QComboBox *widget = qobject_cast<QComboBox*>(it->first);
            assert(widget != 0);
            /* select the current item */
            int a;
            for (a = 0; a < widget->count(); ++a) {
                if (widget->itemData(a).toInt() == currentValue.value) {
                    widget->setCurrentIndex(a);
                    break;
                }
            }
            assert(a < widget->count());
            break; }
        case V4L2_CTRL_TYPE_BUTTON:
            /* a button has no state -> do nothing*/
            break;
        case V4L2_CTRL_TYPE_INTEGER64:
            break;
        case V4L2_CTRL_TYPE_CTRL_CLASS:
            break;
        default:
            assert(0);
            break;
        }
    }

    if (isCamera1Capturing) m_camera1.pauseCapturing(false);
    if (isCamera2Capturing) m_camera2.pauseCapturing(false);

    m_updateControlValuesButton->setEnabled(true);
}


void MainWindow::sliderControlValueChanged(int value)
{
    if (m_senderWidgetToControl.find(sender()) == 
            m_senderWidgetToControl.end()) return;

    m_senderWidgetToControl[sender()].camera->pauseCapturing(true);

    QWidget *senderWidget = qobject_cast<QWidget*>(sender());
    assert (senderWidget != 0);
    senderWidget->setEnabled(false);

    v4l2_control control;
    control.id = m_senderWidgetToControl[sender()].id;
    control.value = value;
    m_senderWidgetToControl[sender()].camera->setControl(control);

    m_senderWidgetToControl[sender()].camera->pauseCapturing(false);

    senderWidget->setEnabled(true);
}


void MainWindow::checkBoxControlStateChanged(int state)
{
    if (m_senderWidgetToControl.find(sender()) == 
            m_senderWidgetToControl.end()) return;

    m_senderWidgetToControl[sender()].camera->pauseCapturing(true);

    assert(state == Qt::Unchecked || state == Qt::Checked);

    QWidget *senderWidget = qobject_cast<QWidget*>(sender());
    assert (senderWidget != 0);
    senderWidget->setEnabled(false);

    v4l2_control control;
    control.id = m_senderWidgetToControl[sender()].id;
    control.value = state == Qt::Unchecked ? 0 : 1;
    m_senderWidgetToControl[sender()].camera->setControl(control);

    m_senderWidgetToControl[sender()].camera->pauseCapturing(false);

    senderWidget->setEnabled(true);
}


void MainWindow::comboBoxControlIndexChanged (int index)
{
    if (m_senderWidgetToControl.find(sender()) == 
            m_senderWidgetToControl.end()) return;

    m_senderWidgetToControl[sender()].camera->pauseCapturing(true);

    QWidget *senderWidget = qobject_cast<QWidget*>(sender());
    assert (senderWidget != 0);
    senderWidget->setEnabled(false);

    v4l2_control control;
    control.id = m_senderWidgetToControl[sender()].id;
    control.value = qobject_cast<QComboBox*>(sender())->itemData(index).toInt();

    m_senderWidgetToControl[sender()].camera->setControl(control);

    m_senderWidgetToControl[sender()].camera->pauseCapturing(false);

    senderWidget->setEnabled(true);
}


void MainWindow::buttonControlClicked(bool checked)
{
    Q_UNUSED(checked);
    if (m_senderWidgetToControl.find(sender()) == 
            m_senderWidgetToControl.end()) return;

    QWidget *senderWidget = qobject_cast<QWidget*>(sender());
    assert (senderWidget != 0);
    senderWidget->setEnabled(false);
    // TODO
    senderWidget->setEnabled(true);
}


void MainWindow::startPaintThread()
{
    assert(isPainting() == false);
    m_paintThread = new std::thread(bind(paintThread, this));
}


void MainWindow::stopPaintThread()
{
    if (isPainting() == true) {
        assert(m_paintThread->joinable() == true);

        m_paintThreadCancellationFlag = true;
        m_paintThread->join();
        m_paintThreadCancellationFlag = false;

        delete m_paintThread;
        m_paintThread = 0;
    }
}


bool MainWindow::isPainting() const
{
    return m_paintThread != 0;
}


void MainWindow::pausePaintThread(bool pause)
{
    if (pause == true) {
        m_pausePaintingMutex.try_lock();
    } else {
        m_pausePaintingMutex.unlock();
    }
}


void MainWindow::createCaptureDeviceControlWidgets(CaptureDevice& camera, QWidget *widgetWhereToAddControlsTo)
{
    const pair<list<struct v4l2_queryctrl>, list<struct v4l2_querymenu> > cameraControls = camera.controls();
    const list<struct v4l2_queryctrl>& controls = cameraControls.first;
    const list<struct v4l2_querymenu>& menuItems = cameraControls.second;


    for (auto it = controls.begin(); it != controls.end(); ++it) {

        QWidget *controlWidget;
        QWidget *controlLabel;
        string controlName = it->name != 0 ? string((const char*) it->name) : string("n/a");

        /* qDebug() << &camera << controlName.c_str() << "min" << it->minimum << "max" << it->maximum << "cur"
                << currentValue.value << "default" << it->default_value; */

        switch (it->type) {
        case V4L2_CTRL_TYPE_INTEGER: {
            controlLabel = new QLabel(controlName.c_str(), widgetWhereToAddControlsTo);
            QSlider *widget = new QSlider(Qt::Horizontal, widgetWhereToAddControlsTo);
            connect(widget, SIGNAL(valueChanged(int)), this, SLOT(sliderControlValueChanged(int)));
            widget->setTracking(false);
            widget->setSingleStep(1);
            widget->setMinimum(it->minimum);
            widget->setMaximum(it->maximum);

            controlWidget = qobject_cast<QWidget*>(widget);
            break; }
        case V4L2_CTRL_TYPE_BOOLEAN: {
            controlLabel = 0;
            QCheckBox *widget = new QCheckBox(controlName.c_str(), widgetWhereToAddControlsTo);
            connect(widget, SIGNAL(stateChanged(int)), this, SLOT(checkBoxControlStateChanged(int)));

            controlWidget = qobject_cast<QWidget*>(widget);
            break; }
        case V4L2_CTRL_TYPE_MENU: { /* never tested this - ronny 090820 */
            controlLabel = new QLabel(controlName.c_str(), widgetWhereToAddControlsTo);
            QComboBox *widget = new QComboBox(widgetWhereToAddControlsTo);
            connect(widget, SIGNAL(currentIndexChanged(int)), this, SLOT(comboBoxControlIndexChanged(int)));

            /* fill the combo box */
            for (auto itMenu = menuItems.begin(); itMenu != menuItems.end(); ++itMenu) {
                if (itMenu->id == it->id) {
                    string menuItemName = itMenu->name != 0 ? string((const char*) itMenu->name) : string("n/a");
                    /* userData contains the index to be used for ioctl calls */
                    widget->addItem(menuItemName.c_str(), QVariant(itMenu->index));
                }
            }

            controlWidget = qobject_cast<QWidget*>(widget);
            break; }
        case V4L2_CTRL_TYPE_BUTTON: { /* never tested this - ronny 090820 */
            controlLabel = 0;
            QPushButton *widget = new QPushButton(controlName.c_str(), widgetWhereToAddControlsTo);
            connect(widget, SIGNAL(clicked(bool)), this, SLOT(buttonControlClicked(bool)));
            controlWidget = qobject_cast<QWidget*>(widget);
            break; }
        case V4L2_CTRL_TYPE_INTEGER64:
            controlLabel = new QLabel(controlName.c_str(), widgetWhereToAddControlsTo);
            controlWidget = qobject_cast<QWidget*>(new QLabel("V4L2_CTRL_TYPE_INTEGER64", widgetWhereToAddControlsTo));
            break;
        case V4L2_CTRL_TYPE_CTRL_CLASS:
            controlLabel = new QLabel(controlName.c_str(), widgetWhereToAddControlsTo);
            controlWidget = qobject_cast<QWidget*>(new QLabel("V4L2_CTRL_TYPE_CTRL_CLASS", widgetWhereToAddControlsTo));
            break;
        default:
            assert(0);
            break;
        }

        QHBoxLayout *controlLayout = new QHBoxLayout();

        if (controlLabel != 0) {
            controlLayout->addWidget(controlLabel);
        }

        if (controlWidget != 0) {
            m_senderWidgetToControl.insert(make_pair(qobject_cast<QObject*>(controlWidget),
                    ControlProperties({it->id, &camera, it->type, it->default_value})));
            controlLayout->addWidget(controlWidget);
        }

        widgetWhereToAddControlsTo->layout()->addItem(controlLayout);
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
    std::mutex& pausePaintingMutex = window->m_pausePaintingMutex;
    bool &m_paintThreadCancellationFlag = window->m_paintThreadCancellationFlag;

    struct timespec lastPictureCamera1 = {numeric_limits<time_t>::min(), 0};
    struct timespec lastPictureCamera2 = {numeric_limits<time_t>::min(), 0};

    while (m_paintThreadCancellationFlag == false) {

        pausePaintingMutex.lock();
        pausePaintingMutex.unlock();

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

            window->m_camera1InfoLabelContents["s"] = anythingToString(
                    (lastPictureCamera1.tv_sec + lastPictureCamera1.tv_nsec / 1000000000.0));

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

            window->m_camera2InfoLabelContents["s"] = anythingToString(
                    (lastPictureCamera2.tv_sec + lastPictureCamera2.tv_nsec / 1000000000.0));

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

