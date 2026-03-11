#include "LogReaderTab.h"
#include "core/LogTailer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>

#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QLabel>
#include <QFileInfo>

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
    m_table = new QTableView(parent);
    m_table->setSortingEnabled(true);
    m_table->setSelectionBehavior(QTableView::SelectItems);
    m_table->setSelectionMode(QTableView::ExtendedSelection);
    m_table->verticalHeader()->setVisible(false);

    auto* model = new QStandardItemModel(m_table);
    model->setHorizontalHeaderLabels({"message"});
    m_table->setModel(model);

    m_table->horizontalHeader()->setStretchLastSection(true);

    static_cast<QVBoxLayout*>(layout())->addWidget(m_table, 1);
}

void LogReaderTab::buildFiltersPanel(QWidget* parent) {
    auto* panel = new QWidget(parent);
    auto* form  = new QFormLayout(panel);
    form->setContentsMargins(0,6,0,6);
    form->setSpacing(6);

    auto* notice = new QLabel("Filters panel.", panel);
    notice->setStyleSheet("color:#a0a0a0;");
    form->addRow(notice);

    static_cast<QVBoxLayout*>(layout())->addWidget(panel, 0);
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

// ----- Slots -----
void LogReaderTab::onBrowse() {
    const QString path = QFileDialog::getOpenFileName(this, "Select log file", QString(), "Log files (*.log *.txt);;All files (*)");
    if (!path.isEmpty()) {
        m_pathEdit->setText(path);
    }
}

void LogReaderTab::onApplyFormat() {
    setStatus(QString("Mode: %1 | Format applied.").arg(m_modeCombo->currentText()));
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

    auto* model = qobject_cast<QStandardItemModel*>(m_table->model());
    if (!model) return;

    model->removeRows(0, model->rowCount());

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStatus(QStringLiteral("Failed to open: %1").arg(path));
        return;
    }
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);

    int count = 0;
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.isNull()) break;
        QList<QStandardItem*> row;
        row << new QStandardItem(line);
        model->appendRow(row);
        ++count;
    }
    f.close();

    setStatus(QStringLiteral("Loaded %1 lines from file.").arg(count));
}

void LogReaderTab::onPathTextChanged(const QString& text) {
    QFileInfo fi(text.trimmed());
    const QString base = fi.fileName().isEmpty() ? QStringLiteral("Untitled") : fi.fileName();
    emit pathChanged(base);
}

void LogReaderTab::onTailNewLine(const QString &line) {
    auto* model = qobject_cast<QStandardItemModel*>(m_table->model());
    if (!model) return;

    QList<QStandardItem*> row;
    const QString trimmed = (line.endsWith('\n') ? line.left(line.size()-1) : line);
    row << new QStandardItem(trimmed);
    model->appendRow(row);
}

void LogReaderTab::onTailFileError(const QString& msg) {
    setStatus(QStringLiteral("Tail error: %1").arg(msg));

}
