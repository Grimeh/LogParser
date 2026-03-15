#include "MultiFilterProxy.h"
#include "LogTableModel.h"
#include <QModelIndex>

MultiFilterProxy::MultiFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

void MultiFilterProxy::ensureRegexCapacity(int columns) {
    if (columns <= 0) { m_colRegex.clear(); return; }
    if ((int)m_colRegex.size() < columns) m_colRegex.resize(columns);
}

void MultiFilterProxy::setFilterRegexForColumn(int column, const std::optional<QRegularExpression>& rx) {
    beginFilterChange();
    ensureRegexCapacity(sourceModel() ? sourceModel()->columnCount() : column+1);
    if (column >= 0) m_colRegex[(size_t)column] = rx;
    endFilterChange();
}

void MultiFilterProxy::clearAllColumnFilters() {
    beginFilterChange();
    m_colRegex.clear();
    endFilterChange();
}

void MultiFilterProxy::setTimeRange(const std::optional<QDateTime>& from,
                                    const std::optional<QDateTime>& to)
{
    beginFilterChange();
    m_from = from;
    m_to   = to;
    endFilterChange();
}

LogTableModel* MultiFilterProxy::sourceModelPtr() const {
    return qobject_cast<LogTableModel*>(sourceModel());
}

bool MultiFilterProxy::filterAcceptsRow(int srcRow, const QModelIndex& srcParent) const {
    const auto* m = sourceModelPtr();
    if (!m) return true;

    const int cols = m->columnCount();
    // Column regex filters
    for (int c = 0; c < cols && c < (int)m_colRegex.size(); ++c) {
        const auto& optRx = m_colRegex[(size_t)c];
        if (!optRx) continue;
        const QModelIndex idx = m->index(srcRow, c, srcParent);
        const QString hay = m->data(idx, Qt::DisplayRole).toString();
        if (!optRx->match(hay).hasMatch()) return false;
    }

    // Time window
    if (m_from || m_to) {
        // Ask model for row dict (or expose asctime via helper)
        const auto dict = m->rowDict(srcRow);
        const QString asc = dict.value(QStringLiteral("asctime"));
        if (asc.isEmpty()) return false;  // no timestamp => exclude if time range used

        // Model doesn't store original format; LogReaderTab parsed QDateTime when ingesting
        // For correct filtering we leverage model’s stored QDateTime:
        // add a helper method in model to get asctime for a row.
        const QDateTime asdt = m->asctimeForRow(srcRow);
        if (!asdt.isValid()) return false;

        if (m_from && asdt < *m_from) return false;
        if (m_to   && asdt > *m_to)   return false;
    }
    return true;
}

bool MultiFilterProxy::lessThan(const QModelIndex& l, const QModelIndex& r) const {
    const auto* m = sourceModelPtr();
    if (!m) return QSortFilterProxyModel::lessThan(l, r);

    // If sorting by "asctime", use QDateTime
    const int sortCol = sortColumn();
    if (sortCol >= 0 && sortCol < m->columnCount()) {
        const QString name = m->columns().value(sortCol);
        if (name == QStringLiteral("asctime")) {
            const QDateTime ld = m->asctimeForRow(l.row());
            const QDateTime rd = m->asctimeForRow(r.row());
            if (ld.isValid() && rd.isValid()) {
                if (ld == rd) return l.row() < r.row();
                return ld < rd;
            }
        }
    }
    // fallback to text compare
    const QString lv = m->data(l, Qt::DisplayRole).toString();
    const QString rv = m->data(r, Qt::DisplayRole).toString();
    if (lv == rv) return l.row() < r.row();
    return QString::compare(lv, rv, Qt::CaseInsensitive) < 0;
}