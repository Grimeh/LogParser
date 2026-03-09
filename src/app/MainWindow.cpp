#include "MainWindow.h"
#include <QLabel>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    auto* central = new QWidget(this);
    auto* layout  = new QVBoxLayout(central);
    layout->addWidget(new QLabel("My Log Reader!", central));
    central->setLayout(layout);
    setCentralWidget(central);

    setWindowTitle("My Log Reader");
    statusBar()->showMessage("Ready");
}