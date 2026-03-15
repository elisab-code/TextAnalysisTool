#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>

/**
 * file reader for text files (UTF-8) providing line-by-line access 
 */
class TextFileReader
{
public:
    TextFileReader() = default;

    bool open(const QString& path);
    bool atEnd() const;
    QString readLine();
    void close();

    // progress reporting
    qint64 size() const;
    qint64 pos() const;

private:
    QFile m_file;
    QTextStream m_stream;
};
