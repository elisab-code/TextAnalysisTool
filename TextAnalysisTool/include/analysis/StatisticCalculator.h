#pragma once

#include <QVector>
#include <QPair>
#include <QString>

class AnalysisResult;

// Computes derived metrics from AnalysisResult (Read-only)
class StatisticCalculator
{
public:
    explicit StatisticCalculator(const AnalysisResult& result);

    // Derived word-length statistics (based on wordLengthHistogram()).
    double averageWordLength() const;   // mean length
    double medianWordLength() const;    // median length

    // Lexical density = non-stop tokens / all tokens
    // Requires AnalysisResult::totalTokensAll()
    double lexicalDensity() const;

    // number of unique (non-stop) tokens
    int vocabularySize() const;         

    // Top-K lists (descending by frequency)
    QVector<QPair<QString, int>> topKWords(int k) const;
    QVector<QPair<QString, int>> topKBigrams(int k) const;

private:
    const AnalysisResult& m_r;

    static QVector<QPair<QString, int>> topKFromHash(const QHash<QString, int>& map, int k);
};
