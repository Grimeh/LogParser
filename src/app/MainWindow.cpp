#include "MainWindow.h"
#include "TabsWidget.h"
#include "PlusTabBar.h"
#include "ui/LogReaderTab.h"

#include <QStatusBar>
#include <QTabWidget>
#include <QShortcut>
#include <QSettings>
#include <iostream>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("LogParser");

    m_tabs = new TabsWidget(this);
    setCentralWidget(m_tabs);
    
    connect(m_tabs, &TabsWidget::plusClicked, this, &MainWindow::createNewTab);
    connect(m_tabs, &TabsWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabs, &TabsWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);

    auto* scNew = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this);
    connect(scNew, &QShortcut::activated, this, &MainWindow::createNewTab);

    restoreUiState();

    // initial tab
    createNewTab();
}

void MainWindow::openLogList(int listSize, char *pathList[]) {
    if (listSize == 0) return;
    for (int i=0; i < listSize; ++i){
        if (i > 0) MainWindow::createNewTab();
        MainWindow::currentTab()->setPath(QString::fromLocal8Bit(pathList[i]));
    }
}

void MainWindow::createNewTab() {
    auto* tab = new LogReaderTab(m_tabs);
    int idx = m_tabs->addTab(tab, "Untitled");
    m_tabs->setCurrentIndex(idx);

    // Update tab title when path changes
    connect(tab, &LogReaderTab::pathChanged, this, &MainWindow::onTabPathChanged);
    
    // Unread markers
    connect(tab, &LogReaderTab::dataAppended, this, &MainWindow::onTabDataAppended);

    // initial state
    m_unreadAny.insert(tab, false);
    m_unreadErr.insert(tab, false);
}

void MainWindow::closeTab(int index) {
    QWidget* w = m_tabs->widget(index);
    if (auto* tab = qobject_cast<LogReaderTab*>(w)) {
        m_unreadAny.remove(tab);
        m_unreadErr.remove(tab);
    }
    m_tabs->removeTab(index);
    delete w;
    if (m_tabs->count() == 0) createNewTab();
}

void MainWindow::onCurrentTabChanged(int index) {
    if (m_lastActive >= 0 && m_lastActive < m_tabs->count()) {
        static_cast<LogReaderTab*>(m_tabs->widget(m_lastActive))->updateLastSeenRow();
    }
    
    if (auto* cur = qobject_cast<LogReaderTab*>(m_tabs->widget(index))) {
        m_unreadAny[cur] = false;
        m_unreadErr[cur] = false;
        clearTabBadge(cur);
    }

    m_lastActive = index;
}

void MainWindow::onTabPathChanged(const QString& base) {    
    if (auto* cur = currentTab()) {
        m_tabs->setTabText(m_tabs->indexOf(cur), base.isEmpty() ? "Untitled" : base);
        setTabBadge(cur);
    }
}

void MainWindow::onTabDataAppended(bool isError) {
    auto* tab = qobject_cast<LogReaderTab*>(sender());
    if (!tab) return;

    if (tab != currentTab()) {
        m_unreadAny[tab] = true;
        if (isError) m_unreadErr[tab] = true;
        setTabBadge(tab);
    }
}

void MainWindow::setTabBadge(LogReaderTab *tab) {
    const int idx = m_tabs->indexOf(tab);
    if (idx < 0) return;
    const bool any  = m_unreadAny.value(tab, false);
    const bool err  = m_unreadErr.value(tab, false);
    const QString base = baseTitleFor(tab);
    if (!any) {
        m_tabs->setTabText(idx, base);
        return;
    }
    const QString prefix = err
        ? QString::fromUtf8("\xF0\x9F\x94\xB4 ")
        : QString::fromUtf8("\xF0\x9F\x9F\xA2 ");
    m_tabs->setTabText(idx, prefix + base);
    m_tabs->setTabToolTip(idx, base);
}

void MainWindow::clearTabBadge(LogReaderTab *tab) {
    const int idx = m_tabs->indexOf(tab);
    if (idx < 0) return;
    const QString base = baseTitleFor(tab);
    m_tabs->setTabText(idx, base);
    m_tabs->setTabToolTip(idx, base);
}

QString MainWindow::baseTitleFor(LogReaderTab *tab) const
{
    const int idx = m_tabs->indexOf(tab);
    if (idx < 0) return "Untitled";
    QString t = m_tabs->tabText(idx);
    // Strip green/red prefix if present
    if (t.startsWith(QString::fromUtf8("\xF0\x9F\x9F\xA2 ")) ||
        t.startsWith(QString::fromUtf8("\xF0\x9F\x94\xB4 "))) {
        t = t.mid(2);
    }

    return t.isEmpty() ? "Untitled" : t;
}

LogReaderTab *MainWindow::currentTab() const {
    return qobject_cast<LogReaderTab*>(m_tabs->currentWidget());
}

void MainWindow::restoreUiState() {
    QSettings s;
    restoreGeometry(s.value("main/geometry").toByteArray());
    restoreState(s.value("main/state").toByteArray());
}
void MainWindow::saveUiState() {
    QSettings s;
    s.setValue("main/geometry", saveGeometry());
    s.setValue("main/state",    saveState());
}
void MainWindow::closeEvent(QCloseEvent* e) {
    saveUiState();
    QMainWindow::closeEvent(e);
}