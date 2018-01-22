#include "download.h"
#include <QDebug>
#include <QFileInfo>

QNetworkAccessManager* Download::manager = Q_NULLPTR;

Download::Download(QUrl url, bool newDownload, QObject* parent): 
	QObject(parent) {
	
	if(manager == Q_NULLPTR) {
		Download::initNetworkAccessManager();
	}
	this->url = url;
	this->progress = 0;
	this->fileSize = -1;
	this->sizeAtPause = 0;
	this->fileName = url.fileName();
	this->state = NOTHING;
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

bool Download::fileExists(QString path) {
	QFileInfo check(path);
	return check.exists() && check.isFile();
}

void Download::startDownload() { 
  	
	if(!file.open(QIODevice::WriteOnly|QIODevice::Append)) {
		qDebug() << "Error: " << file.errorString();
		emit downloadError(file.errorString());
		return;
	}
	
	qDebug() << "Starting Download..." << endl;
	state = DOWNLOADING;
	emitStateChanged();
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
			progress = ((bytesReceived+sizeAtPause)*100) /
				(bytesTotal+sizeAtPause);
			emit progressUpdate(progress);
		}
	});

	//Download finisehd
	connect(reply, &QNetworkReply::finished, this, [&]{
		if(reply->error()) {
			if(reply->error() != QNetworkReply::OperationCanceledError) {
				qDebug() << "Error: " << reply->errorString();
				emit downloadError(reply->errorString());
				state = FAILED;
				emitStateChanged();
			}
		} else {
			qDebug() << "Finished downloading " << fileName << endl;
			state = COMPLETED;
			emitStateChanged();
		}

		file.close(); 
		reply->deleteLater();
		reply = Q_NULLPTR;
	});
}

void Download::pauseDownload() {
	if(!reply)
		return;
	qDebug() << "Download Paused" << endl;
	state = PAUSED;
	emitStateChanged();
	reply->abort();
	reply = Q_NULLPTR;
}

void Download::resumeDownload() {
	if(file.size() > 0) {
		qDebug() << "Download Resumed" << endl;
		sizeAtPause = file.size();
		QByteArray rangeHeader = "bytes="+QByteArray::number(sizeAtPause)+"-";
		request.setRawHeader(QByteArray("Range"), rangeHeader);
	}

	startDownload();
}

/**
 * @brief  Gets fileName and fileSize from the headers
 * 	If "Content-Length" is emply filesize is -1
 * 	If "Content-Disposition" is emply fileName is url.fileName()
 */
void Download::getDownloadInfo() { 
	reply = manager->get(request);
	connect(reply, &QNetworkReply::readyRead, this, [&]{		
		QString data = reply->rawHeader("Content-Length");
		if(!data.isEmpty())
			fileSize = data.toInt();
		
		data = reply->rawHeader("Content-Disposition");
		if(!data.isEmpty()) {
			int x = data.indexOf('=');
			fileName = data.mid(x+1, data.length());
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

QString Download::getFileSizeString() const {
 
		if(!fileSize) return QString();
		int i = 0;
		float size = fileSize;

		while(size>1) {
			if(i==5) break;
			size/=1024;
			++i;
		}
		size *= 1024;
		switch(i) {
			case 1:
				return QString::number(size)+"B";
			case 2:
				return QString::number(size)+"KB";
			case 3:
				return QString::number(size)+"MB";
			case 4:
				return QString::number(size)+"GB";
			default:
				return QString::number(size)+"TB";
		}
}