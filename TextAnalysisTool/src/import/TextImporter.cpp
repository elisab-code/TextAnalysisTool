#include "import/TextImporter.h"

#include <QFile>
#include <QTextStream>
#include <QStringConverter>

QString TextImporter::importPlainTextFile(const QString& path)
{
    // Note: The main application uses a streaming-based import (TextFileReader) for performance.
    // This helper exists as a simple "read whole file" importer for potential future extensions.
    // It returns an empty string if the file cannot be opened.
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    QTextStream s(&f);
    s.setEncoding(QStringConverter::Utf8);

    return s.readAll();
}