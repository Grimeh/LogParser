#include "TabsWidget.h"
#include "PlusTabBar.h"

TabsWidget::TabsWidget(QWidget* parent)
    : QTabWidget(parent)
{
    // Create and install our custom tab bar here (allowed: we're inside the subclass)
    m_bar = new PlusTabBar(this);
    connect(m_bar, &PlusTabBar::plusClicked, this, &TabsWidget::plusClicked);

    setTabBar(m_bar);
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(true);
}