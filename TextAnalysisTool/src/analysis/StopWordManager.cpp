#include "analysis/StopWordManager.h"

#include <QFile>            // Open file
#include <QTextStream>      // Read file
#include <QStringConverter> // UTF-8 Encoding


// Loads stopwords from a UTF-8 encoded text file
bool StopWordManager::loadFromFile(const QString& path)
{
    QFile file(path);

    // Try to open file in read-only text mode
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    // Read file line by line
    while (!stream.atEnd()) {

        // Remove whitespace
        QString line = stream.readLine().trimmed();

        // Ignore comment lines
        if (line.startsWith('#')) {
            continue;
        }
        // Normalize to lowercase 
        line = line.toLower();

        // Insert non-empty stopword into hash set
        if (!line.isEmpty()) {
            m_stopWords.insert(line);
        }
    }

    return true;
}

// Checks whether a word is contained in the stopword set.
bool StopWordManager::isStopWord(const QString& word) const
{
    return m_stopWords.contains(word);
}
// Clears all loaded stopwords.
void StopWordManager::clear()
{
    m_stopWords.clear();
}
