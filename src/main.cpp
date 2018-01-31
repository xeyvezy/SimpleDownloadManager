#include <QApplication>
#include <QHostAddress>
#include <QTcpSocket>
#include "mainwindow.h"

/**
 * SingleInstance using posix sockets - Singleton Process
 */
class SingleInstance {

public:
	SingleInstance(){}
	~SingleInstance() {
		sock.close();
	}

	bool operator()(qint16 iport) {
		return sock.bind(QHostAddress::LocalHost, iport);
	}

private:
	QTcpSocket sock;
};

int main(int argc, char* argv[]) {

	//Default port for instance check is 456743 but users can modify it
	QSettings settings(QSettings::IniFormat, QSettings::UserScope,
		"AB", "Simple Download Manager");
	settings.beginGroup("Main");
	qint16 port = settings.value("iport", qint16(456743)).toInt();
	settings.endGroup();

	SingleInstance instance;
	QApplication app(argc, argv);

	if(instance(port)) {
		//Application MainWindow
		DefaultDirs::initDefaultDirs();
		MainWindow mainwindow;
		mainwindow.show();
		return app.exec();
	} else {
		//Another instance warning
		QMessageBox window( QMessageBox::Icon::Warning,"Error",
			"Another instance already running.",QMessageBox::StandardButton::Ok);
		window.show();
		return window.exec();
	}

	return 0;
}