#include "FirstNewRowDelegate.h"
#include "core/LogTableModel.h"
#include <QPainter>

FirstNewRowDelegate::FirstNewRowDelegate(LogTableModel* source, QObject* parent)
    : QStyledItemDelegate(parent), m_source(source)
{
    m_pen = QPen(QColor(0,255,0));
    m_pen.setWidth(2);
}

void FirstNewRowDelegate::setEnabled(bool on) { m_enabled = on; }

void FirstNewRowDelegate::paint(QPainter* p, const QStyleOptionViewItem& opt,
                                const QModelIndex& idx) const
{
    QStyledItemDelegate::paint(p, opt, idx);
    if (!m_enabled || !m_source) return;

    const int srcRow = idx.row();
    const int firstNew = m_source->firstNewRowIndex();
    if (firstNew < 0) return;

    if (srcRow == firstNew) {
        p->save();
        p->setPen(m_pen);
        const int y = opt.rect.top() + 1;
        p->drawLine(opt.rect.left(), y, opt.rect.right(), y);
        p->restore();
    }
}