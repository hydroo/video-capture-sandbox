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

#include "mainwindow.hpp"

#include "capturedevicestab.hpp"
#include "filtereditortab.hpp"
#include "viewstab.hpp"

#include <QTabWidget>

using namespace std;


MainWindow::MainWindow(QWidget *parent, const set<CaptureDevice*> &captureDevices) :
        QMainWindow(parent)
{
    m_centralWidget = new QTabWidget(this);

    m_centralWidget->addTab(new CaptureDevicesTab(m_centralWidget, captureDevices), tr("Capture Devices"));
    m_centralWidget->addTab(new FilterEditorTab(m_centralWidget), tr("Filter Editor"));
    m_centralWidget->addTab(new ViewsTab(m_centralWidget), tr("Views"));

    setCentralWidget(m_centralWidget);
}


MainWindow::~MainWindow()
{
}

