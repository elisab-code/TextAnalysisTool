cmake_minimum_required(VERSION 3.16)

project(TextAnalysisToolApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent Test)

qt_standard_project_setup()

# ------------------------------------------------------------
# Main application
# ------------------------------------------------------------
qt_add_executable(TextAnalysisTool
    # entry
    src/main.cpp

    # ui
    src/ui/MainWindow.cpp
    include/ui/MainWindow.h

    # application
    src/application/AnalysisController.cpp
    include/application/AnalysisController.h

    # import
    src/import/TextFileReader.cpp
    include/import/TextFileReader.h

    src/import/TextImporter.cpp
    include/import/TextImporter.h

    src/import/TextParser.cpp
    include/import/TextParser.h

    # analysis
    src/analysis/TextAnalyzer.cpp
    include/analysis/TextAnalyzer.h

    src/analysis/StopWordManager.cpp
    include/analysis/StopWordManager.h

    src/analysis/StatisticCalculator.cpp
    include/analysis/StatisticCalculator.h

    # model
    src/model/AnalysisResult.cpp
    include/model/AnalysisResult.h

    src/model/TextDocument.cpp
    include/model/TextDocument.h

    # export
    src/export/ResultExporter.cpp
    include/export/ResultExporter.h

    # Qt resources (stopwords embedded)
    resources/resources.qrc
)

target_include_directories(TextAnalysisTool PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(TextAnalysisTool PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
)

target_compile_options(TextAnalysisTool PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

# Stopwords are embedded via resources.qrc. The copy next to the executable
# is only a fallback, used by MainWindow::stopwordFilePath() if the resource is missing.
add_custom_command(TARGET TextAnalysisTool POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:TextAnalysisTool>/resources
)

# ------------------------------------------------------------
# Tests (QtTest)
# ------------------------------------------------------------
enable_testing()

qt_add_executable(TextAnalysisToolTests
    tests/TextAnalysisTests.cpp

    # Production sources under test (no UI)

    src/application/AnalysisController.cpp
    include/application/AnalysisController.h

    src/import/TextFileReader.cpp
    include/import/TextFileReader.h

    src/import/TextParser.cpp
    include/import/TextParser.h

    src/analysis/TextAnalyzer.cpp
    include/analysis/TextAnalyzer.h

    src/analysis/StopWordManager.cpp
    include/analysis/StopWordManager.h

    src/analysis/StatisticCalculator.cpp
    include/analysis/StatisticCalculator.h

    src/export/ResultExporter.cpp
    include/export/ResultExporter.h

    src/model/AnalysisResult.cpp
    include/model/AnalysisResult.h
)

target_include_directories(TextAnalysisToolTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(TextAnalysisToolTests PRIVATE
    Qt6::Core
    Qt6::Test
)

target_compile_options(TextAnalysisToolTests PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

add_custom_command(TARGET TextAnalysisToolTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE_DIR:TextAnalysisToolTests>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Test>
            $<TARGET_FILE_DIR:TextAnalysisToolTests>
)

add_test(NAME TextAnalysisToolTests COMMAND TextAnalysisToolTests)

# ------------------------------------------------------------
# Test data generator (standalone tool)
# ------------------------------------------------------------
qt_add_executable(TestDataGenerator
    tests/TestDataGenerator.cpp
)

target_include_directories(TestDataGenerator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(TestDataGenerator PRIVATE
    Qt6::Core
)

target_compile_options(TestDataGenerator PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

# main() in TestDataGenerator.cpp is only compiled when this is defined.
target_compile_definitions(TestDataGenerator PRIVATE TEXTANALYSIS_GENERATOR_MAIN)

add_custom_command(TARGET TestDataGenerator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE_DIR:TestDataGenerator>
)