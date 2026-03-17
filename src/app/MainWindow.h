#pragma once
#include <QMainWindow>
#include <QHash>

class TabsWidget;
class LogReaderTab;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;
    void openLogList(int listSize, char* pathList[]);

private slots:
    void createNewTab();
    void closeTab(int index);
    void onCurrentTabChanged(int index);
    void onTabPathChanged(const QString& base);
    void onTabDataAppended(bool isError);

private:
    TabsWidget* m_tabs{};
    int m_lastActive{-1};
    
    QHash<LogReaderTab*, bool> m_unreadAny;
    QHash<LogReaderTab*, bool> m_unreadErr;

    void setTabBadge(LogReaderTab* tab);
    void clearTabBadge(LogReaderTab* tab);
    QString baseTitleFor(LogReaderTab* tab) const;
    LogReaderTab* currentTab() const;
    void restoreUiState();
    void saveUiState();

protected:
    void closeEvent(QCloseEvent* e) override;
};
