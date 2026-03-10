#pragma once
#include <QTabWidget>

class PlusTabBar;

class TabsWidget : public QTabWidget {
    Q_OBJECT
public:
    explicit TabsWidget(QWidget* parent = nullptr);

signals:
    void plusClicked();

private:
    PlusTabBar* m_bar{};
};