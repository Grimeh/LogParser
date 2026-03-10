#pragma once
#include <QMainWindow>

class TabsWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void createNewTab();
    void closeTab(int index);
    void onCurrentTabChanged(int index);
    void updateTabTitleFor(const QString& base);

private:
    TabsWidget* m_tabs{};
    int m_lastActive{-1};
};
