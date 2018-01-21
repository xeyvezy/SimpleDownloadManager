#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[]) {

	QApplication app(argc, argv);

	MainWindow mainwindow;
	mainwindow.show();
	mainwindow.setWindowTitle("SimpleDownloadManager");

	return app.exec(); 
}  