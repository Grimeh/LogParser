#include "LogTableModel.h"
#include <algorithm>

LogTableModel::LogTableModel(const QStringList& columns, QObject* parent)
    : QAbstractTableModel(parent), m_columns(columns)
{}

void LogTableModel::setColumns(const QStringList& cols) {
    beginResetModel();
    m_columns = cols;
    // ensure every existing row has keys for all columns
    for (auto& r : m_rows) {
        for (const auto& c : m_columns) {
            if (!r.dict.contains(c)) r.dict.insert(c, QString());
        }
    }
    endResetModel();
}

void LogTableModel::clear() {
    beginResetModel();
    m_rows.clear();
    m_lastSeenRow = -1;
    endResetModel();
}

void LogTableModel::addRow(const QHash<QString, QString>& dict,
                           const QDateTime& asctime)
{
    Row row;
    row.dict = dict;
    // add missing columns
    for (const auto& c : m_columns) {
        if (!row.dict.contains(c)) row.dict.insert(c, QString());
    }
    row.asctime = asctime;

    const int pos = m_rows.size();
    beginInsertRows(QModelIndex(), pos, pos);
    m_rows.push_back(std::move(row));
    endInsertRows();
}

void LogTableModel::updateLastRowMessage(const QString& suffix) {
    if (m_rows.isEmpty()) return;
    const int lastIdx = m_rows.size() - 1;
    // choose message column or last column as fallback
    int msgCol = m_columns.indexOf("message");
    if (msgCol < 0 && !m_columns.isEmpty())
        msgCol = m_columns.size()-1;

    const QString key = (msgCol >= 0 ? m_columns[msgCol] : QString("message"));
    auto& r = m_rows[lastIdx];
    const QString prev = r.dict.value(key);
    r.dict.insert(key, prev + "\n" + suffix);

    const QModelIndex tl = index(lastIdx, (msgCol >= 0 ? msgCol : 0));
    const QModelIndex br = tl;
    emit dataChanged(tl, br, {Qt::DisplayRole});
}

void LogTableModel::setLastSeenRow(int idx) {
    m_lastSeenRow = idx;
    if (m_rows.isEmpty() || m_columns.isEmpty()) return;
    const QModelIndex tl = index(0,0);
    const QModelIndex br = index(m_rows.size()-1, m_columns.size()-1);
    emit dataChanged(tl, br, {}); // delegate might repaint
}

int LogTableModel::firstNewRowIndex() const {
    const int fn = m_lastSeenRow + 1;
    return (fn >= 0 && fn < m_rows.size()) ? fn : -1;
}

QHash<QString, QString> LogTableModel::rowDict(int row) const {
    if (row < 0 || row >= m_rows.size()) return {};
    return m_rows[row].dict;
}

int LogTableModel::rowCount(const QModelIndex&) const {
    return m_rows.size();
}

int LogTableModel::columnCount(const QModelIndex&) const {
    return m_columns.size();
}

QVariant LogTableModel::headerData(int section, Qt::Orientation o, int role) const {
    if (role == Qt::DisplayRole && o == Qt::Horizontal) {
        if (section >= 0 && section < m_columns.size())
            return m_columns[section];
    }
    return {};
}

QVariant LogTableModel::data(const QModelIndex& idx, int role) const {
    if (!idx.isValid()) return {};
    const int r = idx.row(), c = idx.column();
    if (r < 0 || r >= m_rows.size()) return {};
    if (c < 0 || c >= m_columns.size()) return {};
    const QString key = m_columns[c];
    const auto& row = m_rows[r];

    if (role == Qt::DisplayRole) {
        return row.dict.value(key);
    }

    // color by level
    if (role == Qt::BackgroundRole || role == Qt::ForegroundRole) {
        const QString lvl = row.dict.value("levelname",
                          row.dict.value("level")).toUpper();
        auto [fg, bg] = levelColors(lvl);
        if (role == Qt::BackgroundRole && bg) return *bg;
        if (role == Qt::ForegroundRole && fg) return *fg;
    }

    // center non-message columns
    if (role == Qt::TextAlignmentRole) {
        if (key != "message") return Qt::AlignCenter;
    }

    return {};
}

std::pair<std::optional<QBrush>, std::optional<QBrush>>
LogTableModel::levelColors(const QString& lvl) {
    // (fg, bg); themed but simple
    if (lvl == "CRITICAL" || lvl == "FATAL") {
        return { QBrush(QColor(255,220,220)), QBrush(QColor(80,0,0)) };
    }
    if (lvl == "ERROR") {
        return { QBrush(QColor(255,200,200)), QBrush(QColor(70,0,0)) };
    }
    if (lvl == "WARNING") {
        return { QBrush(QColor(255,240,200)), QBrush(QColor(80,60,0)) };
    }
    if (lvl == "INFO") {
        return { QBrush(QColor(210,230,255)), QBrush(QColor(0,40,70)) };
    }
    if (lvl == "DEBUG") {
        return { QBrush(QColor(200,200,255)), QBrush(QColor(20,20,60)) };
    }
    if (lvl == "TRACE" || lvl == "NOTSET") {
        return { QBrush(QColor(200,200,200)), QBrush(QColor(30,30,30)) };
    }
    return { std::nullopt, std::nullopt };
}