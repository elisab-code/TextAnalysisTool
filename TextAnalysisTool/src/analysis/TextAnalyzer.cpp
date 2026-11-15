#include "analysis/TextAnalyzer.h"
#include "analysis/StopWordManager.h"

TextAnalyzer::TextAnalyzer(const StopWordManager* stopWords)
    : m_stopWords(stopWords)
{
}

void TextAnalyzer::consumeTokens(const QStringList& tokens)
{
    for (const QString& token : tokens) {
        if (token.isEmpty()) {
            continue;
        }

        // Count every token (incl. stopwords) for:
        // - total word count shown in the UI
        // - stop/non-stop ratio
        m_result.addRawToken(token);

        // Track word length distribution.
        // Current behavior: includes stopwords (matches total word-based metrics).
        m_result.addWordLength(token.length());

        const bool isStop = (m_stopWords != nullptr) && m_stopWords->isStopWord(token);

        // Unigram and bigram statistics are computed for non-stop tokens only.
        if (!isStop) {
            m_result.addToken(token);

            // Bigram: connect consecutive non-stop tokens (also across line boundaries).
            if (!m_prevNonStopToken.isEmpty()) {
                m_result.addBigram(m_prevNonStopToken, token);
            }
            m_prevNonStopToken = token;
        }
        // Stopwords do not advance m_prevNonStopToken, so "Haus und Garten"
        // yields the bigram "haus garten" when stopword filtering is active.
    }
}

const AnalysisResult& TextAnalyzer::result() const
{
    return m_result;
}