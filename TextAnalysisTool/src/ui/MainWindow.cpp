#include "ui/MainWindow.h"

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>

#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QFrame>
#include <QTabWidget>

#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QElapsedTimer>

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStringConverter>

#include <QEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

#include <QCoreApplication>

#include "application/AnalysisController.h"
#include "analysis/StopWordManager.h"
#include "analysis/StatisticCalculator.h"
#include "export/ResultExporter.h"

static void applyDarkMockStyle(QWidget* root)
{
    root->setObjectName("AppRoot");

    // Dark UI similar to the mockup:
    // - enforce light text color (fixes "black text" issues)
    // - keep labels transparent (prevents background stripes behind text)
    // - improve groupbox title rendering (title should not cut into the rounded panel)
    // - fix combobox shape (rounded on both sides) + ensure dropdown arrow area matches
    // - make progress bar corners truly rounded
    root->setStyleSheet(R"(
        QWidget#AppRoot {
            background: #1f1f1f;
            color: #eaeaea;
            font-size: 12px;
        }

        QLabel, QCheckBox, QRadioButton {
            color: #eaeaea;
            background: transparent;
        }

        /* ----- GroupBox styling: titles should be clearly visible and not cut the panel ----- */
        QGroupBox {
            background: #2b2b2b;
            border: 1px solid #3a3a3a;
            border-radius: 14px;
            /* provide extra room for the title so it doesn't overlap the border */
            margin-top: 18px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 12px;
            top: 0px;
            padding: 0 8px;
            color: #ffffff;
            font-weight: 700;
            background: #2b2b2b; /* paint behind title to avoid "cutting" the rounded border */
        }

        QLineEdit, QPlainTextEdit, QComboBox, QSpinBox {
            color: #eaeaea;
            background: #1e1e1e;
            border: 1px solid #3a3a3a;
            border-radius: 10px;
        }

        QLineEdit { padding: 6px 8px; border-radius: 8px; }
        QSpinBox { padding: 4px 6px; border-radius: 8px; }

        /* ----- ComboBox: ensure full rounded corners (left + right), including arrow area ----- */
        QComboBox {
            padding: 4px 30px 4px 8px; /* leave room for arrow button */
            border-radius: 10px;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 26px;
            border-left: 1px solid #3a3a3a;
            border-top-right-radius: 10px;
            border-bottom-right-radius: 10px;
            background: #1e1e1e;
        }
        QComboBox::down-arrow {
            width: 10px;
            height: 10px;
        }

        QPlainTextEdit { padding: 8px; }

        QComboBox QAbstractItemView {
            background: #1e1e1e;
            color: #eaeaea;
            selection-background-color: #3a3a3a;
            border: 1px solid #3a3a3a;
        }

        QFrame#DropArea {
            background: #242424;
            border: 1px dashed #5a5a5a;
            border-radius: 12px;
        }

        QPushButton {
            background: #3b3b3b;
            border: 1px solid #4a4a4a;
            border-radius: 10px;
            padding: 8px 12px;
            color: #eaeaea;
        }
        QPushButton:hover { background: #444444; }
        QPushButton:disabled { background: #2f2f2f; color: #888; }

        QPushButton#Primary {
            background: #e00000;
            border: 1px solid #ff4040;
            color: white;
            font-weight: 700;
        }
        QPushButton#Primary:hover { background: #ff0000; }

        /* ----- ProgressBar: rounded corners for both track and chunk ----- */
        QProgressBar {
            background: #2b2b2b;
            border: 1px solid #3a3a3a;
            border-radius: 10px;
            padding: 1px; /* helps chunk rounding visually */
        }
        QProgressBar::chunk {
            background: #e00000;
            border-radius: 9px;
        }

        /* Tabs: compact display instead of multiple large text boxes */
        QTabWidget::pane {
            border: 1px solid #3a3a3a;
            border-radius: 12px;
            top: -1px;
            background: #2b2b2b;
        }
        QTabBar::tab {
            background: #303030;
            border: 1px solid #3a3a3a;
            padding: 6px 10px;
            border-top-left-radius: 10px;
            border-top-right-radius: 10px;
            margin-right: 4px;
            color: #eaeaea;
        }
        QTabBar::tab:selected {
            background: #2b2b2b;
            border-bottom-color: #2b2b2b;
            font-weight: 600;
        }
    )");
}

static QLabel* makeFieldLabel(const QString& text, QWidget* parent)
{
    auto* l = new QLabel(text, parent);
    l->setStyleSheet("color:#cfcfcf;");
    return l;
}

// ---- CSV performance logging (outside debugger) ----------------------------

static QString csvEscape(const QString& s)
{
    // Escape quotes for CSV and wrap field in quotes
    QString out = s;
    out.replace("\"", "\"\"");
    return "\"" + out + "\"";
}

static void appendPerformanceLogCsv(
    const QString& sourceFilePath,
    qint64 fileSizeBytes,
    qint64 elapsedMs,
    const AnalysisResult& result,
    int topK,
    bool stopwordsEnabled,
    const QString& stopwordLang
)
{
    // Log file is stored next to the executable (outside the debugger output)
    const QString logPath = QCoreApplication::applicationDirPath() + "/analysis_log.csv";
    const bool fileExists = QFileInfo::exists(logPath);

    QFile f(logPath);
    if (!f.open(QIODevice::Append | QIODevice::Text)) {
        // Intentionally silent: logging must never break the analysis workflow
        return;
    }

    QTextStream s(&f);
    s.setEncoding(QStringConverter::Utf8);

    // Write header once
    if (!fileExists) {
        s << "timestamp,source_file,file_size_bytes,elapsed_ms,char_count,"
            "tokens_all,tokens_nonstop,unique_words,top_k,stopwords_enabled,stopword_lang\n";
    }

    const QString ts = QDateTime::currentDateTime().toString(Qt::ISODate);

    s << csvEscape(ts) << ","
        << csvEscape(sourceFilePath) << ","
        << fileSizeBytes << ","
        << elapsedMs << ","
        << result.charCount() << ","
        << result.totalTokensAll() << ","
        << result.totalTokens() << ","
        << result.frequencies().size() << ","
        << topK << ","
        << (stopwordsEnabled ? 1 : 0) << ","
        << csvEscape(stopwordLang)
        << "\n";
}

// ---------------------------------------------------------------------------

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("TextAnalysisTool");
    resize(980, 680);

    auto* central = new QWidget(this);
    applyDarkMockStyle(central);

    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(14);

    // Title
    auto* title = new QLabel("Textdokumentanalyse", central);
    title->setStyleSheet("font-size: 18px; font-weight: 700; color: #ffffff;");
    root->addWidget(title);

    // Top row: file + parameters
    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(14);

    // File selection
    auto* gbFile = new QGroupBox("Datei-Auswahl", central);
    gbFile->setObjectName("GbFile");
    auto* fileLay = new QVBoxLayout(gbFile);
    fileLay->setSpacing(10);

    auto* fileHint = new QLabel("Lade eine Textdatei im .txt-Format hoch.", gbFile);
    fileHint->setStyleSheet("color: #cfcfcf;");
    fileLay->addWidget(fileHint);

    auto* fileRow = new QHBoxLayout();
    fileRow->setSpacing(10);

    m_filePathEdit = new QLineEdit(gbFile);
    m_filePathEdit->setPlaceholderText("Dateiname");
    m_browseButton = new QPushButton("hochladen", gbFile);

    // Keep the row aligned and avoid odd stretching
    m_browseButton->setMinimumWidth(96);

    fileRow->addWidget(m_filePathEdit, 1);
    fileRow->addWidget(m_browseButton, 0);
    fileLay->addLayout(fileRow);

    // Drag & drop area
    m_dropArea = new QFrame(gbFile);
    m_dropArea->setObjectName("DropArea");
    m_dropArea->setMinimumHeight(90);
    m_dropArea->setAcceptDrops(true);
    m_dropArea->installEventFilter(this);

    auto* dropLay = new QVBoxLayout(m_dropArea);
    dropLay->setContentsMargins(10, 10, 10, 10);
    auto* dropLabel = new QLabel("Drag & Drop", m_dropArea);
    dropLabel->setAlignment(Qt::AlignCenter);
    dropLabel->setStyleSheet("color: #cfcfcf; font-weight: 600;");
    dropLay->addWidget(dropLabel);

    fileLay->addWidget(m_dropArea);
    topRow->addWidget(gbFile, 1);

    // Analysis parameters
    auto* gbParams = new QGroupBox("Analyseparameter", central);
    gbParams->setObjectName("GbParams");
    auto* paramsLay = new QGridLayout(gbParams);
    paramsLay->setHorizontalSpacing(18);
    paramsLay->setVerticalSpacing(8);
    paramsLay->setColumnStretch(0, 1);
    paramsLay->setColumnStretch(1, 1);

    m_cbChars = new QCheckBox("Zeichenanzahl", gbParams);
    m_cbWords = new QCheckBox("Wortanzahl", gbParams);
    m_cbUnique = new QCheckBox(QString("unterschiedliche W\u00F6rter"), gbParams);

    m_cbTopK = new QCheckBox(QString("Worth\u00E4ufigkeiten (Top-K)"), gbParams);
    m_topKSpin = new QSpinBox(gbParams);
    m_topKSpin->setRange(1, 200);
    m_topKSpin->setValue(10);

    m_cbBigrams = new QCheckBox("Wortpaare (2-Gramme)", gbParams);
    m_cbAvgLen = new QCheckBox(QString("Durchschnittliche Wortl\u00E4nge"), gbParams);
    m_cbStopRatio = new QCheckBox(QString("Verh\u00E4ltnis Stoppw\u00F6rter/\nNicht-Stoppw\u00F6rter"), gbParams);

    m_cbStopwordsEnabled = new QCheckBox(QString("Stoppw\u00F6rter"), gbParams);
    m_cbStopwordsEnabled->setChecked(true);

    m_stopwordLang = new QComboBox(gbParams);
    m_stopwordLang->addItem("Deutsch (de)");
    m_stopwordLang->addItem("English (en)");
    m_stopwordLang->setCurrentIndex(0);

    // Ensure the combobox has enough width so text isn't clipped and rounded corners show
    m_stopwordLang->setMinimumWidth(140);
    m_stopwordLang->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    m_analyzeButton = new QPushButton("analysieren", gbParams);
    m_analyzeButton->setObjectName("Primary");

    // Default selections (like the mockup)
    m_cbChars->setChecked(true);
    m_cbWords->setChecked(true);
    m_cbUnique->setChecked(true);
    m_cbTopK->setChecked(true);
    m_cbBigrams->setChecked(true);
    m_cbAvgLen->setChecked(true);
    m_cbStopRatio->setChecked(true);

    // Left column
    paramsLay->addWidget(m_cbChars, 0, 0);
    paramsLay->addWidget(m_cbWords, 1, 0);
    paramsLay->addWidget(m_cbUnique, 2, 0);

    auto* topKRow = new QHBoxLayout();
    topKRow->setSpacing(8);
    topKRow->addWidget(m_cbTopK);
    topKRow->addStretch(1);
    topKRow->addWidget(new QLabel("K:", gbParams));
    topKRow->addWidget(m_topKSpin);

    auto* topKWrap = new QWidget(gbParams);
    topKWrap->setLayout(topKRow);
    paramsLay->addWidget(topKWrap, 3, 0);

    // Right column
    paramsLay->addWidget(m_cbBigrams, 0, 1);
    paramsLay->addWidget(m_cbAvgLen, 1, 1);
    paramsLay->addWidget(m_cbStopRatio, 2, 1);

    auto* swRow = new QHBoxLayout();
    swRow->setSpacing(8);
    swRow->addWidget(m_cbStopwordsEnabled);
    swRow->addStretch(1);
    swRow->addWidget(m_stopwordLang);

    auto* swWrap = new QWidget(gbParams);
    swWrap->setLayout(swRow);
    paramsLay->addWidget(swWrap, 3, 1);

    paramsLay->addWidget(m_analyzeButton, 4, 1, 1, 1, Qt::AlignRight);

    topRow->addWidget(gbParams, 2);
    root->addLayout(topRow);

    // Progress row
    auto* progRow = new QHBoxLayout();
    progRow->setSpacing(10);

    auto* progLabel = new QLabel("Analysefortschritt:", central);
    progLabel->setStyleSheet("color: #cfcfcf;");

    m_progress = new QProgressBar(central);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(16);

    // Ensure rounded look is actually visible
    m_progress->setAttribute(Qt::WA_StyledBackground, true);

    progRow->addWidget(progLabel);
    progRow->addWidget(m_progress, 1);
    root->addLayout(progRow);

    // Result area
    auto* gbResult = new QGroupBox("Ergebnis", central);
    gbResult->setObjectName("GbResult");

    auto* resultOuter = new QHBoxLayout(gbResult);
    resultOuter->setSpacing(18);

    // Left: summary numbers (must match the visible UI parameters)
    auto* leftBox = new QWidget(gbResult);
    auto* form = new QFormLayout(leftBox);
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFormAlignment(Qt::AlignTop);
    form->setHorizontalSpacing(18);
    form->setVerticalSpacing(10);

    m_lblCharCount = new QLabel("-", leftBox);
    m_lblWordCount = new QLabel("-", leftBox);
    m_lblUniqueCount = new QLabel("-", leftBox);
    m_lblAvgWordLen = new QLabel("-", leftBox);
    m_lblStopRatio = new QLabel("-", leftBox);
    m_lblRuntime = new QLabel("-", leftBox);

    form->addRow(makeFieldLabel("Zeichenanzahl:", leftBox), m_lblCharCount);
    form->addRow(makeFieldLabel("Wortanzahl:", leftBox), m_lblWordCount);
    form->addRow(makeFieldLabel(QString("unterschiedliche W\u00F6rter:"), leftBox), m_lblUniqueCount);

    // IMPORTANT: match parameter names from the UI
    form->addRow(makeFieldLabel(QString("Durchschnittliche Wortl\u00E4nge:"), leftBox), m_lblAvgWordLen);
    form->addRow(makeFieldLabel(QString("Verh\u00E4ltnis Stoppw\u00F6rter/Nicht-Stoppw\u00F6rter:"), leftBox), m_lblStopRatio);

    form->addRow(makeFieldLabel("Laufzeit:", leftBox), m_lblRuntime);

    resultOuter->addWidget(leftBox, 1);

    // Right: tabs with details
    auto* rightBox = new QWidget(gbResult);
    auto* rightLay = new QVBoxLayout(rightBox);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(10);

    auto* tabs = new QTabWidget(rightBox);

    m_topWordsBox = new QPlainTextEdit(tabs);
    m_topWordsBox->setReadOnly(true);

    m_topBigramsBox = new QPlainTextEdit(tabs);
    m_topBigramsBox->setReadOnly(true);

    m_histBox = new QPlainTextEdit(tabs);
    m_histBox->setReadOnly(true);

    tabs->addTab(m_topWordsBox, QString("Top-W\u00F6rter"));
    tabs->addTab(m_topBigramsBox, "2-Gramme");
    tabs->addTab(m_histBox, "Histogramm");

    rightLay->addWidget(tabs, 1);

    // Action buttons: load / save / export
    auto* actionRow = new QHBoxLayout();
    actionRow->addStretch(1);

    m_loadButton = new QPushButton("laden", rightBox);
    m_saveButton = new QPushButton("speichern", rightBox);
    m_exportButton = new QPushButton("exportieren", rightBox);
    m_exportButton->setObjectName("Primary");

    actionRow->addWidget(m_loadButton);
    actionRow->addWidget(m_saveButton);
    actionRow->addWidget(m_exportButton);
    rightLay->addLayout(actionRow);

    resultOuter->addWidget(rightBox, 2);

    root->addWidget(gbResult);
    setCentralWidget(central);

    // Signals/slots
    connect(m_browseButton, &QPushButton::clicked, this, &MainWindow::onBrowseClicked);
    connect(m_analyzeButton, &QPushButton::clicked, this, &MainWindow::onAnalyzeClicked);
    connect(m_exportButton, &QPushButton::clicked, this, &MainWindow::onExportClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::onSaveClicked);
    connect(m_loadButton, &QPushButton::clicked, this, &MainWindow::onLoadClicked);

    resetResultsUi();
    setResultsUiEnabled(false);
}

void MainWindow::setSelectedFile(const QString& path)
{
    m_filePathEdit->setText(path);
    m_lastFilePath = path;
    m_lastResult = AnalysisResult{};
    m_hasResult = false;

    resetResultsUi();
    setResultsUiEnabled(false);
}

void MainWindow::resetResultsUi()
{
    m_lblCharCount->setText("-");
    m_lblWordCount->setText("-");
    m_lblUniqueCount->setText("-");
    m_lblAvgWordLen->setText("-");
    m_lblStopRatio->setText("-");
    m_lblRuntime->setText("-");

    m_topWordsBox->setPlainText("");
    m_topBigramsBox->setPlainText("");
    m_histBox->setPlainText("");
}

void MainWindow::setResultsUiEnabled(bool enabled)
{
    // "Load" should always be possible
    if (m_loadButton) m_loadButton->setEnabled(true);

    // "Save" and "Export" only if a result exists
    if (m_saveButton) m_saveButton->setEnabled(enabled);
    if (m_exportButton) m_exportButton->setEnabled(enabled);
}

QString MainWindow::stopwordFilePath() const
{
    const QString base = QCoreApplication::applicationDirPath() + "/resources/stopwords/";
    const QString file = (m_stopwordLang->currentIndex() == 1) ? "en.txt" : "de.txt";
    return base + file;
}

void MainWindow::updateUiFromResult(const AnalysisResult& result, const QString& runtimeText)
{
    StatisticCalculator calc(result);

    const int allTokens = result.totalTokensAll();
    const int nonStopTokens = result.totalTokens();
    const int stopTokens = allTokens - nonStopTokens;

    // Base values (respect the checkboxes)
    m_lblCharCount->setText(m_cbChars->isChecked() ? QString::number(result.charCount()) : "-");
    m_lblWordCount->setText(m_cbWords->isChecked() ? QString::number(allTokens) : "-");
    m_lblUniqueCount->setText(m_cbUnique->isChecked() ? QString::number(calc.vocabularySize()) : "-");

    // Average word length (UI shows only "Durchschnittliche Wortlänge")
    if (m_cbAvgLen->isChecked()) {
        m_lblAvgWordLen->setText(QString::number(calc.averageWordLength(), 'f', 2));
    }
    else {
        m_lblAvgWordLen->setText("-");
    }

    // Stop/non-stop ratio (UI shows only this ratio)
    if (m_cbStopRatio->isChecked() && allTokens > 0) {
        if (nonStopTokens > 0) {
            const double ratio = static_cast<double>(stopTokens) / static_cast<double>(nonStopTokens);
            // Show numeric ratio plus counts for transparency
            m_lblStopRatio->setText(QString("%1 (%2/%3)")
                .arg(QString::number(ratio, 'f', 3))
                .arg(stopTokens)
                .arg(nonStopTokens));
        }
        else {
            // Edge case: all tokens are stopwords
            m_lblStopRatio->setText(QString("- (%1/%2)")
                .arg(stopTokens)
                .arg(nonStopTokens));
        }
    }
    else {
        m_lblStopRatio->setText("-");
    }

    m_lblRuntime->setText(runtimeText.isEmpty() ? "-" : runtimeText);

    // Top-K content
    const int k = m_topKSpin->value();

    if (m_cbTopK->isChecked()) {
        const auto topWords = calc.topKWords(k);
        QStringList lines;
        for (int i = 0; i < topWords.size(); ++i) {
            lines << QString("%1) %2 (%3)")
                .arg(i + 1)
                .arg(topWords[i].first)
                .arg(topWords[i].second);
        }
        m_topWordsBox->setPlainText(lines.isEmpty() ? "-" : lines.join('\n'));
    }
    else {
        m_topWordsBox->setPlainText("-");
    }

    if (m_cbBigrams->isChecked()) {
        const auto topBigrams = calc.topKBigrams(k);
        QStringList lines;
        for (int i = 0; i < topBigrams.size(); ++i) {
            lines << QString("%1) %2 (%3)")
                .arg(i + 1)
                .arg(topBigrams[i].first)
                .arg(topBigrams[i].second);
        }
        m_topBigramsBox->setPlainText(lines.isEmpty() ? "-" : lines.join('\n'));
    }
    else {
        m_topBigramsBox->setPlainText("-");
    }

    // Word length histogram (1..20) is additional detail (not a checkbox parameter)
    {
        const auto& hist = result.wordLengthHistogram();
        QStringList lines;
        for (int len = 1; len <= 20; ++len) {
            const int count = hist.value(len, 0);
            if (count > 0) lines << QString("len %1: %2").arg(len).arg(count);
        }
        m_histBox->setPlainText(lines.isEmpty() ? "-" : lines.join('\n'));
    }
}

void MainWindow::onBrowseClicked()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Select a text file",
        QString(),
        "Text files (*.txt)"
    );
    if (path.isEmpty()) return;
    setSelectedFile(path);
}

void MainWindow::onAnalyzeClicked()
{
    const QString path = m_filePathEdit->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::information(this, "Hinweis", "Bitte eine .txt-Datei ausw\u00E4hlen.");
        return;
    }
    if (!QFileInfo::exists(path)) {
        QMessageBox::warning(this, "Fehler", "Die ausgew\u00E4hlte Datei existiert nicht.");
        return;
    }

    m_analyzeButton->setEnabled(false);
    m_progress->setValue(0);

    // Busy progress because analysis is currently synchronous
    m_progress->setRange(0, 0);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QCoreApplication::processEvents();

    // Optional stopword list
    StopWordManager stopWords;
    const StopWordManager* swPtr = nullptr;

    if (m_cbStopwordsEnabled->isChecked()) {
        const QString swPath = stopwordFilePath();
        if (stopWords.loadFromFile(swPath)) {
            swPtr = &stopWords;
        }
        else {
            QMessageBox::warning(
                this,
                "Warnung",
                "Stoppwortliste konnte nicht geladen werden:\n" + swPath + "\n\nEs wird ohne Stoppwortfilter fortgesetzt."
            );
        }
    }

    // Run analysis and measure runtime
    QElapsedTimer timer;
    timer.start();

    AnalysisController controller(swPtr);
    const AnalysisResult result = controller.analyzeTxtFile(path);

    const qint64 ms = timer.elapsed();

    // Restore UI state
    QApplication::restoreOverrideCursor();
    m_progress->setRange(0, 100);
    m_progress->setValue(100);
    m_analyzeButton->setEnabled(true);

    // Store result and update UI
    m_lastFilePath = path;
    m_lastResult = result;
    m_hasResult = true;

    updateUiFromResult(m_lastResult, QString("%1 ms").arg(ms));
    setResultsUiEnabled(true);

    // Append a CSV log line next to the executable (outside the debugger)
    {
        const QFileInfo fi(path);
        const qint64 fileSizeBytes = fi.size();

        const bool stopwordsEnabled = m_cbStopwordsEnabled->isChecked();
        const QString stopwordLang = (m_stopwordLang->currentIndex() == 1) ? "en" : "de";

        appendPerformanceLogCsv(
            path,
            fileSizeBytes,
            ms,
            m_lastResult,
            m_topKSpin->value(),
            stopwordsEnabled,
            stopwordLang
        );
    }
}

void MainWindow::onExportClicked()
{
    if (!m_hasResult) {
        QMessageBox::information(this, "Hinweis", "Es liegen noch keine Analyseergebnisse vor.");
        return;
    }

    const QString outPath = QFileDialog::getSaveFileName(
        this,
        "Ergebnis exportieren",
        "analysis_report.txt",
        "Text (*.txt)"
    );
    if (outPath.isEmpty()) return;

    StatisticCalculator calc(m_lastResult);

    QString err;
    if (!ResultExporter::exportReportTxt(outPath, m_lastFilePath, m_lastResult, calc, m_topKSpin->value(), &err)) {
        QMessageBox::warning(this, "Export", err);
        return;
    }

    QMessageBox::information(this, "Export", "Export erfolgreich:\n" + outPath);
}

void MainWindow::onSaveClicked()
{
    if (!m_hasResult) {
        QMessageBox::information(this, "Hinweis", "Es liegen noch keine Analyseergebnisse vor.");
        return;
    }

    const QString outPath = QFileDialog::getSaveFileName(
        this,
        "Analyse speichern",
        "analysis_session.json",
        "JSON (*.json)"
    );
    if (outPath.isEmpty()) return;

    QString err;
    if (!ResultExporter::saveResultJson(outPath, m_lastFilePath, m_lastResult, &err)) {
        QMessageBox::warning(this, "Speichern", err);
        return;
    }

    QMessageBox::information(this, "Speichern", "Session gespeichert:\n" + outPath);
}

void MainWindow::onLoadClicked()
{
    const QString inPath = QFileDialog::getOpenFileName(
        this,
        "Analyse laden",
        QString(),
        "JSON (*.json)"
    );
    if (inPath.isEmpty()) return;

    QString err;
    QString source;
    AnalysisResult loaded;

    if (!ResultExporter::loadResultJson(inPath, &source, &loaded, &err)) {
        QMessageBox::warning(this, "Laden", err);
        return;
    }

    // Apply loaded session
    m_lastFilePath = source;
    m_filePathEdit->setText(source);
    m_lastResult = loaded;
    m_hasResult = true;

    updateUiFromResult(m_lastResult, "geladen");
    setResultsUiEnabled(true);

    QMessageBox::information(this, "Laden", "Session geladen:\n" + inPath);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    // Drag & drop is only enabled inside the dedicated drop area
    if (watched == m_dropArea) {
        if (event->type() == QEvent::DragEnter) {
            auto* e = static_cast<QDragEnterEvent*>(event);
            if (e->mimeData()->hasUrls()) {
                const auto urls = e->mimeData()->urls();
                if (!urls.isEmpty() && urls.first().isLocalFile()) {
                    const QString path = urls.first().toLocalFile();
                    if (path.endsWith(".txt", Qt::CaseInsensitive)) {
                        e->acceptProposedAction();
                        return true;
                    }
                }
            }
            return false;
        }

        if (event->type() == QEvent::Drop) {
            auto* e = static_cast<QDropEvent*>(event);
            const auto urls = e->mimeData()->urls();
            if (!urls.isEmpty() && urls.first().isLocalFile()) {
                const QString path = urls.first().toLocalFile();
                if (path.endsWith(".txt", Qt::CaseInsensitive)) {
                    setSelectedFile(path);
                    e->acceptProposedAction();
                    return true;
                }
            }
            return false;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}