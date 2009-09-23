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

#ifndef CAPTURE_DEVICES_TAB_HPP
#define CAPTURE_DEVICES_TAB_HPP

#include "prereqs.hpp"
#include <list>
#include <mutex>
#include <QImage>
#include <QMap>
#include <QWidget>
#include "capturedevice.hpp"

class QGroupBox;
class QHBoxLayout;
class QLabel;
class QPaintEvent;
class QPushButton;
class QVBoxLayout;

namespace std { class thread; };


class CaptureDevicesTab : public QWidget
{
    Q_OBJECT
public:
    CaptureDevicesTab(QWidget *parent, std::list<CaptureDevice*> captureDevices);
    ~CaptureDevicesTab();

protected:

    virtual void paintEvent(QPaintEvent *event);

private slots:

    void startStopAllDevicesButtonClicked(bool checked);
    void updateAllDeviceControlsButtonClicked(bool checked);

    void sliderControlValueChanged(int value);
    void checkBoxControlStateChanged(int state);
    void comboBoxControlIndexChanged (int index);
    void buttonControlClicked(bool checked);

private:
    
    void startPaintThread();
    void stopPaintThread();
    bool isPainting() const;
    void pausePaintThread(bool pause);

    void createCaptureDeviceControlWidgets(CaptureDevice *device, QWidget *widgetWhereToAddControlsTo);

    static void paintThread(CaptureDevicesTab *window);

    QHBoxLayout *m_mainLayout;
    QVBoxLayout *m_globalButtonsLayout;
    QWidget *m_centralWidget;
    QPushButton *m_updateAllDeviceControlsButton;
    QPushButton *m_startStopAllDevicesButton;


    struct PerCaptureDevice
    {
        CaptureDevice* device;

        QGroupBox *groupBox;
        QVBoxLayout *layout;
        QLabel *infoLabel;
        std::map<std::string,std::string> infoLabelContents;
        QLabel *imageLabel;

        QImage currentImage;
        std::mutex *currentImageMutex;
    };

    std::list<PerCaptureDevice> m_captureDevices;


    struct ControlProperties
    {
        __u32 id;
        CaptureDevice* device;
        enum v4l2_ctrl_type type;
        __s32 default_value;
    };

    /** mapping from widget to respective v4l control of the camera */
    std::map<QObject*, ControlProperties> m_senderWidgetToControl;


    std::thread *m_paintThread;
    bool m_paintThreadCancellationFlag;
    std::mutex m_pausePaintingMutex;

};


#endif /* CAPTURE_DEVICES_TAB_HPP */

