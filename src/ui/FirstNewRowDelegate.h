#pragma once
#include <QStyledItemDelegate>
#include <QPen>

class LogTableModel;

class FirstNewRowDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    FirstNewRowDelegate(LogTableModel* sourceModel, QObject* parent = nullptr);

    void setEnabled(bool on);
    bool isEnabled() const { return m_enabled; }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

private:
    LogTableModel* m_source;
    mutable QPen m_pen;
    bool m_enabled = true;
};