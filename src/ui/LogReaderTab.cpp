#include "LogReaderTab.h"
#include "core/LogTailer.h"
#include "core/LogTableModel.h"
#include "ui/FirstNewRowDelegate.h"
#include "core/MultiFilterProxy.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QtLogging>

LogReaderTab::LogReaderTab(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    buildHeaderRows(this);
    buildTable(this);
    buildFiltersPanel(this);
    buildStatus(this);

    m_compiled = LogParser::compileFormatToRegex("%(asctime)s %(levelname)s %(name)s %(funcName)s %(message)s");

    setStatus("Ready");
}

LogReaderTab::~LogReaderTab() {
    if (m_tailer) {
        m_tailer->stop();
        m_tailer->wait(2000);
        m_tailer->deleteLater();
        m_tailer = nullptr;
    }
}

void LogReaderTab::updateLastSeenRow() {
    m_model->setLastSeenRow(m_model->rowCount() - 1);
}

void LogReaderTab::buildHeaderRows(QWidget* parent) {
    // Row 1: File path + Browse
    {
        auto* row = new QHBoxLayout();
        row->setContentsMargins(0,0,0,0);
        auto* lbl = new QLabel("File:", parent);
        m_pathEdit = new QLineEdit(parent);
        m_pathEdit->setPlaceholderText("Path to .log file");
        m_pathEdit->setMinimumWidth(400);
        connect(m_pathEdit, &QLineEdit::textChanged, this, &LogReaderTab::onPathTextChanged);

        m_browseBtn = new QPushButton("Browse", parent);
        connect(m_browseBtn, &QPushButton::clicked, this, &LogReaderTab::onBrowse);

        row->addWidget(lbl);
        row->addWidget(m_pathEdit, 1);
        row->addWidget(m_browseBtn);
        static_cast<QVBoxLayout*>(layout())->addLayout(row);
    }

    // Row 2: Mode + Format + Datefmt + Apply
    {
        auto* row = new QHBoxLayout();
        row->setContentsMargins(0,0,0,0);

        auto* lblFmt = new QLabel("Format:", parent);
        m_modeCombo = new QComboBox(parent);
        m_modeCombo->addItems({"py_logging_fmt", "regex"});
        m_modeCombo->setCurrentText("py_logging_fmt");

        m_formatEdit = new QLineEdit(parent);
        m_formatEdit->setText("%(asctime)s %(levelname)s %(name)s %(funcName)s %(message)s");
        m_formatEdit->setMinimumWidth(500);

        auto* lblDate = new QLabel("Datefmt:", parent);
        m_datefmtEdit = new QLineEdit(parent);
        m_datefmtEdit->setText("yyyy-MM-dd HH:mm:ss");
        m_datefmtEdit->setMinimumWidth(200);

        m_applyBtn = new QPushButton("Apply Format", parent);
        connect(m_applyBtn, &QPushButton::clicked, this, &LogReaderTab::onApplyFormat);

        row->addWidget(lblFmt);
        row->addWidget(m_modeCombo);
        row->addWidget(m_formatEdit, 1);
        row->addWidget(lblDate);
        row->addWidget(m_datefmtEdit);
        row->addWidget(m_applyBtn);
        static_cast<QVBoxLayout*>(layout())->addLayout(row);
    }

    // Row 3: Controls
    {
        auto* row = new QHBoxLayout();
        row->setContentsMargins(0,0,0,0);

        m_startBtn  = new QPushButton("Start", parent);
        m_stopBtn   = new QPushButton("Stop", parent);
        m_clearBtn  = new QPushButton("Clear", parent);
        m_reloadBtn = new QPushButton("Reload All", parent);

        connect(m_startBtn,  &QPushButton::clicked, this, &LogReaderTab::onStart);
        connect(m_stopBtn,   &QPushButton::clicked, this, &LogReaderTab::onStop);
        connect(m_clearBtn,  &QPushButton::clicked, this, &LogReaderTab::onClear);
        connect(m_reloadBtn, &QPushButton::clicked, this, &LogReaderTab::onReloadAll);

        row->addWidget(m_startBtn);
        row->addWidget(m_stopBtn);
        row->addWidget(m_clearBtn);
        row->addWidget(m_reloadBtn);
        row->addStretch(1);
        static_cast<QVBoxLayout*>(layout())->addLayout(row);

        m_stopBtn->setEnabled(false);
    }
}

void LogReaderTab::buildTable(QWidget* parent) {
    m_model = new LogTableModel(QStringList{"message"}, this);
    
    m_proxy = new MultiFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_table = new QTableView(parent);
    m_table->setModel(m_proxy);
    m_table->setSortingEnabled(true);
    m_table->setSelectionBehavior(QTableView::SelectItems);
    m_table->setSelectionMode(QTableView::ExtendedSelection);
    m_table->verticalHeader()->setVisible(false);

    auto* delegate = new FirstNewRowDelegate(m_model, m_table);
    m_table->setItemDelegate(delegate);

    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->horizontalHeader()->setStretchLastSection(true);

    static_cast<QVBoxLayout*>(layout())->addWidget(m_table, 1);

    connect(m_table->horizontalHeader(), &QHeaderView::sortIndicatorChanged,
            this, [delegate](int /*section*/, Qt::SortOrder /*order*/) {
                delegate->setEnabled(true);
            });

}

void LogReaderTab::buildFiltersPanel(QWidget* parent) {
    auto* panel = new QWidget(parent);
    auto* form  = new QFormLayout(panel);
    form->setContentsMargins(0, 6, 0, 6);
    form->setSpacing(6);

    // Column regex filters
    m_columnsContainer = new QWidget(panel);
    m_columnsForm      = new QFormLayout(m_columnsContainer);
    m_columnsForm->setContentsMargins(0,0,0,0);
    m_columnsForm->setSpacing(6);

    rebuildColumnFiltersForm(m_model->columns());
    form->addRow(m_columnsContainer);

    // Time range
    auto* gridW = new QWidget(panel);
    auto* grid  = new QGridLayout(gridW);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(6);

    auto* lblFrom = new QLabel("Asctime from:");
    auto* lblTo   = new QLabel("Asctime to:");

    m_cbUseFrom = new QCheckBox("Use From", gridW);
    m_cbUseTo   = new QCheckBox("Use To", gridW);

    m_fromDt = new QDateTimeEdit(gridW);
    m_toDt   = new QDateTimeEdit(gridW);
    m_fromDt->setCalendarPopup(true);
    m_toDt->setCalendarPopup(true);
    m_fromDt->setDisplayFormat(m_datefmtQt);
    m_toDt->setDisplayFormat(m_datefmtQt);

    m_noTimeNotice = new QLabel("No asctime parsed; time filter disabled", gridW);
    m_noTimeNotice->setStyleSheet("color:#e0a070;");
    m_noTimeNotice->setVisible(false);

    grid->addWidget(lblFrom, 0, 0, Qt::AlignRight|Qt::AlignVCenter);
    grid->addWidget(m_cbUseFrom, 0, 1);
    grid->addWidget(m_fromDt, 0, 2);
    grid->setColumnStretch(3, 1);

    grid->addWidget(lblTo, 1, 0, Qt::AlignRight|Qt::AlignVCenter);
    grid->addWidget(m_cbUseTo, 1, 1);
    grid->addWidget(m_toDt, 1, 2);
    grid->addWidget(m_noTimeNotice, 1, 3);

    form->addRow(gridW);

    // wire time range
    auto applyTime = [this]{
        std::optional<QDateTime> f, t;
        if (m_cbUseFrom->isChecked()) f = m_fromDt->dateTime();
        if (m_cbUseTo->isChecked())   t = m_toDt->dateTime();
        static_cast<MultiFilterProxy*>(m_proxy)->setTimeRange(f, t);
    };
    connect(m_cbUseFrom, &QCheckBox::toggled, this, [this, applyTime](bool){
        m_userSetFrom = true; applyTime();
    });
    connect(m_cbUseTo, &QCheckBox::toggled, this, [this, applyTime](bool){
        m_userSetTo = true; applyTime();
    });
    connect(m_fromDt, &QDateTimeEdit::dateTimeChanged, this, [this, applyTime](const QDateTime&){
        m_userSetFrom = true; applyTime();
    });
    connect(m_toDt, &QDateTimeEdit::dateTimeChanged, this, [this, applyTime](const QDateTime&){
        m_userSetTo = true; applyTime();
    });

    // --- Clear button ---
    auto* btnClear = new QPushButton("Clear filters", panel);
    connect(btnClear, &QPushButton::clicked, this, &LogReaderTab::clearAllFilters);
    form->addRow(btnClear);

    static_cast<QVBoxLayout*>(layout())->addWidget(panel, 0);

    updateTimeControlsEnabled();
}

void LogReaderTab::buildStatus(QWidget* parent) {
    auto* statusRow = new QHBoxLayout();
    statusRow->setContentsMargins(0,0,0,0);

    m_status = new QLabel("", parent);
    statusRow->addWidget(m_status);
    statusRow->addStretch(1);

    static_cast<QVBoxLayout*>(layout())->addLayout(statusRow);
}

void LogReaderTab::setStatus(const QString& text) {
    if (m_status) m_status->setText(text);
}

QDateTime LogReaderTab::parseAsctimeQt(const QString &s) const
{
    return QDateTime::fromString(s, m_datefmtQt);
}

void LogReaderTab::parseAndAddRow(const QString &raw) {
    auto parsed = LogParser::parseLine(raw, m_compiled);
    if (!parsed) {
        // Continuation of previous line => append to previous line
        QHash<QString, QString> last = m_model->rowDict(m_model->rowCount()-1);
        if (!last.isEmpty() && LogParser::isTracebackLine(raw, &last)) {
            m_model->updateLastRowMessage(raw);
        } else {
            // keep as message-only row
            QHash<QString, QString> dict;
            const QString key = m_model->columns().contains("message")
                                ? "message" : m_model->columns().isEmpty()
                                ? "message" : m_model->columns().last();
            dict.insert(key, raw);
            m_model->addRow(dict);
        }
    } else {
        // parse asctime if present
        QDateTime asdt;
        if (parsed->contains("asctime")) {
            asdt = parseAsctimeQt(parsed->value("asctime"));
            if (asdt.isValid()) {
                if (!m_minDt.isValid() || asdt < m_minDt) m_minDt = asdt;
                m_haveAnyDatetime = true;
                if (!m_maxDt.isValid() || asdt > m_maxDt) m_maxDt = asdt;
            }
        }
        m_model->addRow(*parsed, asdt);
    }
    maybeSeedTimeEditors();
    updateTimeControlsEnabled();
}

void LogReaderTab::rebuildColumnFiltersForm(const QStringList &cols) {
    while (m_columnsForm->rowCount() > 0)
        m_columnsForm->removeRow(0);

    m_showChecks.clear();
    m_filterEdits.clear();
    m_showChecks.reserve(cols.size());
    m_filterEdits.reserve(cols.size());

    for (int i = 0; i < cols.size(); ++i) {
        const QString& name = cols[i];

        auto* rowW = new QWidget(m_columnsContainer);
        auto* h    = new QHBoxLayout(rowW);
        h->setContentsMargins(0,0,0,0);
        h->setSpacing(6);

        auto* cbShow = new QCheckBox("Show", rowW);
        cbShow->setChecked(true);
        connect(cbShow, &QCheckBox::toggled, this, [this, i](bool on){
            // Toggle column visibility on the VIEW side
            m_table->setColumnHidden(i, !on);
        });
        m_showChecks.push_back(cbShow);

        auto* edit = new QLineEdit(rowW);
        edit->setPlaceholderText(QString("Regex for '%1'").arg(name));
        connect(edit, &QLineEdit::textChanged, this, [this, i, edit](const QString& text){
            // Validate regex; if invalid, color edit background red and clear filter
            if (text.isEmpty()) {
                edit->setStyleSheet("");
                static_cast<MultiFilterProxy*>(m_proxy)->setFilterRegexForColumn(i, std::nullopt);
                return;
            }
            QRegularExpression rx(text, QRegularExpression::CaseInsensitiveOption);
            if (!rx.isValid()) {
                edit->setStyleSheet("background: rgba(180,50,50,0.5);");
                static_cast<MultiFilterProxy*>(m_proxy)->setFilterRegexForColumn(i, std::nullopt);
            } else {
                edit->setStyleSheet("");
                static_cast<MultiFilterProxy*>(m_proxy)->setFilterRegexForColumn(i, rx);
            }
        });
        m_filterEdits.push_back(edit);

        h->addWidget(cbShow);
        h->addWidget(edit);

        m_columnsForm->addRow(new QLabel(name + ":"), rowW);
    }
}

void LogReaderTab::clearAllFilters() {
    for (auto* e : m_filterEdits) {
        e->blockSignals(true);
        e->clear();
        e->setStyleSheet("");
        e->blockSignals(false);
    }
    static_cast<MultiFilterProxy*>(m_proxy)->clearAllColumnFilters();

    m_cbUseFrom->setChecked(false);
    m_cbUseTo->setChecked(false);
    static_cast<MultiFilterProxy*>(m_proxy)->setTimeRange(std::nullopt, std::nullopt);

    setStatus("Filters cleared.");
}

void LogReaderTab::updateTimeControlsEnabled() {
    const bool have = m_haveAnyDatetime;
    m_cbUseFrom->setEnabled(have);
    m_cbUseTo->setEnabled(have);
    m_fromDt->setEnabled(have);
    m_toDt->setEnabled(have);
    m_noTimeNotice->setVisible(!have);
}

void LogReaderTab::maybeSeedTimeEditors() {
    if (m_haveAnyDatetime) {
        if (!m_userSetFrom && m_minDt.isValid()) {
            m_fromDt->blockSignals(true);
            m_fromDt->setDateTime(m_minDt);
            m_fromDt->blockSignals(false);
        }
        if (!m_userSetTo && m_maxDt.isValid()) {
            m_toDt->blockSignals(true);
            m_toDt->setDateTime(m_maxDt);
            m_toDt->blockSignals(false);
        }
    }
}

void LogReaderTab::rebuildModelCols(const QStringList &cols) {
    m_model->setColumns(cols.isEmpty() ? QStringList{"message"} : cols);

    // Resize: message stretch, others content
    auto* hdr = m_table->horizontalHeader();
    for (int i = 0; i < m_model->columnCount(); ++i) {
        const QString name = m_model->columns()[i];
        if (name == "message") hdr->setSectionResizeMode(i, QHeaderView::Stretch);
        else                   hdr->setSectionResizeMode(i, QHeaderView::ResizeToContents);

    }
    rebuildColumnFiltersForm(m_model->columns());
}

// ----- Slots -----
void LogReaderTab::onBrowse() {
    const QString path = QFileDialog::getOpenFileName(this, "Select log file", QString(), "Log files (*.log *.txt);;All files (*)");
    if (!path.isEmpty()) {
        m_pathEdit->setText(path);
    }
}

void LogReaderTab::onApplyFormat() {
    const QString mode = m_modeCombo->currentText().trimmed();
    const QString fmt  = m_formatEdit->text().trimmed();
    if (fmt.isEmpty()) { setStatus("Format empty."); return; }

    LogParser::ParseResult compiled;
    if (mode == "regex") compiled = LogParser::compileNamedRegex(fmt);
    else                 compiled = LogParser::compileFormatToRegex(fmt);

    if (!compiled.success) {
        setStatus(QStringLiteral("Format error: %1").arg(compiled.error));
        return;
    }
    m_compiled = compiled;

    const QString df = m_datefmtEdit->text().trimmed();
    if (!df.isEmpty()) m_datefmtQt = df;

    rebuildModelCols(m_compiled.columns);
    setStatus(QStringLiteral("Applied format. Columns: %1")
              .arg(m_compiled.columns.join(", ")));
    rebuildColumnFiltersForm(m_model->columns());
    m_haveAnyDatetime = false;
    m_minDt = {}; m_maxDt = {};
    m_userSetFrom = m_userSetTo = false;
    updateTimeControlsEnabled();
    static_cast<MultiFilterProxy*>(m_proxy)->setTimeRange(std::nullopt, std::nullopt);
    // Prefer sorting by asctime if present
    const int asCol = m_compiled.columns.indexOf("asctime");
    if (asCol >= 0) m_table->sortByColumn(asCol, Qt::AscendingOrder);
}


void LogReaderTab::onStart() {
    if (m_tailer) {
        setStatus("Already running.");
        return;
    }
    const QString path = m_pathEdit->text().trimmed();
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        setStatus("Please select an existing log file.");
        return;
    }

    // Start tailing
    m_tailer = new LogTailer(path, /*pollMillis*/200, this);
    connect(m_tailer, &LogTailer::newLine,   this, &LogReaderTab::onTailNewLine);
    connect(m_tailer, &LogTailer::fileError, this, &LogReaderTab::onTailFileError);
    m_tailer->start();

    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    setStatus(QStringLiteral("Tailing: %1").arg(path));
}

void LogReaderTab::onStop() {
    if (!m_tailer) {
        setStatus("Not running.");
        return;
    }
    m_tailer->stop();
    m_tailer->wait(2000);
    m_tailer->deleteLater();
    m_tailer = nullptr;

    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    setStatus("Stopped.");

}

void LogReaderTab::onClear() {
    auto* m = qobject_cast<QStandardItemModel*>(m_table->model());
    if (m) m->removeRows(0, m->rowCount());
    m_haveAnyDatetime = false;
    m_minDt = {}; m_maxDt = {};
    m_userSetFrom = m_userSetTo = false;
    updateTimeControlsEnabled();
    static_cast<MultiFilterProxy*>(m_proxy)->setTimeRange(std::nullopt, std::nullopt);
    setStatus("Cleared.");
}

void LogReaderTab::onReloadAll() {
    const QString path = m_pathEdit->text().trimmed();
    if (path.isEmpty() || !QFileInfo(path).isFile()) {
        setStatus(QStringLiteral("No such file: %1").arg(path.isEmpty() ? "<empty>" : path));
        return;
    }

    // Stop if running
    if (m_tailer) {
        onStop();
    }

    m_model->clear();

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStatus(QStringLiteral("Failed to open: %1").arg(path));
        return;
    }
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);

    int count = 0;
    while (!in.atEnd()) {
        const QString raw = in.readLine();
        if (raw.isNull()) break;
        parseAndAddRow(raw);
        ++count;
    }
    f.close();

    // After bulk load, mark all as seen
    updateLastSeenRow();

    // default sort
    int asCol = m_model->columns().indexOf("asctime");
    if (asCol >= 0) m_table->sortByColumn(asCol, Qt::AscendingOrder);

    setStatus(QStringLiteral("Loaded %1 lines from file.").arg(count));
}

void LogReaderTab::onPathTextChanged(const QString& text) {
    QFileInfo fi(text.trimmed());
    const QString base = fi.fileName().isEmpty() ? QStringLiteral("Untitled") : fi.fileName();
    emit pathChanged(base);
}

void LogReaderTab::onTailNewLine(const QString &line) {
    const QString raw = line.endsWith('\n') ? line.chopped(1) : line;
    parseAndAddRow(raw);
}

void LogReaderTab::onTailFileError(const QString& msg) {
    setStatus(QStringLiteral("Tail error: %1").arg(msg));

}
