#include "download.h"
#include <QDebug>
#include <QFileInfo>

QNetworkAccessManager* Download::manager = Q_NULLPTR;

Download::Download(uint id, QUrl url, bool newDownload, QObject* parent): 
	QObject(parent) {
	
	if(manager == Q_NULLPTR) {
		Download::initNetworkAccessManager();
	}
	this->id = id;
	this->url = url;
	this->progress = 0;
	this->fileSize = -1;
	this->reply = Q_NULLPTR;
	this->request = QNetworkRequest(this->url);
	//Redirect
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, 
		QVariant("MyRequest")); 
} 

void Download::initNetworkAccessManager() {
	Download::manager = new QNetworkAccessManager();
} 

void Download::updateDownloadInfo() {
	//set fileName&fileSize
	getDownloadInfo();
	if(fileExists(fileName)) {
		int i = 1;
		while(fileExists(QString::number(i)+"-"+fileName)) {
			++i;
		}
		fileName = QString::number(i)+"-"+fileName;
	}
	
	this->file.setFileName(fileName);
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
		if(bytesTotal > 0 && fileSize < 0) {
			fileSize = bytesTotal;
			emit fileSizeUpdate(fileSize);
		}
		if(bytesReceived > 0) {
			progress = (bytesReceived*100)/bytesTotal;
			emit progressUpdate(progress);
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

bool Download::fileExists(QString path) {
	QFileInfo check(path);
	return check.exists() && check.isFile();
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
		if(!size.isEmpty())
			fileSize = size.toInt();
		fileName = reply->rawHeader("Content-Disposition");
		if(fileName.isEmpty()) {
			qDebug() << "Couldn't get the file name from header."
						<< "Using url file name." << endl;
			fileName = url.fileName();
		} else {
			int x = fileName.indexOf('=');
			fileName = fileName.mid(x+1, fileName.length());
		} 
		reply->abort();
		reply->deleteLater();
		reply = Q_NULLPTR;
	});

	//wait for reply to finish 
	QEventLoop loop;
	connect(reply, &QNetworkReply::finished, &loop, [&]{
		loop.exit();
		
		if(reply->error() == reply->ProtocolUnknownError) 
			emit downloadInfoComplete("Invalid Downlaod URL!", reply->error());
		else if(reply->error() != reply->OperationCanceledError)
			emit downloadInfoComplete(reply->errorString(), reply->error());
		else 
		 	emit downloadInfoComplete(reply->errorString(), 0);
	});
	loop.exec(); 
}