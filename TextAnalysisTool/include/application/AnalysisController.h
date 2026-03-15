#pragma once

#include <QString>
#include "model/AnalysisResult.h"

class StopWordManager;

/**
 * Coordinates the analysis workflow.
 *
 * The controller:
 *  - opens the input file
 *  - reads it line-by-line (streaming)
 *  - tokenizes each line
 *  - feeds tokens into the analyzer
 *  - returns the final AnalysisResult
 */
class AnalysisController
{
public:
    /**
     * Creates a controller
     * stopWords optional (if null, analysis runs without stopword filtering)
     */
    explicit AnalysisController(const StopWordManager* stopWords = nullptr);

    /**
     * Analyzes a .txt-file (path selected by the user)
     * Returns computed statistics. 
     */
    AnalysisResult analyzeTxtFile(const QString& path);

    // progress callback (0..100)
    AnalysisResult analyzeTxtFileWithProgress(
        const QString& path,
        const std::function<void(int)>& onProgress
    );

private:
    const StopWordManager* m_stopWords = nullptr; // injected dependency 
};
