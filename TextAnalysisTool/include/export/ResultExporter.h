#pragma once

#include <QString>
#include "model/AnalysisResult.h"

class StatisticCalculator;

/**
 * Export/Import utilities for analysis results.
 *
 * - exportReportTxt(): export report (UTF-8)
 * - saveResultJson(): persist results for later reuse
 * - loadResultJson(): restore a previously saved session
 */
class ResultExporter
{
public:
    static bool exportReportTxt(
        const QString& outPath,
        const QString& sourceFile,
        const AnalysisResult& result,
        const StatisticCalculator& calc,
        int topK,
        QString* error = nullptr
    );

    static bool saveResultJson(
        const QString& outPath,
        const QString& sourceFile,
        const AnalysisResult& result,
        QString* error = nullptr
    );

    static bool loadResultJson(
        const QString& inPath,
        QString* sourceFileOut,
        AnalysisResult* resultOut,
        QString* error = nullptr
    );
};