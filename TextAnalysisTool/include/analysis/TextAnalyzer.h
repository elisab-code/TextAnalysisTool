#pragma once

#include "model/AnalysisResult.h"
#include <QStringList>

class StopWordManager;

/**
 * Core analysis engine (single-pass / streaming).
 *
 * Provides raw statistics needed by the UI:
 *  - Word count (all tokens incl. stopwords) via addRawToken()
 *  - Stop/non-stop ratio via totalTokensAll() vs totalTokens()
 *  - Unigram frequencies (non-stop tokens)
 *  - Bigram frequencies (non-stop tokens, across line boundaries)
 *  - Word-length histogram (currently for all tokens incl. stopwords)
 */
class TextAnalyzer
{
public:
    explicit TextAnalyzer(const StopWordManager* stopWords = nullptr);

    /**
     * Consumes tokens incrementally and updates statistics in a single pass.
     */
    void consumeTokens(const QStringList& tokens);

    /**
     * Returns computed analysis result (reference to avoid copying).
     */
    const AnalysisResult& result() const;

private:
    const StopWordManager* m_stopWords = nullptr;  // optional dependency
    AnalysisResult m_result;                       // aggregated statistics

    // Used to build bigrams across line boundaries (non-stop tokens only).
    QString m_prevNonStopToken;
};