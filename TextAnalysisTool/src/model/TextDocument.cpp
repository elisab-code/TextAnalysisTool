#include "model/TextDocument.h"

void TextDocument::setTokens(const QStringList& tokens)
{
    m_tokens = tokens;
}

const QStringList& TextDocument::tokens() const
{
    return m_tokens;
}

bool TextDocument::isEmpty() const
{
    return m_tokens.isEmpty();
}

int TextDocument::tokenCount() const
{
    return m_tokens.size();
}
