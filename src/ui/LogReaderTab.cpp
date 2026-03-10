#include "LogReaderTab.h"

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
    setStatus("Start clicked");
}

void LogReaderTab::onStop() {
    setStatus("Stop clicked.");
}

void LogReaderTab::onClear() {
    auto* m = qobject_cast<QStandardItemModel*>(m_table->model());
    if (m) m->removeRows(0, m->rowCount());
    setStatus("Cleared.");
}

void LogReaderTab::onReloadAll() {
    setStatus("Reload All clicked");
}

void LogReaderTab::onPathTextChanged(const QString& text) {
    QFileInfo fi(text.trimmed());
    const QString base = fi.fileName().isEmpty() ? QStringLiteral("Untitled") : fi.fileName();
    emit pathChanged(base);
}