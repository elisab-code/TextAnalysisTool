#pragma once

#include <QString>

/**
 * Converts supported input formats (e.g., .txt) into plain UTF-8/Unicode text
 * for further parsing and analysis. 
 */

class TextImporter
{
public:
	TextImporter() = default;
	QString importPlainTextFile(const QString& path);
};