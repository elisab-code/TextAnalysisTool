#pragma once

#include <QHash>
#include <QString>
#include <QMetaType>
#include <QtGlobal>

/**
 * Stores all computed statistical results of the analysis.
 *
 * Design:
 *  - Streaming-friendly: Analyzer updates counts incrementally.
 *  - Distinguishes between:
 *      (1) all tokens (incl. stopwords)  -> for lexical density / ratios
 *      (2) non-stop tokens only          -> for unigram/bigram stats
 *
 * Safety/robustness:
 *  - Enforces hard limits for UNIQUE tokens/bigrams to prevent memory explosions
 *    on pathological inputs.
 *  - Existing keys are still counted; only *new* keys are rejected once the limit is hit.
 */
class AnalysisResult
{
public:
    // Hard limits for unique keys (memory safety).
    static constexpr int kMaxUniqueTokens = 1'000'000;
    static constexpr int kMaxUniqueBigrams = 1'000'000;

    // Counts every token (including stopwords), does not store frequencies.
    void addRawToken(const QString& token);

    // Counts only non-stopwords and updates unigram frequencies.
    void addToken(const QString& token);

    // Adds a bigram consisting of two consecutive non-stop tokens.
    void addBigram(const QString& a, const QString& b);

    // Updates histogram for word lengths.
    void addWordLength(int len);

    // Non-stop token count (after stopword filtering).
    int totalTokens() const;

    // Token count including stopwords.
    int totalTokensAll() const;

    // Character count of the processed document (sum of line lengths).
    void setCharCount(qint64 n);
    qint64 charCount() const;

    // Limit flags (true if at least one new key had to be dropped due to limits).
    bool tokenLimitReached() const { return m_tokenLimitReached; }
    bool bigramLimitReached() const { return m_bigramLimitReached; }

    // Setters for JSON load (read-only pipeline remains unchanged).
    void setTotalTokensAll(int n);
    void setTotalTokensNonStop(int n);
    void setFrequencies(const QHash<QString, int>& f);
    void setBigramFrequencies(const QHash<QString, int>& b);
    void setWordLengthHistogram(const QHash<int, int>& h);

    // Unigram frequency map (non-stop tokens).
    const QHash<QString, int>& frequencies() const;

    // Bigram frequency map ("word1 word2" -> count).
    const QHash<QString, int>& bigramFrequencies() const;

    // Word-length histogram (length -> frequency).
    const QHash<int, int>& wordLengthHistogram() const;

private:
    int m_totalTokensNonStop = 0; // counts tokens after stopword filtering
    int m_totalTokensAll = 0;     // counts all tokens (incl. stopwords)

    qint64 m_charCount = 0;

    QHash<QString, int> m_freq;       // unigram frequencies (non-stop)
    QHash<QString, int> m_bigramFreq; // bigram frequencies (non-stop)
    QHash<int, int> m_lengthHist;     // word length histogram

    bool m_tokenLimitReached = false;
    bool m_bigramLimitReached = false;
};

Q_DECLARE_METATYPE(AnalysisResult)