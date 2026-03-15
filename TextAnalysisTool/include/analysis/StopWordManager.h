#pragma once

#include <QSet>
#include <QString>

class StopWordManager
{
public:
    // Loads one stopword per line from a UTF-8 text file.
    // Lines starting with '#' are treated as comments (optional).
    bool loadFromFile(const QString& path);

    // Returns true if 'word' is in the currently loaded stopword set.
    bool isStopWord(const QString& word) const;

    // Clears all loaded stopwords (useful for switching language lists).
    void clear();

private:
    QSet<QString> m_stopWords; // Hash-set for O(1) average lookup.
};
