#pragma once

#include <QString>
#include <QStringList>

/**
 * Holds the tokenized representation of a text document.
 */
class TextDocument
{
public:
    TextDocument() = default;

    void setTokens(const QStringList& tokens);
    const QStringList& tokens() const;

    bool isEmpty() const;
    int tokenCount() const;

private:
    QStringList m_tokens;
};
