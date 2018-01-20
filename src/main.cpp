#include <QApplication>
#include "download.h"

int main(int argc, char* argv[]) {

	QApplication app(argc, argv);
	Download download(0, QUrl("https://github.com/AshishBhattarai/2d-engine/archive/master.zip"), 1);
	
	download.startDownload();

	return app.exec(); 
}  