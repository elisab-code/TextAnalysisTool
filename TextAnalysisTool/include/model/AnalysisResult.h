#pragma once

#include <QHash>
#include <QString>
#include <QMetaType>

/**
 * Stores all computed statistical results of the analysis.
 *
 * Design:
 *  - Streaming-friendly: Analyzer updates counts incrementally.
 *  - Distinguishes between:
 *      (1) all tokens (incl. stopwords)  -> for lexical density / ratios
 *      (2) non-stop tokens only          -> for unigram/bigram stats
 */
class AnalysisResult
{
public:
    // counts every token (including stopwords), but does NOT store frequencies
    void addRawToken(const QString& token);

    // Counts only NON-stopwords and updates unigram frequencies
    void addToken(const QString& token);

    // Adds a bigram consisting of two consecutive NON-stop tokens
    void addBigram(const QString& a, const QString& b);

    // Updates histogram for word lengths (you currently call this for all tokens)
    void addWordLength(int len);

    // Non-stop token count (after stopword filtering)
    int totalTokens() const;

    // token count including stopwords
    int totalTokensAll() const;

    // character count of the processed document (sum of line lengths)
    void setCharCount(qint64 n);
    qint64 charCount() const;

    // setters for load JSON (read-only pipeline remains unchanged)
    void setTotalTokensAll(int n);
    void setTotalTokensNonStop(int n);
    void setFrequencies(const QHash<QString, int>& f);
    void setBigramFrequencies(const QHash<QString, int>& b);
    void setWordLengthHistogram(const QHash<int, int>& h);

    // Unigram frequency map (non-stop tokens)
    const QHash<QString, int>& frequencies() const;

    // Bigram frequency map ("word1 word2" -> count)
    const QHash<QString, int>& bigramFrequencies() const;

    // Word-length histogram (length -> frequency)
    const QHash<int, int>& wordLengthHistogram() const;

private:
    int m_totalTokensNonStop = 0; // counts tokens after stopword filtering
    int m_totalTokensAll = 0;     // counts all tokens (incl. stopwords)

    qint64 m_charCount = 0;

    QHash<QString, int> m_freq;       // unigram frequencies (non-stop)
    QHash<QString, int> m_bigramFreq; // bigram frequencies (non-stop)
    QHash<int, int> m_lengthHist;     // word length histogram
};
Q_DECLARE_METATYPE(AnalysisResult)
