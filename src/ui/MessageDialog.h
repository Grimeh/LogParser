#pragma once
#include <QDialog>
#include <QJsonValue>
#include <QPointer>
#include <QVector>

class QPlainTextEdit;
class QTabWidget;
class QTreeView;
class QStandardItemModel;
class QLineEdit;
class QLabel;
class QPushButton;
class QStandardItem;

class MessageDialog : public QDialog {
    Q_OBJECT
public:
    explicit MessageDialog(const QString& text, QWidget* parent = nullptr);

private slots:
    void onSearchChanged(const QString& term);
    void onTogglePretty();
    void onFilterPathChanged(const QString& path);
    void onExpandAll();
    void onCollapseAll();

private:
    // raw text
    QString          m_rawText;
    bool             m_isPretty = false;

    // JSON state
    bool             m_hasJson = false;
    QJsonValue       m_fullJson;      // full parsed JSON (if any)
    QJsonValue       m_filteredJson;  // result of JSON-path (if any)
    QPointer<QStandardItemModel> m_treeModel;

    // UI
    QPointer<QTabWidget>     m_tabs;
    QPointer<QPlainTextEdit> m_edit;
    QPointer<QTreeView>      m_tree;
    QPointer<QLineEdit>      m_searchEdit;
    QPointer<QLabel>         m_searchCount;
    QPointer<QLineEdit>      m_filterPath;
    QPointer<QLabel>         m_status;
    QPointer<QPushButton>    m_btnPretty;

    // Build & state helpers
    void buildUi();
    void setFilterUiEnabled(bool on);

    // JSON helpers
    void tryParseJsonFromRaw();
    bool extractFirstJsonFragment(const QString& s,
                                  int& outBegin, int& outEnd,
                                  QJsonValue& outJson,
                                  QString& error);
    void setTextFromJson(const QJsonValue& v, bool pretty);
    void rebuildTree(const QJsonValue& v);

    // Expansion preservation
    using Path = QVector<QString>;   // sequence of key/index strings
    QVector<Path> gatherExpandedPaths() const;
    void          reexpandPaths(const QVector<Path>&);

    // Search highlight
    void highlightText(const QString& term);
    void highlightTree(const QString& term);

    // JSON-path evaluator: $['key'][0]['sub'] ...
    bool evalJsonPath(const QJsonValue& root, const QString& expr, QJsonValue& out, QString& error);
    bool stepKey(QJsonValue& cur, const QString& key, QString& error);
    bool stepIndex(QJsonValue& cur, int idx, QString& error);

    // Build QStandardItemModel from QJsonValue
    void addNode(QStandardItem* parentKeyCol, const QString& key, const QJsonValue& v);

    // Util
    static QString toDisplay(const QJsonValue& v);
};