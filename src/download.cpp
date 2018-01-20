#include "download.h"
#include <QDebug>

QNetworkAccessManager* Download::manager = Q_NULLPTR;

Download::Download(uint id, QUrl url, bool newDownload, QObject* parent): 
	QObject(parent) {
	
	if(manager == Q_NULLPTR) {
		Download::initNetworkAccessManager();
	}
	this->id = id;
	this->url = url;
	this->reply = Q_NULLPTR;
	this->request = QNetworkRequest(this->url);
	//Redirect
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, 
		QVariant("MyRequest")); 
	//set fileName&fileSize
	getDownloadInfo();
	this->file.setFileName(fileName);
} 

void Download::initNetworkAccessManager() {
	Download::manager = new QNetworkAccessManager();
} 

/**
 * @brief  Gets fileName and fileSize from the headers
 * 	If "Content-Length" is emply filesize is -1
 * 	If "Content-Disposition" is emply fileName is url.fileName()
 */
void Download::getDownloadInfo() { 
	reply = manager->get(request);
	connect(reply, &QNetworkReply::readyRead, this, [&]{	
		QString size = reply->rawHeader("Content-Length");
		if(size.isEmpty())
			fileSize = -1;	
		else 
			fileSize = size.toInt();
		fileName = reply->rawHeader("Content-Disposition");
		if(fileName.isEmpty()) {
			qDebug() << "Couldn't get the file name from header."
						<< "Using url file name." << endl;
			fileName = url.fileName();
		} else {
			int x = fileName.indexOf("=");
			fileName = fileName.right(x);
		}
		reply->abort();
		reply->deleteLater();
		reply = Q_NULLPTR;
	});

	//wait for reply to finish 
	QEventLoop loop;
	connect(reply, &QNetworkReply::finished, &loop, [&]{
		loop.exit();
		//TODO: Connection Error Handel 
	});
	loop.exec();
}

void Download::startDownload() { 

	qDebug() << "Starting Download..." << endl;

	if(!file.open(QIODevice::WriteOnly)) {
		qDebug() << "Error: " << file.errorString();
		return;
	}
	state = DOWNLOADING;
	reply = manager->get(request);

	//Write downloaded portion while downloading
	connect(reply, &QNetworkReply::readyRead, this, [&]{
		file.write(reply->readAll());
	});

	//Download progress
	connect(reply, &QNetworkReply::downloadProgress, this, [&]
		(qint64 bytesReceived, qint64 bytesTotal) {
		if(bytesReceived > 0) {
			qDebug() << bytesReceived << " / " << bytesTotal << endl;
		}
	});

	//Download finisehd
	connect(reply, &QNetworkReply::finished, this, [&]{
		if(reply->error()) {
			qDebug() << "Error: " << reply->errorString();
		} else {
			qDebug() << "Finished downloading " << fileName << endl;
		}

		file.close();
		reply->deleteLater();
		reply = Q_NULLPTR;
	});
}
