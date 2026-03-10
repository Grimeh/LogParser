#pragma once
#include <QTabBar>
#include <QToolButton>

class PlusTabBar : public QTabBar {
    Q_OBJECT
public:
    explicit PlusTabBar(QWidget* parent = nullptr);

signals:
    void plusClicked();

protected:
    void tabLayoutChange() override;
    void resizeEvent(QResizeEvent* e) override;

private:
    void repositionPlus();
    QToolButton* m_plus;
};