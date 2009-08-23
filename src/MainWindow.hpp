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

