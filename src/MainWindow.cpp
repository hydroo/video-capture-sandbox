#include "MainWindow.hpp"

#include <iostream>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

using std::cerr;
using std::endl;


MainWindow::MainWindow(QWidget *parent, Camera& camera) :
        QMainWindow(parent),
        m_camera(camera)
{
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
}


MainWindow::~MainWindow()
{
}


QSize MainWindow::sizeHint() const
{
    return QSize(800,600);
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    (void) event;
}


void MainWindow::startStopButtonClicked(bool checked)
{
    (void) checked;
    // TODO
}

