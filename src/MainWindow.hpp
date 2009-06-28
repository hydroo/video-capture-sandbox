#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP


#include "Prereqs.hpp"

#include <QMainWindow>

class QHBoxLayout;
class QLabel;
class QPushButton;
class QVBoxLayout;
class Camera;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent, Camera& camera);
    ~MainWindow();

    virtual QSize sizeHint() const;

protected:

    virtual void closeEvent(QCloseEvent *event);

protected slots:

    void startStopButtonClicked(bool checked);

private:

    QHBoxLayout *m_mainLayout;
    QVBoxLayout *m_buttonLayout;

    QWidget *m_centralWidget;
    QLabel *m_rawImageLabel;
    QPushButton *m_startStopButton;

    Camera& m_camera;

private:


};


#endif /* MAIN_WINDOW_HPP */

