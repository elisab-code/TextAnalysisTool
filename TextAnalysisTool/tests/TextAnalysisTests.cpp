#include <QtTest/QtTest>

#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStringConverter>

#include "import/TextParser.h"
#include "analysis/StopWordManager.h"
#include "application/AnalysisController.h"
#include "analysis/StatisticCalculator.h"
#include "export/ResultExporter.h"

// QtTest-based test suite for the core text analysis functionality.
// These tests cover tokenization, stopword handling, counting logic,
// derived statistics, and export/import behavior.
class TextAnalysisTests : public QObject
{
    Q_OBJECT

private:
    // Helper: write a UTF-8 text file inside a temporary directory.
    // Returns the full path of the written file or an empty string on failure.
    static QString writeUtf8File(QTemporaryDir& dir, const QString& fileName, const QString& content)
    {
        const QString path = dir.path() + "/" + fileName;
        QFile f(path);

        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return QString(); // caller will QVERIFY(!isEmpty())
        }

        QTextStream s(&f);
        s.setEncoding(QStringConverter::Utf8);
        s << content;
        f.close();

        return path;
    }

private slots:
    // Tokenization tests
    void tokenization_basic();
    void tokenization_umlauts();

    // Stopword loading and lookup
    void stopwords_load_and_lookup();

    // End-to-end analysis tests
    void analysis_counts_no_stopwords();
    void analysis_counts_with_stopwords();
    void bigrams_across_line_boundaries();

    // Derived metric validation
    void calculator_metrics();

    // Export/import validation
    void exporter_json_roundtrip();
    void exporter_txt_report_created();
};

// Verifies that the parser lowercases tokens and preserves valid internal punctuation
// such as apostrophes and hyphens.
void TextAnalysisTests::tokenization_basic()
{
    // Tokenizer should lowercase and keep internal apostrophes/hyphens
    TextParser p;
    const QStringList tokens = p.parseLine("Hello, world! don't e-mail 42");
    const QStringList expected{ "hello", "world", "don't", "e-mail", "42" };
    QCOMPARE(tokens, expected);
}

// Verifies that Unicode letters such as German umlauts are tokenized correctly
// and converted to lowercase.
void TextAnalysisTests::tokenization_umlauts()
{
    // Unicode letters should be tokenized and lowercased
    TextParser p;
    const QString input = QString("\u00C4\u00D6\u00DC\u00DF");   // ÄÖÜß
    const QStringList tokens = p.parseLine(input);
    const QStringList expected{ QString("\u00E4\u00F6\u00FC\u00DF") }; // äöüß
    QCOMPARE(tokens, expected);
}

// Verifies loading of stopwords from a UTF-8 file, including comment handling,
// trimming, lowercasing, and lookup behavior.
void TextAnalysisTests::stopwords_load_and_lookup()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "Failed to create QTemporaryDir.");

    // Stopword file: includes comments and mixed case
    const QString swPath = writeUtf8File(
        dir,
        "stopwords.txt",
        "# comment line\nDer\ndie\n  DAS \n\n"
    );
    QVERIFY2(!swPath.isEmpty(), "Failed to write stopwords temp file.");

    StopWordManager sw;
    QVERIFY(sw.loadFromFile(swPath));

    // StopWordManager stores lowercased entries; input is expected to be normalized by the parser
    QVERIFY(sw.isStopWord("der"));
    QVERIFY(sw.isStopWord("die"));
    QVERIFY(sw.isStopWord("das"));

    // Not found
    QVERIFY(!sw.isStopWord("haus"));

    // Case-sensitive lookup (by design)
    QVERIFY(!sw.isStopWord("Die"));
}

// Verifies token counts, word frequencies, bigrams, and character count
// when no stopword filter is active.
void TextAnalysisTests::analysis_counts_no_stopwords()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "Failed to create QTemporaryDir.");

    // One line: "a a b" (newline excluded by readLine)
    const QString txtPath = writeUtf8File(dir, "sample.txt", "a a b\n");
    QVERIFY2(!txtPath.isEmpty(), "Failed to write sample temp file.");

    AnalysisController controller(nullptr);
    const AnalysisResult r = controller.analyzeTxtFile(txtPath);

    QCOMPARE(r.totalTokensAll(), 3);
    QCOMPARE(r.totalTokens(), 3);

    QCOMPARE(r.frequencies().value("a", 0), 2);
    QCOMPARE(r.frequencies().value("b", 0), 1);

    // Bigrams: "a a" and "a b"
    QCOMPARE(r.bigramFrequencies().value("a a", 0), 1);
    QCOMPARE(r.bigramFrequencies().value("a b", 0), 1);

    // Char count counts the line without newline: "a a b" has length 5
    QCOMPARE(r.charCount(), static_cast<qint64>(5));
}

// Verifies that stopwords are still counted in totalTokensAll(),
// but excluded from the filtered token frequency map and derived bigrams.
void TextAnalysisTests::analysis_counts_with_stopwords()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "Failed to create QTemporaryDir.");

    const QString txtPath = writeUtf8File(dir, "sample.txt", "a a b\n");
    QVERIFY2(!txtPath.isEmpty(), "Failed to write sample temp file.");

    const QString swPath = writeUtf8File(dir, "stopwords.txt", "a\n");
    QVERIFY2(!swPath.isEmpty(), "Failed to write stopwords temp file.");

    StopWordManager sw;
    QVERIFY(sw.loadFromFile(swPath));

    AnalysisController controller(&sw);
    const AnalysisResult r = controller.analyzeTxtFile(txtPath);

    // All tokens still counted for density
    QCOMPARE(r.totalTokensAll(), 3);

    // Non-stop tokens only: "b"
    QCOMPARE(r.totalTokens(), 1);
    QCOMPARE(r.frequencies().contains("a"), false);
    QCOMPARE(r.frequencies().value("b", 0), 1);

    // No bigrams possible with only one non-stop token
    QCOMPARE(r.bigramFrequencies().size(), 0);

    // Same character count
    QCOMPARE(r.charCount(), static_cast<qint64>(5));
}

// Verifies that bigrams are formed across line boundaries and not reset at each new line.
void TextAnalysisTests::bigrams_across_line_boundaries()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "Failed to create QTemporaryDir.");

    // Two lines: "a b" and "c" -> bigram "b c" should be counted across line boundary
    const QString txtPath = writeUtf8File(dir, "sample.txt", "a b\nc\n");
    QVERIFY2(!txtPath.isEmpty(), "Failed to write sample temp file.");

    AnalysisController controller(nullptr);
    const AnalysisResult r = controller.analyzeTxtFile(txtPath);

    QCOMPARE(r.totalTokensAll(), 3);
    QCOMPARE(r.totalTokens(), 3);

    QCOMPARE(r.bigramFrequencies().value("a b", 0), 1);
    QCOMPARE(r.bigramFrequencies().value("b c", 0), 1);

    // Char count: "a b" length 3 + "c" length 1 = 4
    QCOMPARE(r.charCount(), static_cast<qint64>(4));
}

// Verifies derived statistics such as average word length, median word length,
// lexical density, and vocabulary size using a small synthetic result object.
void TextAnalysisTests::calculator_metrics()
{
    // Build a small synthetic result to validate derived metrics
    AnalysisResult r;

    // Three tokens total, but only two are counted as non-stop
    r.addRawToken("a");   r.addWordLength(1);
    r.addRawToken("aa");  r.addWordLength(2);
    r.addRawToken("bbb"); r.addWordLength(3);

    // Non-stop tokens: "aa", "bbb"
    r.addToken("aa");
    r.addToken("bbb");

    StatisticCalculator calc(r);

    // Histogram lengths: 1,2,3 => average is 2.0, median is 2.0
    QCOMPARE(QString::number(calc.averageWordLength(), 'f', 2), QString("2.00"));
    QCOMPARE(QString::number(calc.medianWordLength(), 'f', 2), QString("2.00"));

    // Lexical density = non-stop / all = 2/3 ≈ 0.666...
    QVERIFY(calc.lexicalDensity() > 0.66 && calc.lexicalDensity() < 0.67);

    // Vocabulary size should match non-stop frequency map size (2)
    QCOMPARE(calc.vocabularySize(), 2);
}

// Verifies that an AnalysisResult can be serialized to JSON and loaded back
// without losing the essential analysis data.
void TextAnalysisTests::exporter_json_roundtrip()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "Failed to create QTemporaryDir.");

    const QString txtPath = writeUtf8File(dir, "sample.txt", "a a b\n");
    QVERIFY2(!txtPath.isEmpty(), "Failed to write sample temp file.");

    AnalysisController controller(nullptr);
    const AnalysisResult original = controller.analyzeTxtFile(txtPath);

    const QString jsonPath = dir.path() + "/session.json";

    QString err;
    QVERIFY2(ResultExporter::saveResultJson(jsonPath, txtPath, original, &err),
        qPrintable(err.isEmpty() ? QString("saveResultJson failed.") : err));

    QString loadedSource;
    AnalysisResult loaded;
    QVERIFY2(ResultExporter::loadResultJson(jsonPath, &loadedSource, &loaded, &err),
        qPrintable(err.isEmpty() ? QString("loadResultJson failed.") : err));

    QCOMPARE(loadedSource, txtPath);
    QCOMPARE(loaded.charCount(), original.charCount());
    QCOMPARE(loaded.totalTokensAll(), original.totalTokensAll());
    QCOMPARE(loaded.totalTokens(), original.totalTokens());

    QCOMPARE(loaded.frequencies().value("a", 0), 2);
    QCOMPARE(loaded.frequencies().value("b", 0), 1);
}

// Verifies that a plain-text report file is created successfully
// and contains expected section headers/content.
void TextAnalysisTests::exporter_txt_report_created()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "Failed to create QTemporaryDir.");

    const QString txtPath = writeUtf8File(dir, "sample.txt", "a a b\n");
    QVERIFY2(!txtPath.isEmpty(), "Failed to write sample temp file.");

    AnalysisController controller(nullptr);
    const AnalysisResult r = controller.analyzeTxtFile(txtPath);
    StatisticCalculator calc(r);

    const QString outPath = dir.path() + "/report.txt";
    QString err;

    QVERIFY2(ResultExporter::exportReportTxt(outPath, txtPath, r, calc, 10, &err),
        qPrintable(err.isEmpty() ? QString("exportReportTxt failed.") : err));
    QVERIFY(QFileInfo::exists(outPath));

    QFile f(outPath);
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString content = QString::fromUtf8(f.readAll());
    QVERIFY(content.contains("TextAnalysisTool - Report"));
    QVERIFY(content.contains("Kennzahlen"));
    QVERIFY(content.contains("Zeichenanzahl"));
}

// Generates the test application's main function and executes all test slots above.
QTEST_MAIN(TextAnalysisTests)
#include "TextAnalysisTests.moc"