/* videocapture is tool with no special purpose
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

    MainWindow(QWidget *parent, CaptureDevice& camera1, CaptureDevice& camera2);
    ~MainWindow();

    virtual QSize sizeHint() const;

protected:

    virtual void closeEvent(QCloseEvent *event);
    virtual void paintEvent(QPaintEvent *event);

private slots:

    void startStopButtonClicked(bool checked);
    void updateControlValuesButtonClicked(bool checked);
    void sliderControlValueChanged(int value);
    void checkBoxControlStateChanged(int state);
    void comboBoxControlIndexChanged (int index);
    void buttonControlClicked(bool checked);

private:

    void startPaintThread();
    void stopPaintThread();
    bool isPainting() const;
    void pausePaintThread(bool pause);

    void createCaptureDeviceControlWidgets(CaptureDevice& camera, QWidget *widgetWhereToAddControlsTo);

    static void paintThread(MainWindow* window);

    QHBoxLayout *m_mainLayout;
    QVBoxLayout *m_globalButtonsLayout;
    QWidget *m_centralWidget;
    QPushButton *m_startStopButton;
    QPushButton *m_updateControlValuesButton;
    QGroupBox *m_camera1GroupBox;
        QVBoxLayout *m_camera1Layout;
        QLabel *m_camera1InfoLabel;
        std::map<std::string,std::string> m_camera1InfoLabelContents;
        QLabel *m_camera1ImageLabel;
        QPushButton *m_camera1StartStopButton;
    QGroupBox *m_camera2GroupBox;
        QLabel *m_camera2InfoLabel;
        std::map<std::string,std::string> m_camera2InfoLabelContents;
        QVBoxLayout *m_camera2Layout;
        QLabel *m_camera2ImageLabel;
        QPushButton *m_camera2StartStopButton;

    struct ControlProperties
    {
        __u32 id;
        CaptureDevice* camera;
        enum v4l2_ctrl_type type;
        __s32 default_value;
    };


    std::map<QObject*, ControlProperties> m_senderWidgetToControl;

    CaptureDevice& m_camera1;
    CaptureDevice& m_camera2;

    std::thread *m_paintThread;

    bool m_paintThreadCancellationFlag;

    QImage m_currentCamera1Image;
    QImage m_currentCamera2Image;

    std::mutex m_currentCamera1ImageMutex;
    std::mutex m_currentCamera2ImageMutex;
    std::mutex m_pausePaintingMutex;
};


#endif /* MAIN_WINDOW_HPP */

