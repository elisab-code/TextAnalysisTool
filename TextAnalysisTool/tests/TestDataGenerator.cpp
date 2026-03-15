#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStringConverter>
#include <QStringList>
#include <QHash>
#include <QRandomGenerator>
#include <QDateTime>
#include <QCoreApplication>

/*
    TestDataGenerator.cpp

    Purpose:
    - Generate deterministic or random test text files for:
        (a) performance tests (target file size in bytes/MB)
        (b) correctness tests (exact token counts with expected frequencies)

    Design goals:
    - Streaming write (no huge in-memory strings)
    - Deterministic output option (cycle mode)
    - Optional expected frequency map output for validation
*/

namespace TestDataGenerator
{
    struct ResultInfo
    {
        qint64 bytesWritten = 0;
        qint64 tokensWritten = 0;
        QHash<QString, int> expectedTokenCounts; // counts of generated tokens
    };

    enum class Mode
    {
        Cycle,  // deterministic: vocab[i % vocabSize]
        Random  // pseudo-random: vocab[randomIndex]
    };

    static QString lastError;

    static QString errorString()
    {
        return lastError;
    }

    // Generate a file with approximately targetBytes size.
    // It will stop before exceeding the target (best-effort), so the final size is <= targetBytes.
    static bool generateBySizeBytes(
        const QString& outPath,
        qint64 targetBytes,
        const QStringList& vocabulary,
        int tokensPerLine,
        Mode mode,
        quint32 seed,
        ResultInfo* outInfo = nullptr
    )
    {
        lastError.clear();

        if (targetBytes <= 0) {
            lastError = "targetBytes must be > 0";
            return false;
        }
        if (vocabulary.isEmpty()) {
            lastError = "vocabulary must not be empty";
            return false;
        }
        if (tokensPerLine <= 0) {
            lastError = "tokensPerLine must be > 0";
            return false;
        }

        QFile f(outPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            lastError = "Failed to open output file: " + outPath;
            return false;
        }

        QTextStream s(&f);
        s.setEncoding(QStringConverter::Utf8);

        QRandomGenerator rng(seed);

        ResultInfo info;
        int tokenIndex = 0;
        int col = 0;

        // Write tokens until we reach the target size.
        // We try to avoid exceeding the target by estimating the UTF-8 byte length of the next token + separator.
        while (true) {
            const QString token =
                (mode == Mode::Cycle)
                ? vocabulary.at(tokenIndex % vocabulary.size())
                : vocabulary.at(static_cast<int>(rng.bounded(static_cast<quint32>(vocabulary.size()))));

            // Separator: space or newline
            const QString sep = (col + 1 >= tokensPerLine) ? "\n" : " ";

            const int tokenBytes = token.toUtf8().size();
            const int sepBytes = sep.toUtf8().size();
            const qint64 posBytes = f.pos();

            // Stop if writing the next token would exceed the target.
            if (posBytes + tokenBytes + sepBytes > targetBytes) {
                break;
            }

            s << token << sep;

            info.tokensWritten++;
            info.expectedTokenCounts[token]++;

            col = (col + 1) % tokensPerLine;
            tokenIndex++;

            // Periodically flush to keep QFile::pos reasonably up-to-date for very large files.
            if ((info.tokensWritten % 100000) == 0) {
                s.flush();
            }
        }

        s.flush();
        info.bytesWritten = f.size();
        f.close();

        if (outInfo) *outInfo = info;
        return true;
    }

    // Convenience: generate by MB
    static bool generateBySizeMB(
        const QString& outPath,
        int sizeMB,
        const QStringList& vocabulary,
        int tokensPerLine,
        Mode mode,
        quint32 seed,
        ResultInfo* outInfo = nullptr
    )
    {
        if (sizeMB <= 0) {
            lastError = "sizeMB must be > 0";
            return false;
        }
        const qint64 bytes = static_cast<qint64>(sizeMB) * 1024LL * 1024LL;
        return generateBySizeBytes(outPath, bytes, vocabulary, tokensPerLine, mode, seed, outInfo);
    }

    // Generate a file with an exact number of tokens (useful for correctness tests).
    static bool generateByTokenCount(
        const QString& outPath,
        qint64 tokenCount,
        const QStringList& vocabulary,
        int tokensPerLine,
        Mode mode,
        quint32 seed,
        ResultInfo* outInfo = nullptr
    )
    {
        lastError.clear();

        if (tokenCount <= 0) {
            lastError = "tokenCount must be > 0";
            return false;
        }
        if (vocabulary.isEmpty()) {
            lastError = "vocabulary must not be empty";
            return false;
        }
        if (tokensPerLine <= 0) {
            lastError = "tokensPerLine must be > 0";
            return false;
        }

        QFile f(outPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            lastError = "Failed to open output file: " + outPath;
            return false;
        }

        QTextStream s(&f);
        s.setEncoding(QStringConverter::Utf8);

        QRandomGenerator rng(seed);

        ResultInfo info;
        info.tokensWritten = 0;

        int tokenIndex = 0;
        int col = 0;

        while (info.tokensWritten < tokenCount) {
            const QString token =
                (mode == Mode::Cycle)
                ? vocabulary.at(tokenIndex % vocabulary.size())
                : vocabulary.at(static_cast<int>(rng.bounded(static_cast<quint32>(vocabulary.size()))));

            const QString sep = (col + 1 >= tokensPerLine) ? "\n" : " ";

            s << token << sep;

            info.tokensWritten++;
            info.expectedTokenCounts[token]++;

            col = (col + 1) % tokensPerLine;
            tokenIndex++;

            if ((info.tokensWritten % 100000) == 0) {
                s.flush();
            }
        }

        s.flush();
        info.bytesWritten = f.size();
        f.close();

        if (outInfo) *outInfo = info;
        return true;
    }
} // namespace TestDataGenerator


// ---- Optional CLI tool -----------------------------------------------------
// If you want a standalone generator executable, compile this file with
// -DTEXTANALYSIS_GENERATOR_MAIN and add a CMake target for it.

#ifdef TEXTANALYSIS_GENERATOR_MAIN
#include <QCommandLineParser>
#include <QCommandLineOption>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("TestDataGenerator");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Generate deterministic/random test text files for TextAnalysisTool.");
    parser.addHelpOption();
    parser.addVersionOption(); // uses -v / --version -> do NOT reuse -v for other options

    QCommandLineOption outOpt(QStringList() << "o" << "out", "Output file path", "path");
    QCommandLineOption mbOpt(QStringList() << "m" << "mb", "Target size in MB", "mb");
    QCommandLineOption tokensOpt(QStringList() << "t" << "tokens", "Exact token count", "count");

    // Use "-w" as short option and keep "--vocab" as long option.
    QCommandLineOption vocabOpt(QStringList() << "w" << "vocab", "Comma-separated vocabulary list", "list");

    QCommandLineOption modeOpt(QStringList() << "mode", "Mode: cycle|random", "mode", "cycle");
    QCommandLineOption lineOpt(QStringList() << "line", "Tokens per line (newline after N tokens)", "n", "20");
    QCommandLineOption seedOpt(QStringList() << "seed", "Random seed (for random mode)", "seed", "42");

    parser.addOption(outOpt);
    parser.addOption(mbOpt);
    parser.addOption(tokensOpt);
    parser.addOption(vocabOpt);
    parser.addOption(modeOpt);
    parser.addOption(lineOpt);
    parser.addOption(seedOpt);

    parser.process(app);

    const QString outPath = parser.value(outOpt);
    if (outPath.isEmpty()) {
        qCritical("Missing --out");
        return 2;
    }

    const QString vocabStr = parser.value(vocabOpt);
    if (vocabStr.isEmpty()) {
        qCritical("Missing --vocab (or -w)");
        return 2;
    }

    const QStringList vocab = vocabStr.split(',', Qt::SkipEmptyParts);
    const int tokensPerLine = parser.value(lineOpt).toInt();
    const quint32 seed = parser.value(seedOpt).toUInt();

    const QString modeStr = parser.value(modeOpt).toLower();
    const auto mode = (modeStr == "random") ? TestDataGenerator::Mode::Random : TestDataGenerator::Mode::Cycle;

    TestDataGenerator::ResultInfo info;

    if (!parser.value(mbOpt).isEmpty()) {
        const int mb = parser.value(mbOpt).toInt();
        if (!TestDataGenerator::generateBySizeMB(outPath, mb, vocab, tokensPerLine, mode, seed, &info)) {
            qCritical("%s", qPrintable(TestDataGenerator::errorString()));
            return 1;
        }
    }
    else if (!parser.value(tokensOpt).isEmpty()) {
        const qint64 t = parser.value(tokensOpt).toLongLong();
        if (!TestDataGenerator::generateByTokenCount(outPath, t, vocab, tokensPerLine, mode, seed, &info)) {
            qCritical("%s", qPrintable(TestDataGenerator::errorString()));
            return 1;
        }
    }
    else {
        qCritical("Specify either --mb or --tokens");
        return 2;
    }

    qInfo("Done. bytes=%lld tokens=%lld unique=%d",
        static_cast<long long>(info.bytesWritten),
        static_cast<long long>(info.tokensWritten),
        info.expectedTokenCounts.size());

    return 0;
}
#endif