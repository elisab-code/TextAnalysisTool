#include "model/AnalysisResult.h"

// Counts every token (including stopwords).
// Needed for lexical density / stopword ratio.
void AnalysisResult::addRawToken(const QString& token)
{
    if (!token.isEmpty()) {
        ++m_totalTokensAll;
    }
}

// Incrementally updates unigram statistics.
// Called once per token during streaming analysis.
void AnalysisResult::addToken(const QString& token)
{
    if (token.isEmpty()) {
        return;
    }

    // Count token even if we later cannot store a new key (limit reached).
    ++m_totalTokensNonStop;

    auto it = m_freq.find(token);
    if (it != m_freq.end()) {
        ++it.value();
        return;
    }

    // New key: enforce limit.
    if (m_freq.size() >= kMaxUniqueTokens) {
        m_tokenLimitReached = true;
        return;
    }

    m_freq.insert(token, 1);
}

// Returns total number of processed tokens (non-stopwords).
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
    const QString key = a + " " + b;

    auto it = m_bigramFreq.find(key);
    if (it != m_bigramFreq.end()) {
        ++it.value();
        return;
    }

    if (m_bigramFreq.size() >= kMaxUniqueBigrams) {
        m_bigramLimitReached = true;
        return;
    }

    m_bigramFreq.insert(key, 1);
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

void AnalysisResult::setFrequencies(const QHash<QString, int>& f)
{
    m_freq = f;
    if (m_freq.size() > kMaxUniqueTokens) {
        m_tokenLimitReached = true;
    }
}

void AnalysisResult::setBigramFrequencies(const QHash<QString, int>& b)
{
    m_bigramFreq = b;
    if (m_bigramFreq.size() > kMaxUniqueBigrams) {
        m_bigramLimitReached = true;
    }
}

void AnalysisResult::setWordLengthHistogram(const QHash<int, int>& h) { m_lengthHist = h; }