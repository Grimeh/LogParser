#pragma once
#include <QAbstractTableModel>
#include <QDateTime>
#include <QBrush>
#include <QVector>
#include <QHash>

class LogTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit LogTableModel(const QStringList& columns, QObject* parent = nullptr);

    // columns management
    void setColumns(const QStringList& columns);
    const QStringList& columns() const { return m_columns; }

    // data ops
    void clear();
    void addRow(const QHash<QString, QString>& dict, const QDateTime& asctime = {});
    void updateLastRowMessage(const QString& suffix);

    // last-seen index (for "first new row" marker)
    void setLastSeenRow(int idx);
    int  firstNewRowIndex() const;

    // helpers
    QHash<QString, QString> rowDict(int row) const;

    // Qt model
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant headerData(int section, Qt::Orientation, int role) const override;
    QVariant data(const QModelIndex& index, int role) const override;

private:
    struct Row {
        QHash<QString, QString> dict;
        QDateTime asctime;
    };
    QStringList m_columns;
    QVector<Row> m_rows;
    int m_lastSeenRow = -1;

    // color helpers
    static std::pair<std::optional<QBrush>, std::optional<QBrush>>
    levelColors(const QString& lvl) ;
};