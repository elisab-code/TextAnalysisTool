#include <QApplication>
#include <QMetaType>
#include "model/AnalysisResult.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);	// initialize Qt application
	qRegisterMetaType<AnalysisResult>("AnalysisResult");	// AnalysisResult can be sent via queued signals/slots across threads
	MainWindow w;					// create MainWindow
	w.show();						// show window
	return app.exec();				// start event loop
}