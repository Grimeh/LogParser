#pragma once
#include <QThread>
#include <QAtomicInteger>
#include <QString>

class LogTailer : public QThread {
    Q_OBJECT
public:
    explicit LogTailer(const QString& path,
                       int pollMillis = 200,
                       QObject* parent = nullptr);
    ~LogTailer() override;

    void stop();

signals:
    void newLine(const QString& line);
    void fileError(const QString& message);

protected:
    void run() override;

private:
    QString m_path;
    int     m_pollMs;
    QAtomicInteger<bool> m_running { false };
};