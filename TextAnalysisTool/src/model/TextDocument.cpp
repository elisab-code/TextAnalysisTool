#include "model/TextDocument.h"

// Stores the tokenized representation of the document.
void TextDocument::setTokens(const QStringList& tokens)
{
    m_tokens = tokens;
}

// Returns the document tokens as a read-only reference
// to avoid unnecessary copying.
const QStringList& TextDocument::tokens() const
{
    return m_tokens;
}

// Indicates whether the document currently contains any tokens.
bool TextDocument::isEmpty() const
{
    return m_tokens.isEmpty();
}

// Returns the total number of tokens stored in the document.
int TextDocument::tokenCount() const
{
    return m_tokens.size();
}