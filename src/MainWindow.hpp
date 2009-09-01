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

#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP


#include "Prereqs.hpp"

#include <list>
#include <mutex>
#include <QMap>
#include <QMainWindow>
#include <QImage>
#include "CaptureDevice.hpp"

class QGroupBox;
class QHBoxLayout;
class QLabel;
class QPaintEvent;
class QPushButton;
class QVBoxLayout;
class CaptureDevice;


namespace std { class thread; };


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent, std::list<CaptureDevice*> captureDevices);
    ~MainWindow();

    virtual QSize sizeHint() const;

protected:

    virtual void closeEvent(QCloseEvent *event);
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

    void createCaptureDeviceControlWidgets(CaptureDevice* device, QWidget *widgetWhereToAddControlsTo);

    static void paintThread(MainWindow* window);


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


#endif /* MAIN_WINDOW_HPP */

