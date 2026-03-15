#include "model/AnalysisResult.h"

/* Counts every token(including stopwords).
* Needed for lexical density / stopword ratio.
* updates the "all tokens" counter
*/
void AnalysisResult::addRawToken(const QString & token)
{
    if (!token.isEmpty()) {
        ++m_totalTokensAll;   // includes stopwords
    }
}

// Incrementally updates unigram statistics.
// Called once per token during streaming analysis.
void AnalysisResult::addToken(const QString& token)
{
    // Ignore empty tokens
    if (!token.isEmpty()) {

        // Increase global token counter (non-stopwords only)
        ++m_totalTokensNonStop;

        // Increase frequency count for this token.
        ++m_freq[token];
    }
}

// Returns total number of processed tokens.
int AnalysisResult::totalTokens() const
{
    return m_totalTokensNonStop;
}

// Returns total number of tokens including stopwords.
int AnalysisResult::totalTokensAll() const
{
    return m_totalTokensAll;
}

// Returns reference to unigram frequency table.
const QHash<QString, int>& AnalysisResult::frequencies() const
{
    return m_freq;
}

// Adds a bigram consisting of two consecutive tokens.
void AnalysisResult::addBigram(const QString& a, const QString& b)
{
    ++m_bigramFreq[a + " " + b];
}

// Updates the word-length histogram.
void AnalysisResult::addWordLength(int len)
{
    ++m_lengthHist[len];
}

// Returns reference to bigram frequency table.
const QHash<QString, int>& AnalysisResult::bigramFrequencies() const
{
    return m_bigramFreq;
}

// Returns reference to word-length histogram.
const QHash<int, int>& AnalysisResult::wordLengthHistogram() const
{
    return m_lengthHist;
}

void AnalysisResult::setCharCount(qint64 n)
{
    m_charCount = n;
}

qint64 AnalysisResult::charCount() const
{
    return m_charCount;
}
// setters for JSON load
void AnalysisResult::setTotalTokensAll(int n) { m_totalTokensAll = n; }
void AnalysisResult::setTotalTokensNonStop(int n) { m_totalTokensNonStop = n; }
void AnalysisResult::setFrequencies(const QHash<QString, int>& f) { m_freq = f; }
void AnalysisResult::setBigramFrequencies(const QHash<QString, int>& b) { m_bigramFreq = b; }
void AnalysisResult::setWordLengthHistogram(const QHash<int, int>& h) { m_lengthHist = h; }