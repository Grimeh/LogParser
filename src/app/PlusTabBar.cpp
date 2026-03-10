#include "PlusTabBar.h"
#include <QStyle>
#include <QResizeEvent>

PlusTabBar::PlusTabBar(QWidget* parent)
    : QTabBar(parent), m_plus(new QToolButton(this))
{
    m_plus->setText("+");
    m_plus->setToolTip("New tab");
    connect(m_plus, &QToolButton::clicked, this, &PlusTabBar::plusClicked);

    setMovable(true);
    repositionPlus();
}

void PlusTabBar::tabLayoutChange() {
    QTabBar::tabLayoutChange();
    repositionPlus();
}

void PlusTabBar::resizeEvent(QResizeEvent* e) {
    QTabBar::resizeEvent(e);
    repositionPlus();
}

void PlusTabBar::repositionPlus() {
    int x = 8;
    if (count() > 0) {
        QRect last = tabRect(count() - 1);
        x = last.right() + 8;
    }
    const int w = m_plus->sizeHint().width();
    const int h = m_plus->sizeHint().height();
    const int y = (height() - h) / 2;
    x = std::min(std::max(4, x), width() - w - 4);
    m_plus->move(x, std::max(0, y));
}