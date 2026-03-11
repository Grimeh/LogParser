#include "LogTailer.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QThread>

LogTailer::LogTailer(const QString& path, int pollMillis, QObject* parent)
    : QThread(parent)
    , m_path(path)
    , m_pollMs(pollMillis)
{
}

LogTailer::~LogTailer() {
    stop();
    wait(2000);
}

void LogTailer::stop() {
    m_running.storeRelease(false);
}

void LogTailer::run() {
    m_running.storeRelease(true);

    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit fileError(QStringLiteral("Failed to open file: %1").arg(m_path));
        return;
    }

    // Initially seek end of file
    if (!file.seek(file.size())) {
        emit fileError(QStringLiteral("Failed to seek to end: %1").arg(m_path));
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    qint64 lastPos = file.pos();

    while (m_running.loadAcquire()) {
        // handle truncation (file shrank)
        const qint64 sizeNow = file.size();
        if (sizeNow < lastPos) {
            // reopen from start
            file.close();
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                emit fileError(QStringLiteral("Reopen failed (after truncation): %1").arg(m_path));
                QThread::msleep(m_pollMs);
                continue;
            }
            in.setDevice(&file);
            in.setEncoding(QStringConverter::Utf8);
            lastPos = 0;
        }

        bool hadAny = false;
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (line.isNull()) break;
            hadAny = true;
            lastPos = file.pos();
            emit newLine(line + QLatin1Char('\n'));
            if (!m_running.loadAcquire()) break;
        }

        if (!hadAny) {
            QThread::msleep(m_pollMs);
            lastPos = file.pos();
        }
    }

    file.close();
}