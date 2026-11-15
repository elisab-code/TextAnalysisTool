#include "application/AnalysisController.h"

#include "import/TextFileReader.h"
#include "import/TextParser.h"
#include "analysis/TextAnalyzer.h"
#include "analysis/StopWordManager.h"

#include <QDebug>
#include <QtGlobal>

// StopWordManager is injected from outside
AnalysisController::AnalysisController(const StopWordManager* stopWords)
    : m_stopWords(stopWords)
{
}

/**
 * Performs a streaming analysis of the given .txt file.
 *
 * Workflow:
 *  1) Open file (TextFileReader)
 *  2) Read line-by-line (no full file load)
 *  3) Parse each line into normalized tokens (TextParser)
 *  4) Feed tokens into TextAnalyzer (incremental statistics)
 *  5) Set charCount (sum of line lengths)
 *  6) Return the final AnalysisResult
 */
AnalysisResult AnalysisController::analyzeTxtFile(const QString& path)
{
    TextFileReader reader;              // File I/O abstraction
    TextParser parser;                  // Tokenization + normalization (line-based)
    TextAnalyzer analyzer(m_stopWords); // Analyzer uses stopwords if provided

    // Abort early if file cannot be opened
    if (!reader.open(path)) {
        qWarning() << "Failed to open file:" << path;
        return {}; // default/empty AnalysisResult
    }

    qint64 charCount = 0;

    // Stream-based processing: single pass over the input
    while (!reader.atEnd()) {
        const QString line = reader.readLine();

        // Character count: UTF-16 code units per line, line breaks excluded
        // (keeps the value independent of CRLF vs. LF).
        charCount += line.size();

        const QStringList tokens = parser.parseLine(line);
        analyzer.consumeTokens(tokens);
    }

    reader.close();

    // Copy result so we can attach additional derived metadata (charCount)
    AnalysisResult result = analyzer.result();
    result.setCharCount(charCount);

    // Debug output for development/testing
    qDebug() << "Total tokens (all):" << result.totalTokensAll();
    qDebug() << "Total tokens (non-stop):" << result.totalTokens();
    qDebug() << "Unique tokens:" << result.frequencies().size();
    qDebug() << "Char count:" << result.charCount();

    return result;
}
