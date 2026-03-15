#pragma once
#include <QSortFilterProxyModel>
#include <QDateTime>
#include <QRegularExpression>
#include <optional>
#include <vector>

class LogTableModel;

class MultiFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit MultiFilterProxy(QObject* parent = nullptr);

    // Per-column regex filter (nullptr = cleared)
    void setFilterRegexForColumn(int column, const std::optional<QRegularExpression>& rx);
    void clearAllColumnFilters();

    // Time window (inclusive); std::nullopt means “disabled”
    void setTimeRange(const std::optional<QDateTime>& from, const std::optional<QDateTime>& to);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    std::vector<std::optional<QRegularExpression>> m_colRegex;
    std::optional<QDateTime> m_from;
    std::optional<QDateTime> m_to;

    LogTableModel* sourceModelPtr() const;
    void ensureRegexCapacity(int columns);
};