#pragma once
#include <QWidget>
#include <QDateTime>
#include "core/LogParser.h"

class QLineEdit;
class QPushButton;
class QComboBox;
class QTableView;
class QLabel;
class QFormLayout;
class QCheckBox;
class QDateTimeEdit;

class LogTailer;
class QSortFilterProxyModel;
class LogTableModel;

class LogReaderTab : public QWidget {
    Q_OBJECT
public:
    explicit LogReaderTab(QWidget* parent = nullptr);
    ~LogReaderTab() override;

    void updateLastSeenRow();
    void setPath(const QString& path);

signals:
    void pathChanged(const QString& baseName);
    void dataAppended(bool isError);

private:
    // row 1
    QLineEdit*  m_pathEdit{};
    QPushButton* m_browseBtn{};

    // row 2
    QComboBox*  m_modeCombo{};
    QLineEdit*  m_formatEdit{};
    QLineEdit*  m_datefmtEdit{};
    QPushButton* m_applyBtn{};

    // row 3
    QPushButton* m_startBtn{};
    QPushButton* m_stopBtn{};
    QPushButton* m_clearBtn{};
    QPushButton* m_reloadBtn{};

    // table
    QTableView* m_table{};

    // status
    QLabel* m_status{};

    // LogTailer
    LogTailer* m_tailer{};

    // Model and proxy
    LogTableModel*         m_model{};
    QSortFilterProxyModel* m_proxy{};

    // parsing state
    LogParser::ParseResult m_compiled;
    QString                m_datefmtQt = QStringLiteral("yyyy-MM-dd HH:mm:ss");

    // filters panel widgets
    QWidget*     m_columnsContainer{};     // holds form rows for columns
    QFormLayout* m_columnsForm{};
    QVector<QCheckBox*> m_showChecks;
    QVector<QLineEdit*> m_filterEdits;

    // time range
    QCheckBox*     m_cbUseFrom{};
    QCheckBox*     m_cbUseTo{};
    QDateTimeEdit* m_fromDt{};
    QDateTimeEdit* m_toDt{};
    QLabel*        m_noTimeNotice{};

    // datetime tracking
    bool      m_haveAnyDatetime = false;
    bool      m_userSetFrom = false;
    bool      m_userSetTo   = false;
    QDateTime m_minDt;
    QDateTime m_maxDt;

private slots:
    void onBrowse();
    void onApplyFormat();
    void onStart();
    void onStop();
    void onClear();
    void onReloadAll();
    void onPathTextChanged(const QString& text);
    void onTailNewLine(const QString& line);
    void onTailFileError(const QString& msg);
    void onCellDoubleClicked(const QModelIndex& proxyIndex);
    void copySelection();

private:
    void buildHeaderRows(QWidget* parent);
    void buildTable(QWidget* parent);
    void buildFiltersPanel(QWidget* parent);
    void buildStatus(QWidget* parent);
    void setStatus(const QString& text);
    void rebuildModelCols(const QStringList& cols);
    QDateTime parseAsctimeQt(const QString& s) const;
    void parseAndAddRow(const QString& raw, const bool& emitDataAppend);
    void rebuildColumnFiltersForm(const QStringList& cols);
    void clearAllFilters();
    void updateTimeControlsEnabled();
    void maybeSeedTimeEditors();
    bool rowIsError(const QHash<QString, QString>& dict) const;
};
