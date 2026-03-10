#include "MainWindow.h"
#include "TabsWidget.h"
#include "PlusTabBar.h"
#include "ui/LogReaderTab.h"

#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("My Log Reader");

    m_tabs = new TabsWidget(this);

    
    connect(m_tabs, &TabsWidget::plusClicked, this, &MainWindow::createNewTab);
    connect(m_tabs, &TabsWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabs, &TabsWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);

    setCentralWidget(m_tabs);

    // initial tab
    createNewTab();
}

void MainWindow::createNewTab() {
    auto* tab = new LogReaderTab(m_tabs);
    int idx = m_tabs->addTab(tab, "Untitled");
    m_tabs->setCurrentIndex(idx);

    // Update tab title when path changes
    connect(tab, &LogReaderTab::pathChanged, this, &MainWindow::updateTabTitleFor);
}

void MainWindow::closeTab(int index) {
    QWidget* w = m_tabs->widget(index);
    m_tabs->removeTab(index);
    delete w;
}

void MainWindow::onCurrentTabChanged(int index) {
    m_lastActive = index;
}

void MainWindow::updateTabTitleFor(const QString& base) {
    int idx = m_tabs->currentIndex();
    if (idx >= 0)
        m_tabs->setTabText(idx, base.isEmpty() ? "Untitled" : base);
}