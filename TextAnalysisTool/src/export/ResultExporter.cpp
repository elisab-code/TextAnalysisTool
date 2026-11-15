#include "export/ResultExporter.h"
#include "analysis/StatisticCalculator.h"

#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

static void setErr(QString* err, const QString& msg)
{
    if (err) *err = msg;
}

/**
 * Writes a UTF-8 BOM to make sure common Windows editors
 * reliably detect UTF-8 and display umlauts (äöüß) correctly.
 */
static void writeUtf8BomIfNeeded(QFile& f)
{
    if (f.pos() == 0) {
        const char bom[] = { char(0xEF), char(0xBB), char(0xBF) };
        f.write(bom, 3);
    }
}

bool ResultExporter::exportReportTxt(
    const QString& outPath,
    const QString& sourceFile,
    const AnalysisResult& result,
    const StatisticCalculator& calc,
    int topK,
    QString* error
)
{
    QFile f(outPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        setErr(error, "Konnte Datei nicht schreiben: " + outPath);
        return false;
    }

    writeUtf8BomIfNeeded(f);

    QTextStream s(&f);
    s.setEncoding(QStringConverter::Utf8);

    const int allTokens = result.totalTokensAll();
    const int nonStop = result.totalTokens();
    const int stop = allTokens - nonStop;

    s << "TextAnalysisTool - Report\n";
    s << "Zeitpunkt: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    s << "Quelle: " << sourceFile << "\n\n";

    s << "Kennzahlen\n";
    s << "Zeichenanzahl: " << result.charCount() << "\n";
    s << "Wortanzahl (alle Tokens): " << allTokens << "\n";
    s << "Nicht-Stoppwörter: " << nonStop << "\n";
    s << "Stoppwörter: " << stop << "\n";
    s << "Unterschiedliche Wörter: " << calc.vocabularySize() << "\n";
    s << "Ø Wortlänge: " << QString::number(calc.averageWordLength(), 'f', 2) << "\n";
    s << "Median Wortlänge: " << QString::number(calc.medianWordLength(), 'f', 2) << "\n";
    s << "Lexikalische Dichte: " << QString::number(calc.lexicalDensity(), 'f', 3) << "\n\n";

    s << "Top-Wörter (K=" << topK << ")\n";
    for (const auto& p : calc.topKWords(topK)) {
        s << p.first << " : " << p.second << "\n";
    }

    s << "\nTop-Bigramme (K=" << topK << ")\n";
    for (const auto& p : calc.topKBigrams(topK)) {
        s << p.first << " : " << p.second << "\n";
    }

    s << "\nWortlängen-Histogramm (1..20)\n";
    const auto& hist = result.wordLengthHistogram();
    for (int len = 1; len <= 20; ++len) {
        const int c = hist.value(len, 0);
        if (c > 0) s << "len " << len << ": " << c << "\n";
    }

    return true;
}

bool ResultExporter::saveResultJson(
    const QString& outPath,
    const QString& sourceFile,
    const AnalysisResult& result,
    QString* error
)
{
    QJsonObject root;
    root["version"] = 1;
    root["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["sourceFile"] = sourceFile;

    // JSON has no 64-bit integers; double is exact up to 2^53.
    root["charCount"] = static_cast<double>(result.charCount());
    root["totalTokensAll"] = result.totalTokensAll();
    root["totalTokensNonStop"] = result.totalTokens();

    QJsonArray freqArr;
    for (auto it = result.frequencies().cbegin(); it != result.frequencies().cend(); ++it) {
        QJsonObject o;
        o["token"] = it.key();
        o["count"] = it.value();
        freqArr.append(o);
    }
    root["frequencies"] = freqArr;

    QJsonArray bigramArr;
    for (auto it = result.bigramFrequencies().cbegin(); it != result.bigramFrequencies().cend(); ++it) {
        QJsonObject o;
        o["bigram"] = it.key();
        o["count"] = it.value();
        bigramArr.append(o);
    }
    root["bigrams"] = bigramArr;

    QJsonArray histArr;
    for (auto it = result.wordLengthHistogram().cbegin(); it != result.wordLengthHistogram().cend(); ++it) {
        QJsonObject o;
        o["len"] = it.key();
        o["count"] = it.value();
        histArr.append(o);
    }
    root["lengthHistogram"] = histArr;

    QFile f(outPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        setErr(error, "Konnte JSON nicht schreiben: " + outPath);
        return false;
    }

    // QJsonDocument emits UTF-8.
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool ResultExporter::loadResultJson(
    const QString& inPath,
    QString* sourceFileOut,
    AnalysisResult* resultOut,
    QString* error
)
{
    if (!resultOut) {
        setErr(error, "resultOut ist null.");
        return false;
    }

    QFile f(inPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setErr(error, "Konnte JSON nicht lesen: " + inPath);
        return false;
    }

    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) {
        setErr(error, "Ungültiges JSON-Format.");
        return false;
    }

    const QJsonObject root = doc.object();
    const int version = root.value("version").toInt();
    if (version != 1) {
        setErr(error, "Nicht unterstützte Version: " + QString::number(version));
        return false;
    }

    if (sourceFileOut) *sourceFileOut = root.value("sourceFile").toString();

    AnalysisResult r;
    r.setCharCount(static_cast<qint64>(root.value("charCount").toDouble()));
    r.setTotalTokensAll(root.value("totalTokensAll").toInt());
    r.setTotalTokensNonStop(root.value("totalTokensNonStop").toInt());

    QHash<QString, int> freq;
    for (const auto& v : root.value("frequencies").toArray()) {
        const auto o = v.toObject();
        freq[o.value("token").toString()] = o.value("count").toInt();
    }
    r.setFrequencies(freq);

    QHash<QString, int> big;
    for (const auto& v : root.value("bigrams").toArray()) {
        const auto o = v.toObject();
        big[o.value("bigram").toString()] = o.value("count").toInt();
    }
    r.setBigramFrequencies(big);

    QHash<int, int> hist;
    for (const auto& v : root.value("lengthHistogram").toArray()) {
        const auto o = v.toObject();
        hist[o.value("len").toInt()] = o.value("count").toInt();
    }
    r.setWordLengthHistogram(hist);

    *resultOut = r;
    return true;
}