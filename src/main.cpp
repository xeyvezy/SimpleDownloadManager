#include <QApplication>
#include "mainwindow.h"

#ifdef __unix
	#include <sys/socket.h>
	#include <unistd.h>
	#include <netdb.h> 
	#include <netinet/in.h>
	#define	CLOSE(sockfd) close(sockfd)
#elif _WIN32
	#include <winsock.h>
	#define CLOSE(sockfd) closesocket(sockfd)
#else 
	#error "Unknown Platform"
#endif

/**
 * Functor
 * SingleInstance using posix sockets - Singleton Process
 */
class SingleInstance {

public:
	SingleInstance(): sockfd(-1) {}

	~SingleInstance() {
		CLOSE(sockfd);
	}

	bool operator()(const char* iport) {
		struct addrinfo hints, *res;

		#ifdef _WIN32
			WSADATA wsaData;
			MAKEWORD(1,1);

			if(WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
				qDebug() << "WSAStartup failed." << endl;
				exit(1);	
			}
		#endif

		memset(&hints, 0, sizeof hints);
 		hints.ai_family = AF_UNSPEC; 
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		//The port must be unique
		if(getaddrinfo(NULL, iport, &hints, &res)) {
			qDebug() << "Addrinfo failed" << endl;
			exit(1);
		}
 
		if((sockfd = socket(res->ai_family, 
			res->ai_socktype, res->ai_protocol)) < 0) {
			
			qDebug() << "Socket creation failed" << endl;
			exit(1);
		}
		
		//if bind fails there is another application using the port (another instance running)
		if(bind(sockfd, res->ai_addr, res->ai_addrlen) < 0)
			return false;

		return true;
	}

private:
	int sockfd;

};

int main(int argc, char* argv[]) {

	//Default port for instance check is 456743 but users can modify it
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, 
		"AB", "Simple Download Manager");
	settings.beginGroup("Main");
	QString port = settings.value("iport",QString("456743")).toString();
	settings.endGroup();

	SingleInstance instance;
	QApplication app(argc, argv);

	if(instance(port.toStdString().c_str())) {
		//Application MainWindow
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