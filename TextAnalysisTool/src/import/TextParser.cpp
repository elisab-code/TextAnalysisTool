#include "import/TextParser.h"
#include <QRegularExpression>

QStringList TextParser::parseLine(const QString& line) const
{
    // Normalize to lowercase to ensure case-insensitive counting
    const QString normalized = line.toLower();

    /* Extract tokens using a Unicode-aware pattern
    *   \p{L} : any kind of letter (incl. äöüß)
    *   \p{N} : numeric digits
    *   allow internal apostrophes or hyphens (e.g., "don't", "e-mail")
    */
    static const QRegularExpression tokenRe(
        R"((?:[\p{L}\p{N}]+(?:['-][\p{L}\p{N}]+)*))"
    );

    // Pre-allocate space for approximately 16 tokens
    // (16: average number of words per line)
    QStringList tokens;
    tokens.reserve(16); 

    // Find all matches of the regex in the normalized line
    auto it = tokenRe.globalMatch(normalized);
    while (it.hasNext()) {                      
        const auto match = it.next();

        // captured(0) returns the full matched token
        tokens.push_back(match.captured(0));    
    }

    return tokens;
}
