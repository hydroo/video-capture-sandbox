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
    void sliderControlValueChanged(int value);
    void checkBoxControlStateChanged(int state);
    void comboBoxControlIndexChanged (int index);
    void buttonControlClicked(bool checked);

private:

    void startPaintThread();
    void stopPaintThread();

    void createCaptureDeviceControlWidgets(CaptureDevice& camera, QWidget *widgetWhereToAddControlsTo);

    static void paintThread(MainWindow* window);

    QHBoxLayout *m_mainLayout;
    QVBoxLayout *m_globalButtonsLayout;
    QWidget *m_centralWidget;
    QPushButton *m_startStopButton;
    QGroupBox *m_camera1GroupBox;
        QVBoxLayout *m_camera1Layout;
        QLabel *m_camera1ImageLabel;
        QPushButton *m_camera1StartStopButton;
    QGroupBox *m_camera2GroupBox;
        QVBoxLayout *m_camera2Layout;
        QLabel *m_camera2ImageLabel;
        QPushButton *m_camera2StartStopButton;

    QMap<QObject*, QPair<__u32, CaptureDevice*> > m_senderWidgetToControl;

    CaptureDevice& m_camera1;
    CaptureDevice& m_camera2;

    std::thread *m_paintThread;

    bool m_paintThreadCancellationFlag;

    QImage m_currentCamera1Image;
    QImage m_currentCamera2Image;

    std::mutex m_currentCamera1ImageMutex;
    std::mutex m_currentCamera2ImageMutex;

};


#endif /* MAIN_WINDOW_HPP */

