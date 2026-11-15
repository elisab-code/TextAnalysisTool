#include "import/TextFileReader.h"

bool TextFileReader::open(const QString& path)  // Opens the file and prepares the text stream (UTF-8).
{
    m_file.setFileName(path);   // Store the file path in the QFile object. 

    // Try to open the file for reading as text.
    if (!m_file.open(QIODevice::ReadOnly | QIODevice::Text)) { // If file could not be opened: cancel.
        return false;          
    }

    // Attach the QTextStream to the opened QFile.
    m_stream.setDevice(&m_file);

    // Interpret the incoming bytes as UTF-8. 
    m_stream.setEncoding(QStringConverter::Utf8);

    return true;                // File and stream are ready to be used.
}

bool TextFileReader::atEnd() const
{
    // Returns true if the stream has reached the end of the input.
    return m_stream.atEnd();
}

QString TextFileReader::readLine()
{
    // Reads one line from the stream.
    return m_stream.readLine();
}

void TextFileReader::close()
{
    // close the file handle.
    if (m_file.isOpen()) {
        m_file.close();
    }
}
qint64 TextFileReader::size() const { return m_file.size(); }
qint64 TextFileReader::pos() const { return m_file.pos(); }
