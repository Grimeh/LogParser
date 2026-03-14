#include "LogParser.h"
#include <QVector>
#include <QPair>

namespace {

QRegularExpression tokenRe(R"(%\(([^)]+)\)s)");  // %(name)s

// Extract named group names in order of appearance
QStringList orderedNamedGroups(const QRegularExpression& re) {
    QStringList groups = re.namedCaptureGroups();
    if (!groups.isEmpty()) groups.removeFirst(); // remove whole match
    return groups;
}

}

namespace LogParser {

ParseResult compileFormatToRegex(const QString& fmt) {
    ParseResult r;
    if (fmt.isEmpty()) {
        r.success = false; r.error = "Empty format.";
        return r;
    }
    // Extract group names
    QVector<QPair<QString, bool>> parts; // (text, isToken)
    int last = 0;
    auto it = tokenRe.globalMatch(fmt);
    QStringList columns;
    int idx = 0;
    while (it.hasNext()) {
        auto m = it.next();
        const int start = m.capturedStart();
        if (start > last) {
            parts.push_back({fmt.mid(last, start - last), false});
        }
        const QString name = m.captured(1);
        columns.push_back(name);
        parts.push_back({name, true});
        last = m.capturedEnd();
        ++idx;
    }
    if (idx == 0) {
        r.success = false; r.error = "No %(...)s tokens found in format.";
        return r;
    }
    if (last < fmt.size()) {
        parts.push_back({fmt.mid(last), false});
    }

    // Compose pattern string
    QString pattern("^");
    int tokenCount = 0;
    for (const auto& p : parts) if (p.second) ++tokenCount;

    int seenTokens = 0;
    for (const auto& p : parts) {
        if (!p.second) {
            pattern += QRegularExpression::escape(p.first);
        } else {
            ++seenTokens;
            const bool isLastToken = (seenTokens == tokenCount);
            pattern += QString("(?<%1>%2)")
                       .arg(p.first,
                            isLastToken ? ".*" : ".*?");
        }
    }
    pattern += "$";

    // Compile regex
    QRegularExpression re(pattern);
    if (!re.isValid()) {
        r.success = false; r.error = QString("Regex compile error: %1").arg(re.errorString());
        return r;
    }
    r.success = true;
    r.columns = columns;
    r.regex = re;
    return r;
}

ParseResult compileNamedRegex(const QString& pattern) {
    ParseResult r;
    if (pattern.isEmpty()) {
        r.success = false; r.error = "Empty regex.";
        return r;
    }
    // Ensure anchored, keep flags as-is
    QString anchored = pattern;
    if (!anchored.startsWith('^')) anchored.prepend('^');
    if (!anchored.endsWith('$')) anchored.append('$');
    QRegularExpression re(anchored);
    if (!re.isValid()) {
        r.success = false; r.error = QString("Regex compile error: %1").arg(re.errorString());
        return r;
    }
    QStringList cols = orderedNamedGroups(re);
    if (cols.isEmpty()) {
        // no groups => capture whole line as message
        QRegularExpression alt(QString("^(?<message>%1)$").arg(pattern));
        if (!alt.isValid()) {
            r.success = false; r.error = QString("Regex compile error: %1").arg(alt.errorString());
            return r;
        }
        r.regex = alt;
        r.columns = {"message"};
        r.success = true;
        return r;
    }
    r.regex   = re;
    r.columns = cols;
    r.success = true;
    return r;
}

std::optional<QHash<QString, QString>>
parseLine(const QString& line, const ParseResult& compiled) {
    if (!compiled.success || !compiled.regex.isValid())
        return std::nullopt;

    const auto m = compiled.regex.match(line);
    if (!m.hasMatch())
        return std::nullopt;

    QHash<QString, QString> dict;
    for (const auto& c : compiled.columns) {
        dict.insert(c, m.captured(c));
    }
    return dict;
}

// ---- traceback ----

static QRegularExpression RE_TRACEBACK_START(R"(^(?:Traceback \(most recent call last\):))");
static QRegularExpression RE_FILE_LINE(R"(^\s*File ".*", line \d+, in .+)");
static QRegularExpression RE_CAUSE_LINE(R"(^During handling of the above exception.*)");
static QRegularExpression RE_EXCEPTION_LINE(R"(^\s*[A-Za-z_][\w.]*: .+)");

bool isTracebackLine(const QString& text,
                     const QHash<QString, QString>* lastRowDict) {
    const QString t = text;
    if (RE_TRACEBACK_START.match(t).hasMatch() ||
        RE_FILE_LINE.match(t).hasMatch()       ||
        RE_CAUSE_LINE.match(t).hasMatch()      ||
        RE_EXCEPTION_LINE.match(t).hasMatch())
        return true;

    if (t.startsWith(' ') || t.startsWith('\t')) {
        if (!lastRowDict) return false;
        const QString lvl = lastRowDict->value("levelname",
                           lastRowDict->value("level")).toUpper();
        const QString prevMsg = lastRowDict->value("message");
        return (lvl == "ERROR" || lvl == "CRITICAL" || prevMsg.endsWith(':'));
    }
    return false;
}

} // namespace LogParser