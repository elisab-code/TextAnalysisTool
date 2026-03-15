#pragma once

#include <QMainWindow>
#include "model/AnalysisResult.h"

class QLineEdit;
class QPushButton;
class QProgressBar;
class QCheckBox;
class QSpinBox;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QFrame;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onBrowseClicked();
    void onAnalyzeClicked();
    void onExportClicked();
    void onSaveClicked();
    void onLoadClicked();

private:
    // File input
    QLineEdit* m_filePathEdit = nullptr;
    QPushButton* m_browseButton = nullptr;
    QFrame* m_dropArea = nullptr;

    // Session actions (JSON)
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_loadButton = nullptr;

    // Parameters
    QCheckBox* m_cbChars = nullptr;
    QCheckBox* m_cbWords = nullptr;
    QCheckBox* m_cbUnique = nullptr;
    QCheckBox* m_cbTopK = nullptr;
    QSpinBox* m_topKSpin = nullptr;

    QCheckBox* m_cbBigrams = nullptr;
    QCheckBox* m_cbAvgLen = nullptr;
    QCheckBox* m_cbStopRatio = nullptr;

    QCheckBox* m_cbStopwordsEnabled = nullptr;
    QComboBox* m_stopwordLang = nullptr;

    QPushButton* m_analyzeButton = nullptr;

    // Progress
    QProgressBar* m_progress = nullptr;

    // Result summary (left)
    QLabel* m_lblCharCount = nullptr;
    QLabel* m_lblWordCount = nullptr;
    QLabel* m_lblUniqueCount = nullptr;

    // IMPORTANT: match visible UI parameters
    QLabel* m_lblAvgWordLen = nullptr;    // "Durchschnittliche Wortlänge"
    QLabel* m_lblStopRatio = nullptr;     // "Verhältnis Stoppwörter/Nicht-Stoppwörter"

    QLabel* m_lblRuntime = nullptr;

    // Result details (right, tabs)
    QPlainTextEdit* m_topWordsBox = nullptr;
    QPlainTextEdit* m_topBigramsBox = nullptr;
    QPlainTextEdit* m_histBox = nullptr;

    QPushButton* m_exportButton = nullptr;

    // State
    QString m_lastFilePath;
    AnalysisResult m_lastResult;
    bool m_hasResult = false;

    // Helpers
    void setSelectedFile(const QString& path);
    void resetResultsUi();
    void setResultsUiEnabled(bool enabled);
    void updateUiFromResult(const AnalysisResult& result, const QString& runtimeText);

    QString stopwordFilePath() const;
};