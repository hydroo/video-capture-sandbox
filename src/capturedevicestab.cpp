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

#include "capturedevicestab.hpp"

#include <QPainter>
#include <QPaintEvent>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QtDebug>
#include <QVBoxLayout>

#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <thread>

#include <linux/videodev2.h>

using namespace std;

template <typename T>
static string anythingToString(T t);


CaptureDevicesTab::CaptureDevicesTab(QWidget *parent, set<CaptureDevice*> captureDevices)
        : QWidget(parent), m_paintThread(0), m_paintThreadCancellationFlag(false)
{
    /* *** init ui *** */
    m_mainLayout = new QHBoxLayout();
    m_globalButtonsLayout = new QVBoxLayout();
    m_globalButtonsLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_centralWidget = new QWidget(this);
    m_startStopAllDevicesButton= new QPushButton(tr("Start"), m_centralWidget);
    m_startStopAllDevicesButton->setCheckable(true);
    connect(m_startStopAllDevicesButton, SIGNAL(clicked(bool)), this, SLOT(startStopAllDevicesButtonClicked(bool)));
    m_updateAllDeviceControlsButton= new QPushButton(tr("Update Controls"), m_centralWidget);
    connect(m_updateAllDeviceControlsButton, SIGNAL(clicked(bool)), this,
            SLOT(updateAllDeviceControlsButtonClicked(bool)));


    /* *** init all capture devices *** */
    int loopCount = 0;
    for (auto it = captureDevices.begin(); it != captureDevices.end(); ++it, ++loopCount) {

        m_captureDevices.push_back(PerCaptureDevice());
        PerCaptureDevice &captureDevice = m_captureDevices.back();

        captureDevice.device = *it;
        captureDevice.groupBox = new QGroupBox(tr("Camera %1").arg(loopCount), m_centralWidget);
        captureDevice.layout = new QVBoxLayout();
        captureDevice.infoLabel = new QLabel(captureDevice.groupBox);
        captureDevice.infoLabelContents = map<string, string>();
        captureDevice.imageLabel = new QLabel(captureDevice.groupBox);
        captureDevice.currentImage = QImage();
        captureDevice.currentImageMutex = new mutex();

        captureDevice.layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        captureDevice.imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        captureDevice.layout->addWidget(captureDevice.infoLabel);
        captureDevice.layout->addWidget(captureDevice.imageLabel);
        captureDevice.groupBox->setLayout(captureDevice.layout);
        m_mainLayout->addWidget(captureDevice.groupBox);

        createCaptureDeviceControlWidgets(captureDevice.device, qobject_cast<QWidget*>(captureDevice.groupBox));
    }

    m_globalButtonsLayout->addWidget(m_startStopAllDevicesButton);
    m_globalButtonsLayout->addWidget(m_updateAllDeviceControlsButton);
    m_mainLayout->addLayout(m_globalButtonsLayout);
    this->setLayout(m_mainLayout);

    updateAllDeviceControlsButtonClicked(false);
}


CaptureDevicesTab::~CaptureDevicesTab()
{
    pausePaintThread(false);
    stopPaintThread();

    for (auto it = m_captureDevices.begin(); it != m_captureDevices.end(); ++it) {
        it->device->stopCapturing();
    }

    for (auto it = m_captureDevices.begin(); it != m_captureDevices.end(); ++it) {
        delete it->currentImageMutex;
    }
}


void CaptureDevicesTab::paintEvent(QPaintEvent *event)
{
    /* KLUDGE
     * pixmaps can only be used from the gui thread.
     *
     * Here we are using Qt's queue to get a decent spot when to update the label's images.
     * Normally one would try to get hold of the GUI thread in a different manner,
     * but for now this is ok
     */

    for (auto it = m_captureDevices.begin(); it != m_captureDevices.end(); ++it) {

        /* update image */
        it->currentImageMutex->lock();
        it->imageLabel->setPixmap(QPixmap::fromImage(it->currentImage));
        it->currentImageMutex->unlock();

        /* update info label text */
        ostringstream infoLabelText;
        for (auto itInfoLabelText = it->infoLabelContents.begin();
                itInfoLabelText != it->infoLabelContents.end(); ++itInfoLabelText) {
            infoLabelText << itInfoLabelText->first << " : " << itInfoLabelText->second << endl;
        }
        it->infoLabel->setText(infoLabelText.str().c_str());

    }

    QWidget::paintEvent(event);
}


void CaptureDevicesTab::startStopAllDevicesButtonClicked(bool checked)
{
    if (checked == true ) {

        for (auto it = m_captureDevices.begin(); it != m_captureDevices.end(); ++it) {
            if (it->device->isCapturing() == false) it->device->startCapturing();
            else it->device->pauseCapturing(false);
        }

        if (isPainting() == false) startPaintThread();
        else pausePaintThread(false);

    } else {

        assert(isPainting() == true);

        pausePaintThread(true);

        for (auto it = m_captureDevices.begin(); it != m_captureDevices.end(); ++it) {
            assert(it->device->isCapturing() == true);
            it->device->pauseCapturing(true);
        }

    }
}


void CaptureDevicesTab::updateAllDeviceControlsButtonClicked(bool checked)
{
    Q_UNUSED(checked);

    m_updateAllDeviceControlsButton->setEnabled(false);


    /* disable each capture device */
    list<bool> wasPaused;
    for (auto it = m_captureDevices.begin(); it != m_captureDevices.end(); ++it) {
        if (it->device->isCapturingPaused() == true) {
            wasPaused.push_back(true);
        } else {
            wasPaused.push_back(false);
            it->device->pauseCapturing(true);
        }
    }


    /* for each controls do update */
    for (auto itControls = m_senderWidgetToControl.begin(); itControls != m_senderWidgetToControl.end(); ++itControls) {

        /* leave out disabled controls */
        if (qobject_cast<QWidget*>(itControls->first)->isEnabled() == false) continue;

        struct v4l2_control currentValue;
        currentValue.id = itControls->second.id;
        if (itControls->second.device->control(currentValue) == true) {
            /* currentValue->value = xxx */
        } else {
            cerr << __PRETTY_FUNCTION__ << "error getting control value. Using default." << endl;
            currentValue.value = itControls->second.default_value;
        }
        
        switch (itControls->second.type) {
        case V4L2_CTRL_TYPE_INTEGER: {
            QSlider *widget = qobject_cast<QSlider*>(itControls->first);
            assert(widget != 0);
            widget->setValue(currentValue.value);
            break; }
        case V4L2_CTRL_TYPE_BOOLEAN: {
            QCheckBox *widget = qobject_cast<QCheckBox*>(itControls->first);
            assert(widget != 0);
            widget->setCheckState(currentValue.value == 0 ? Qt::Unchecked : Qt::Checked);
            break; }
        case V4L2_CTRL_TYPE_MENU: { /* never tested this - ronny 090820 */
            QComboBox *widget = qobject_cast<QComboBox*>(itControls->first);
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


    /* reenable each capture device */
    auto it2 = wasPaused.begin();
    for (auto it = m_captureDevices.begin(); it != m_captureDevices.end(); ++it, ++it2) {
        if ((*it2) == false) it->device->pauseCapturing(false);
        else it->device->pauseCapturing(true);
    }

    m_updateAllDeviceControlsButton->setEnabled(true);
}


void CaptureDevicesTab::sliderControlValueChanged(int value)
{
    if (m_senderWidgetToControl.find(sender()) == 
            m_senderWidgetToControl.end()) return;

    m_senderWidgetToControl[sender()].device->pauseCapturing(true);

    QWidget *senderWidget = qobject_cast<QWidget*>(sender());
    assert (senderWidget != 0);
    senderWidget->setEnabled(false);

    v4l2_control control;
    control.id = m_senderWidgetToControl[sender()].id;
    control.value = value;
    m_senderWidgetToControl[sender()].device->setControl(control);

    m_senderWidgetToControl[sender()].device->pauseCapturing(false);

    senderWidget->setEnabled(true);
}


void CaptureDevicesTab::checkBoxControlStateChanged(int state)
{
    if (m_senderWidgetToControl.find(sender()) == 
            m_senderWidgetToControl.end()) return;

    m_senderWidgetToControl[sender()].device->pauseCapturing(true);

    assert(state == Qt::Unchecked || state == Qt::Checked);

    QWidget *senderWidget = qobject_cast<QWidget*>(sender());
    assert (senderWidget != 0);
    senderWidget->setEnabled(false);

    v4l2_control control;
    control.id = m_senderWidgetToControl[sender()].id;
    control.value = state == Qt::Unchecked ? 0 : 1;
    m_senderWidgetToControl[sender()].device->setControl(control);

    m_senderWidgetToControl[sender()].device->pauseCapturing(false);

    senderWidget->setEnabled(true);
}


void CaptureDevicesTab::comboBoxControlIndexChanged (int index)
{
    if (m_senderWidgetToControl.find(sender()) == 
            m_senderWidgetToControl.end()) return;

    m_senderWidgetToControl[sender()].device->pauseCapturing(true);

    QWidget *senderWidget = qobject_cast<QWidget*>(sender());
    assert (senderWidget != 0);
    senderWidget->setEnabled(false);

    v4l2_control control;
    control.id = m_senderWidgetToControl[sender()].id;
    control.value = qobject_cast<QComboBox*>(sender())->itemData(index).toInt();

    m_senderWidgetToControl[sender()].device->setControl(control);

    m_senderWidgetToControl[sender()].device->pauseCapturing(false);

    senderWidget->setEnabled(true);
}


void CaptureDevicesTab::buttonControlClicked(bool checked)
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


void CaptureDevicesTab::startPaintThread()
{
    assert(isPainting() == false);
    m_paintThread = new std::thread(bind(paintThread, this));
}


void CaptureDevicesTab::stopPaintThread()
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


bool CaptureDevicesTab::isPainting() const
{
    return m_paintThread != 0;
}


void CaptureDevicesTab::pausePaintThread(bool pause)
{
    if (pause == true) {
        m_pausePaintingMutex.try_lock();
    } else {
        m_pausePaintingMutex.unlock();
    }
}


void CaptureDevicesTab::createCaptureDeviceControlWidgets(CaptureDevice *device, QWidget *widgetWhereToAddControlsTo)
{
    const pair<list<struct v4l2_queryctrl>, list<struct v4l2_querymenu> > cameraControls = device->controls();
    const list<struct v4l2_queryctrl> &controls = cameraControls.first;
    const list<struct v4l2_querymenu> &menuItems = cameraControls.second;


    for (auto it = controls.begin(); it != controls.end(); ++it) {

        QWidget *controlWidget;
        QWidget *controlLabel;
        string controlName = it->name != 0 ? string((const char*) it->name) : string("n/a");

        /* qDebug() << device << controlName.c_str() << "min" << it->minimum << "max" << it->maximum << "cur"
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
            controlLabel->setEnabled(!(it->flags & V4L2_CTRL_FLAG_DISABLED));
            controlLayout->addWidget(controlLabel);
        }

        if (controlWidget != 0) {
            m_senderWidgetToControl.insert(make_pair(qobject_cast<QObject*>(controlWidget),
                    ControlProperties({it->id, device, it->type, it->default_value})));
            controlWidget->setEnabled(!(it->flags & V4L2_CTRL_FLAG_DISABLED));
            controlLayout->addWidget(controlWidget);
        }

        widgetWhereToAddControlsTo->layout()->addItem(controlLayout);
    }
}


void CaptureDevicesTab::paintThread(CaptureDevicesTab *window)
{
    std::mutex &pausePaintingMutex = window->m_pausePaintingMutex;
    bool &m_paintThreadCancellationFlag = window->m_paintThreadCancellationFlag;

    /* init list of last image times */
    list<timespec> lastImageTimes;
    for (auto it = window->m_captureDevices.begin(); it != window->m_captureDevices.end(); ++it) {
        lastImageTimes.push_back(timespec());
        lastImageTimes.back() = {numeric_limits<time_t>::min(), 0};
    }


    while (m_paintThreadCancellationFlag == false) {

        bool updateGUI = false;

        /* pausing mechanism */
        pausePaintingMutex.lock();
        pausePaintingMutex.unlock();

        auto itImageTimes = lastImageTimes.begin();
        for (auto it = window->m_captureDevices.begin();
                it != window->m_captureDevices.end()
                ; ++it, ++itImageTimes) {
        
            if (it->device->newerBuffersAvailable(*itImageTimes) > 0) {

                updateGUI = true;

                deque<const CaptureDevice::Buffer*> buffers;
                const CaptureDevice::Buffer *buffer = buffers[0];

                buffers = it->device->lockFirstNBuffers(1);
                buffer = buffers[0];

                *itImageTimes = buffer->time;

                it->infoLabelContents["time"] = anythingToString(
                        (itImageTimes->tv_sec + itImageTimes->tv_nsec / 1000000000.0));

                it->currentImageMutex->lock();
                it->currentImage = QImage(buffer->buffer, it->device->captureSize().first,
                        it->device->captureSize().second, QImage::Format_RGB888);
                it->currentImageMutex->unlock();
                
                it->device->unlock(buffers);
            }

            /*
             does not work with N capture devices TODO

             if (window->m_camera1InfoLabelContents.find("time") !=window->m_camera1InfoLabelContents.end() &&
                    window->m_camera2InfoLabelContents.find("time") !=window->m_camera2InfoLabelContents.end()) {
                double t1 = atof(window->m_camera1InfoLabelContents["time"].c_str());
                double t2 = atof(window->m_camera2InfoLabelContents["time"].c_str());

                window->m_camera1InfoLabelContents["deviation"] = window->m_camera2InfoLabelContents["deviation"]
                        = anythingToString(fabs(t1-t2));
            }*/
        }


        if (updateGUI == true) {

            window->update();

        } else {

            struct timespec sleepLength = { 0, 1000 };
            clock_nanosleep(CLOCK_MONOTONIC, 0, &sleepLength, 0);

        } 
    }
}


/* *** local *************************************************************** */
template <typename T>
string anythingToString(T t)
{
    ostringstream os;
    os << t;
    return os.str();
}

