#pragma once
#include <QWidget>

class QLineEdit;
class QPushButton;
class QComboBox;
class QTableView;
class QLabel;

class LogReaderTab : public QWidget {
    Q_OBJECT
public:
    explicit LogReaderTab(QWidget* parent = nullptr);

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

private slots:
    void onBrowse();
    void onApplyFormat();
    void onStart();
    void onStop();
    void onClear();
    void onReloadAll();
    void onPathTextChanged(const QString& text);

private:
    void buildHeaderRows(QWidget* parent);
    void buildTable(QWidget* parent);
    void buildFiltersPanel(QWidget* parent);
    void buildStatus(QWidget* parent);
    void setStatus(const QString& text);
};
