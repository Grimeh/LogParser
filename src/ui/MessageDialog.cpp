#include "MessageDialog.h"
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTreeView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextEdit>
#include <QColor>

MessageDialog::MessageDialog(const QString& text, QWidget* parent)
    : QDialog(parent), m_rawText(text)
{
    setWindowTitle(tr("Full message"));
    resize(700, 433);
    buildUi();
    tryParseJsonFromRaw();
    onFilterPathChanged(m_filterPath->text());
}

void MessageDialog::buildUi() {
    auto* outer = new QVBoxLayout(this);

    // search row
    auto* searchRow = new QHBoxLayout();
    searchRow->addWidget(new QLabel(tr("Search:")));
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Find…"));
    m_searchCount = new QLabel("", this);
    searchRow->addWidget(m_searchEdit, /*stretch*/1);
    searchRow->addWidget(m_searchCount);
    outer->addLayout(searchRow);

    // JSON filter row
    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel(tr("JSON filter:")));
    auto* dollar = new QLabel("$", this);
    m_filterPath = new QLineEdit(this);
    m_filterPath->setPlaceholderText(tr("['foo']['bar'][0]"));
    filterRow->addWidget(dollar);
    filterRow->addWidget(m_filterPath, /*stretch*/1);
    outer->addLayout(filterRow);

    // --- tabs ---
    m_tabs = new QTabWidget(this);
    outer->addWidget(m_tabs, /*stretch*/1);

    // Text tab
    auto* textPage = new QWidget(this);
    auto* textV    = new QVBoxLayout(textPage);
    textV->setContentsMargins(0,0,0,0);
    m_edit = new QPlainTextEdit(textPage);
    m_edit->setReadOnly(true);
    m_edit->setPlainText(m_rawText);
    textV->addWidget(m_edit, 1);

    auto* textBtns = new QHBoxLayout();
    m_btnPretty = new QPushButton(tr("Format as JSON"), textPage);
    textBtns->addWidget(m_btnPretty);
    textBtns->addStretch(1);
    textV->addLayout(textBtns);

    m_tabs->addTab(textPage, tr("Text"));

    // Tree tab
    auto* treePage = new QWidget(this);
    auto* treeV    = new QVBoxLayout(treePage);
    treeV->setContentsMargins(0,0,0,0);

    m_tree = new QTreeView(treePage);
    m_tree->setAlternatingRowColors(true);
    m_tree->setUniformRowHeights(true);
    treeV->addWidget(m_tree, 1);

    auto* treeBtns = new QHBoxLayout();
    auto* btnExpand   = new QPushButton(tr("Expand All"), treePage);
    auto* btnCollapse = new QPushButton(tr("Collapse All"), treePage);
    treeBtns->addWidget(btnExpand);
    treeBtns->addWidget(btnCollapse);
    treeBtns->addStretch(1);
    treeV->addLayout(treeBtns);

    m_tabs->addTab(treePage, tr("Tree"));

    // bottom row
    auto* bottom = new QHBoxLayout();
    m_status = new QLabel("", this);
    bottom->addWidget(m_status);
    bottom->addStretch(1);
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Close, this);
    bottom->addWidget(bb);
    outer->addLayout(bottom);

    // wiring
    connect(bb, &QDialogButtonBox::rejected, this, &MessageDialog::accept);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MessageDialog::onSearchChanged);
    connect(m_btnPretty,  &QPushButton::clicked,   this, &MessageDialog::onTogglePretty);
    connect(m_filterPath, &QLineEdit::textChanged, this, &MessageDialog::onFilterPathChanged);
    connect(btnExpand,    &QPushButton::clicked,   this, &MessageDialog::onExpandAll);
    connect(btnCollapse,  &QPushButton::clicked,   this, &MessageDialog::onCollapseAll);

    setFilterUiEnabled(false); // disabled until JSON detected
    m_tabs->setTabEnabled(1, false);
    m_tabs->setTabToolTip(1, tr("Tree becomes available when content is valid JSON"));
    m_status->setText("");
}

void MessageDialog::setFilterUiEnabled(bool on) {
    m_filterPath->setEnabled(on);
    if (!on) m_filterPath->setToolTip(tr("JSON filter is available when content is valid JSON"));
    else     m_filterPath->setToolTip(tr("Enter a path tail after $ (e.g. ['foo']['bar'][0])"));
}

void MessageDialog::tryParseJsonFromRaw() {
    m_hasJson = false;
    m_fullJson = QJsonValue();

    QJsonParseError err{};
    const auto doc = QJsonDocument::fromJson(m_rawText.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        // not JSON
        m_status->setText(tr("Not JSON: %1").arg(err.errorString()));
        m_btnPretty->setText(tr("Format as JSON"));
        m_isPretty = false;
        setFilterUiEnabled(false);
        m_tabs->setTabEnabled(1, false);
        return;
    }
    m_hasJson = true;
    m_fullJson = (doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object()));
    m_status->setText(tr("JSON detected"));
    setFilterUiEnabled(true);
    m_tabs->setTabEnabled(1, true);

    // show pretty in text if currently pretty is requested
    if (m_isPretty) setTextFromJson(m_fullJson, /*pretty*/true);
    // build tree on full json
    rebuildTree(m_fullJson);
}

void MessageDialog::setTextFromJson(const QJsonValue& v, bool pretty) {
    if (!v.isArray() && !v.isObject()) return;
    QJsonDocument doc(v.toObject());
    if (v.isArray()) doc = QJsonDocument(v.toArray());
    const QByteArray out = pretty ? doc.toJson(QJsonDocument::Indented)
                                  : doc.toJson(QJsonDocument::Compact);
    m_edit->setPlainText(QString::fromUtf8(out));
}

void MessageDialog::rebuildTree(const QJsonValue& v) {
    // keep expansion
    const auto expanded = m_treeModel ? gatherExpandedPaths() : QVector<Path>{};

    auto* model = new QStandardItemModel(m_tree);
    model->setHorizontalHeaderLabels({tr("Key"), tr("Value")});
    auto* root = model->invisibleRootItem();

    // start from (empty key, value)
    addNode(root, QString(), v);

    m_treeModel = model;
    m_tree->setModel(model);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    reexpandPaths(expanded);
    highlightTree(m_searchEdit->text());
}

void MessageDialog::addNode(QStandardItem* parentKeyCol, const QString& key, const QJsonValue& v) {
    auto* keyItem = new QStandardItem(key);
    QStandardItem* valItem = nullptr;

    if (v.isObject()) {
        valItem = new QStandardItem(QStringLiteral("{…}"));
        parentKeyCol->appendRow({keyItem, valItem});
        const auto obj = v.toObject();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            addNode(keyItem, it.key(), it.value());
        }
    } else if (v.isArray()) {
        const auto arr = v.toArray();
        valItem = new QStandardItem(QStringLiteral("[%1]").arg(arr.size()));
        parentKeyCol->appendRow({keyItem, valItem});
        for (int i = 0; i < arr.size(); ++i) {
            addNode(keyItem, QString::number(i), arr.at(i));
        }
    } else {
        valItem = new QStandardItem(toDisplay(v));
        parentKeyCol->appendRow({keyItem, valItem});
    }
}

QString MessageDialog::toDisplay(const QJsonValue& v) {
    switch (v.type()) {
        case QJsonValue::Null:   return QStringLiteral("null");
        case QJsonValue::Bool:   return v.toBool() ? QStringLiteral("true") : QStringLiteral("false");
        case QJsonValue::Double: return QString::number(v.toDouble(), 'g', 16);
        case QJsonValue::String: return v.toString();
        default: return QString();
    }
}

// expansion preservation
QVector<MessageDialog::Path> MessageDialog::gatherExpandedPaths() const {
    QVector<Path> paths;
    if (!m_treeModel) return paths;

    auto gather = [&](auto&& self, QStandardItem* item, Path cur) -> void {
        if (!item) return;
        const QModelIndex idx = m_treeModel->indexFromItem(item);
        if (m_tree->isExpanded(idx)) paths.push_back(cur);
        for (int r = 0; r < item->rowCount(); ++r) {
            auto* child = item->child(r, 0);
            if (!child) continue;
            auto next = cur;
            next.push_back(child->text());
            self(self, child, next);
        }
    };
    auto* root = m_treeModel->invisibleRootItem();
    for (int r = 0; r < root->rowCount(); ++r) {
        auto* top = root->child(r, 0);
        if (!top) continue;
        gather(gather, top, Path{top->text()});
    }
    return paths;
}

void MessageDialog::reexpandPaths(const QVector<Path>& paths) {
    if (!m_treeModel) return;

    auto findPath = [&](const Path& p) -> QStandardItem* {
        auto* item = m_treeModel->invisibleRootItem();
        for (const auto& seg : p) {
            QStandardItem* found = nullptr;
            for (int r = 0; r < item->rowCount(); ++r) {
                auto* cand = item->child(r, 0);
                if (cand && cand->text() == seg) { found = cand; break; }
            }
            if (!found) return nullptr;
            item = found;
        }
        return item;
    };

    for (const auto& p : paths) {
        if (auto* it = findPath(p)) {
            const QModelIndex idx = m_treeModel->indexFromItem(it);
            m_tree->setExpanded(idx, true);
        }
    }
}

// search highlighting
void MessageDialog::onSearchChanged(const QString& term) {
    highlightText(term);
    highlightTree(term);
}

void MessageDialog::highlightText(const QString& term) {
    QList<QTextEdit::ExtraSelection> sels;
    if (!term.isEmpty()) {
        const QString text = m_edit->toPlainText();
        const QString hayL = text.toLower();
        const QString needle = term.toLower();
        int i = 0, count = 0;
        QTextCharFormat fmt;
        fmt.setBackground(QColor(255,235,59,180));
        fmt.setForeground(Qt::black);
        while ((i = hayL.indexOf(needle, i)) >= 0) {
            QTextEdit::ExtraSelection sel;
            QTextCursor c(m_edit->document());
            c.setPosition(i);
            c.setPosition(i + needle.size(), QTextCursor::KeepAnchor);
            sel.cursor = c;
            sel.format = fmt;
            sels.push_back(sel);
            i += needle.size();
            ++count;
        }
        m_searchCount->setText(count == 0 ? "" :
                               tr("%1 match%2").arg(count).arg(count==1?"":"es"));
    } else {
        m_searchCount->setText("");
    }
    m_edit->setExtraSelections(sels);
}

void MessageDialog::highlightTree(const QString& term) {
    if (!m_treeModel) return;
    const QString termL = term.toLower();

    auto reset = [](QStandardItem* it){
        it->setData(QVariant(), Qt::BackgroundRole);
        it->setData(QVariant(), Qt::ForegroundRole);
    };
    auto apply = [](QStandardItem* it){
        it->setBackground(QColor(255,235,59,180));
        it->setForeground(Qt::black);
    };

    auto* root = m_treeModel->invisibleRootItem();
    for (int r = 0; r < root->rowCount(); ++r) {
        auto walk = [&](auto&& self, QStandardItem* item) -> void {
            if (!item) return;
            auto* parent = item->parent() ? item->parent() : root;
            auto* val    = parent->child(item->row(), 1);

            reset(item);
            if (val) reset(val);

            if (!termL.isEmpty()) {
                const QString key = item->text();
                const QString value = val ? val->text() : "";
                if (key.toLower().contains(termL) || value.toLower().contains(termL)) {
                    apply(item);
                    if (val) apply(val);
                }
            }
            for (int i = 0; i < item->rowCount(); ++i) self(self, item->child(i, 0));
        };
        auto* top = root->child(r, 0);
        if (top) walk(walk, top);
    }
}

// JSON-path
bool MessageDialog::stepKey(QJsonValue& cur, const QString& key, QString& error) {
    if (!cur.isObject()) { error = tr("Current value is not an object"); return false; }
    const auto obj = cur.toObject();
    if (!obj.contains(key)) { error = tr("Key not found: %1").arg(key); return false; }
    cur = obj.value(key);
    return true;
}
bool MessageDialog::stepIndex(QJsonValue& cur, int idx, QString& error) {
    if (!cur.isArray()) { error = tr("Current value is not an array"); return false; }
    const auto arr = cur.toArray();
    if (idx < 0 || idx >= arr.size()) { error = tr("Index out of range: %1").arg(idx); return false; }
    cur = arr.at(idx);
    return true;
}

bool MessageDialog::evalJsonPath(const QJsonValue& root, const QString& expr, QJsonValue& out, QString& error) {
    // expects path tail that starts with $ or empty: "$['a'][0]"
    QString s = expr.trimmed();
    if (s.isEmpty()) { out = root; return true; }
    if (!s.startsWith('$')) { error = tr("Path must start with $"); return false; }

    int i = 1, n = s.size();
    QJsonValue cur = root;
    while (i < n) {
        if (s[i] != '[') { error = tr("Expected [ at %1").arg(i); return false; }
        ++i;
        if (i >= n) { error = tr("Unterminated ["); return false; }

        if (s[i] == '\'' || s[i] == '"') {
            const QChar quote = s[i++];
            const int start = i;
            while (i < n && s[i] != quote) ++i;
            if (i >= n) { error = tr("Unterminated quoted key"); return false; }
            const QString key = s.mid(start, i - start);
            ++i;
            if (i >= n || s[i] != ']') { error = tr("Expected ] after key"); return false; }
            ++i;
            if (!stepKey(cur, key, error)) return false;
        } else {
            // index
            int start = i;
            while (i < n && s[i].isDigit()) ++i;
            if (i == start) { error = tr("Expected index at %1").arg(i); return false; }
            const int idx = s.mid(start, i - start).toInt();
            if (i >= n || s[i] != ']') { error = tr("Expected ] after index"); return false; }
            ++i;
            if (!stepIndex(cur, idx, error)) return false;
        }
    }
    out = cur;
    return true;
}

// UI actions
void MessageDialog::onTogglePretty() {
    if (!m_hasJson) {
        // Try formatting current text as JSON to enable features
        if (!m_isPretty) {
            QJsonParseError err{};
            const auto doc = QJsonDocument::fromJson(m_edit->toPlainText().toUtf8(), &err);
            if (err.error != QJsonParseError::NoError) {
                m_status->setText(tr("Not valid JSON: %1").arg(err.errorString()));
                return;
            }
            m_hasJson = true;
            m_fullJson = (doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object()));
            setFilterUiEnabled(true);
            m_tabs->setTabEnabled(1, true);
        }
    }

    m_isPretty = !m_isPretty;
    if (m_isPretty) {
        // Show pretty of either full or current filtered output if filter present
        QJsonValue shown = m_fullJson;
        const QString tail = m_filterPath->text().trimmed();
        if (!tail.isEmpty()) {
            QJsonValue sub; QString err;
            if (evalJsonPath(m_fullJson, "$" + tail, sub, err)) shown = sub;
        }
        setTextFromJson(shown, /*pretty*/true);
        m_btnPretty->setText(tr("Show raw"));
        m_status->setText(tr("Pretty-printed JSON"));
    } else {
        m_edit->setPlainText(m_rawText);
        m_btnPretty->setText(tr("Format as JSON"));
        m_status->setText("");
    }
    highlightText(m_searchEdit->text());
}

void MessageDialog::onFilterPathChanged(const QString& tail) {
    if (!m_hasJson) { setFilterUiEnabled(false); m_tabs->setTabEnabled(1, false); return; }
    setFilterUiEnabled(true);  m_tabs->setTabEnabled(1, true);

    const QString expr = "$" + tail.trimmed();
    QJsonValue sub; QString err;
    if (!tail.isEmpty() && !evalJsonPath(m_fullJson, expr, sub, err)) {
        m_status->setText(tr("Filter error: %1").arg(err));
        // keep old tree, but reflect error in status
        return;
    }
    const QJsonValue shown = tail.isEmpty() ? m_fullJson : sub;

    if (m_isPretty) setTextFromJson(shown, /*pretty*/true);
    // Rebuild tree and keep expansion/highlights
    rebuildTree(shown);
    m_status->setText(tail.isEmpty() ? tr("JSON (full)") : tr("Filtered: %1").arg(expr));
}

void MessageDialog::onExpandAll()   { m_tree->expandAll(); }
void MessageDialog::onCollapseAll() { m_tree->collapseAll(); }