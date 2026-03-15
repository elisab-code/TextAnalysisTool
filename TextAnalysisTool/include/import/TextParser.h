#pragma once

#include <QString>
#include <QStringList>

// line-based tokenizer/normalizer
class TextParser
{
public:
    QStringList parseLine(const QString& line) const;
};
