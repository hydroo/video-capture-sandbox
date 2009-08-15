#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP


#include "Prereqs.hpp"

#include <mutex>
#include <QMainWindow>
#include <QImage>

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

protected slots:

    void startStopButtonClicked(bool checked);

private:

    static void paintThread(MainWindow* window);

    QHBoxLayout *m_mainLayout;
    QVBoxLayout *m_buttonLayout;

    QWidget *m_centralWidget;
    QLabel *m_rawImageLabel;
    QPushButton *m_startStopButton;

    CaptureDevice& m_camera1;
    CaptureDevice& m_camera2;

    std::thread *m_paintThread;

    bool m_paintThreadCancellationFlag;

private:

    void startPaintThread();
    void stopPaintThread();

    QImage m_currentCamera1Image;
    QImage m_currentCamera2Image;
    std::mutex m_currentCamera1ImageMutex;
    std::mutex m_currentCamera2ImageMutex;


};


#endif /* MAIN_WINDOW_HPP */

