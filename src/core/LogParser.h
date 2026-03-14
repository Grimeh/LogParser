#pragma once
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QHash>
#include <optional>

namespace LogParser {

struct ParseResult {
    bool success = false;
    QStringList columns;
    QRegularExpression regex;
    QString error;
};


// Convert Python logging format string (e.g. "%(asctime)s %(levelname)s ... %(message)s")
// to a single regex with named groups. All tokens except the last use ".*?";
// the last token is greedy ".*".
ParseResult compileFormatToRegex(const QString& fmt);

// Compile a named group regex. If no named groups exist, treat whole line as 'message'.
// Ensures ^...$ anchors.
ParseResult compileNamedRegex(const QString& pattern);

// Try to parse the line with compiled result; returns key->value map on success.
std::optional<QHash<QString, QString>> parseLine(const QString& line, const ParseResult& compiled);

// Heuristics to decide whether a line is traceback/continuation of previous error row.
bool isTracebackLine(const QString& text,
                     const QHash<QString, QString>* lastRowDict);

} // namespace LogParser