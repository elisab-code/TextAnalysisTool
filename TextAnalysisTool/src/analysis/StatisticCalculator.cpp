#include "analysis/StatisticCalculator.h"
#include "model/AnalysisResult.h"

#include <QHash>
#include <vector>
#include <queue>
#include <algorithm>

StatisticCalculator::StatisticCalculator(const AnalysisResult& result)
    : m_r(result)
{
}

double StatisticCalculator::averageWordLength() const
{
    // Histogram: word length -> number of words with that length.
    const auto& hist = m_r.wordLengthHistogram();

    qint64 totalCount = 0;
    qint64 sumLen = 0;

    for (auto it = hist.cbegin(); it != hist.cend(); ++it) {
        const int len = it.key();
        const int count = it.value();
        if (count <= 0) continue;
        totalCount += count;
        sumLen += static_cast<qint64>(len) * static_cast<qint64>(count);
    }

    if (totalCount == 0) return 0.0;
    return static_cast<double>(sumLen) / static_cast<double>(totalCount);
}

// Computes the median directly from the histogram instead of expanding it
// into one entry per word, so memory stays O(distinct lengths).
double StatisticCalculator::medianWordLength() const
{
    const auto& hist = m_r.wordLengthHistogram();

    std::vector<std::pair<int, int>> items;
    items.reserve(static_cast<size_t>(hist.size()));

    qint64 totalCount = 0;
    for (auto it = hist.cbegin(); it != hist.cend(); ++it) {
        const int len = it.key();
        const int count = it.value();
        if (count <= 0) continue;
        items.emplace_back(len, count);
        totalCount += count;
    }

    if (totalCount == 0) return 0.0;

    std::sort(items.begin(), items.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    // 0-based median positions. For odd counts mid1 == mid2,
    // for even counts the median is the mean of both middle values.
    const qint64 mid1 = (totalCount - 1) / 2;
    const qint64 mid2 = totalCount / 2;

    int val1 = 0;
    int val2 = 0;

    qint64 cumulative = 0;
    for (const auto& [len, count] : items) {
        const qint64 next = cumulative + count;

        if (cumulative <= mid1 && mid1 < next) val1 = len;
        if (cumulative <= mid2 && mid2 < next) { val2 = len; break; }

        cumulative = next;
    }

    if (mid1 == mid2) return static_cast<double>(val1);
    return (static_cast<double>(val1) + static_cast<double>(val2)) / 2.0;
}

// Share of content words: non-stop tokens / all tokens (incl. stopwords).
double StatisticCalculator::lexicalDensity() const
{
    const int all = m_r.totalTokensAll();
    if (all <= 0) return 0.0;

    const int nonStop = m_r.totalTokens();
    return static_cast<double>(nonStop) / static_cast<double>(all);
}

int StatisticCalculator::vocabularySize() const
{
    return m_r.frequencies().size();
}

QVector<QPair<QString, int>> StatisticCalculator::topKWords(int k) const
{
    return topKFromHash(m_r.frequencies(), k);
}

QVector<QPair<QString, int>> StatisticCalculator::topKBigrams(int k) const
{
    return topKFromHash(m_r.bigramFrequencies(), k);
}

// Top-k selection with a min-heap of size k: O(n log k) instead of sorting
// the whole map (O(n log n)), which matters for large vocabularies.
QVector<QPair<QString, int>> StatisticCalculator::topKFromHash(const QHash<QString, int>& map, int k)
{
    QVector<QPair<QString, int>> out;
    if (k <= 0 || map.isEmpty()) return out;

    struct Node {
        int count;
        QString key;
    };

    // Smallest count on top, so it can be replaced by a more frequent entry.
    struct MinByCount {
        bool operator()(const Node& a, const Node& b) const {
            return a.count > b.count;
        }
    };

    std::priority_queue<Node, std::vector<Node>, MinByCount> heap;

    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        const QString& key = it.key();
        const int count = it.value();

        if (count <= 0) continue;

        if (static_cast<int>(heap.size()) < k) {
            heap.push(Node{ count, key });
        }
        else if (count > heap.top().count) {
            heap.pop();
            heap.push(Node{ count, key });
        }
    }

    out.reserve(static_cast<int>(heap.size()));
    while (!heap.empty()) {
        const Node n = heap.top();
        heap.pop();
        out.push_back({ n.key, n.count });
    }

    std::sort(out.begin(), out.end(),
        [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
            return a.second > b.second; // descending
        });

    return out;
}
