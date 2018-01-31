#include "download.h"
#include <QDebug>
#include <QFileInfo>

QNetworkAccessManager* Download::manager = Q_NULLPTR;

Download::Download(QUrl url, QObject* parent):
		QObject(parent) {

	if(manager == Q_NULLPTR) {
		Download::initNetworkAccessManager();
	}
	this->url = url;
	this->progress = 0;
	this->speed = 0;
	this->lastReceived = 0;
	this->fileSize = -1;
	this->sizeAtPause = 0;
	this->fileName = url.fileName();
	this->state = NOTHING;
	this->reply = Q_NULLPTR;
	this->request = QNetworkRequest(this->url);
	this->stime.start();
	//Redirect
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute,
		QVariant("MyRequest"));
}

Download::Download(QString fileName, QUrl url, qint64 fileSize,
		DownloadState state, QObject *parent): Download(url, parent) {
	this->fileName = fileName;
	this->fileSize = fileSize;
	this->state = state;
	this->file.setFileName(DefaultDirs::DEFAULT_TEMP+fileName);
	this->fileSizeString = convertSizeTString(fileSize);

	if(state == COMPLETED)
		this->progress = 100;
	else {
		this->sizeAtPause = file.size();
		this->progress = (sizeAtPause*100)/fileSize;
	}
}

void Download::initNetworkAccessManager() {
	Download::manager = new QNetworkAccessManager();
}

void Download::deleteManager() {
	delete Download::manager;
}

void Download::startDownload() {

	if(!file.open(QIODevice::WriteOnly|QIODevice::Append)) {
		qDebug() << "Error: " << file.errorString();
		emit downloadError(file.errorString());
		return;
	}

	#ifdef DEBUG
		qDebug() << "Starting Download: " <<  fileName << endl;
	#endif
	state = DOWNLOADING;
	emitStateChanged();
	reply = manager->get(request);

	//Write downloaded portion while downloading
	connect(reply, &QNetworkReply::readyRead, this, [&]{
		file.write(reply->readAll());
	});

	//Download progress
	connect(reply, &QNetworkReply::downloadProgress, this,
			[&](qint64 bytesReceived, qint64 bytesTotal) {
		if(bytesTotal > 0 && fileSize < 0) {
			fileSize = bytesTotal;
			fileSizeString = convertSizeTString(fileSize);
			emit fileSizeUpdate(fileSize);
		}
		if(bytesReceived > 0) {
			int curProgress = ((bytesReceived+sizeAtPause)*100) /
				(bytesTotal+sizeAtPause);
			if((curProgress-progress) > 0) {
				progress = curProgress;
				emit progressUpdate(progress);
			}
			if(stime.elapsed() > 1000) {
				speed = bytesReceived - lastReceived;
				lastReceived = bytesReceived;
				speedString = convertSizeTString(speed);
				emit downloadSpeed(speed);
				stime.restart();
			}
			// #ifdef DEBUG
			// 	qDebug() << "Progress: " << progress << endl
			// 		<< "Speed: " << speed << endl
			// 		<< "BRecv/BTot: " << bytesReceived << "/" << bytesTotal
			// 		<< endl;
			// #endif
		}
	});

	//Download finisehd
	connect(reply, &QNetworkReply::finished, this, [&]{
		if(reply->error()) {
			if(reply->error() != QNetworkReply::OperationCanceledError) {
				qDebug() << "Error: " << reply->errorString();
				emit downloadError(reply->errorString());
				state = FAILED;
			}
		} else {
			#ifdef DEBUG
				qDebug() << "Finished downloading " << fileName << endl;
			#endif
			state = COMPLETED;
		}

		speedString = "";
		emit downloadSpeed(0);
		emitStateChanged();
		file.close();
		reply->deleteLater();
		reply = Q_NULLPTR;
	});
}

void Download::pauseDownload() {
	if(!reply || state != DOWNLOADING) return;
	#ifdef DEBUG
		qDebug() << "Download Paused" << fileName << endl
		 	<< "Size on disk: " << file.size() << endl;
	#endif
	state = PAUSED;
	emitStateChanged();
	reply->abort();
	reply = Q_NULLPTR;
}

void Download::resumeDownload() {
	if(state != PAUSED) return;
	if(file.size() > 0) {
		#ifdef DEBUG
			qDebug() << "Download Resumed" << endl
				<< "Size on disk: " << file.size() << endl;
		#endif
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
		fileSizeString = convertSizeTString(fileSize);
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
	connect(reply, &QNetworkReply::finished, this, [&]{
		if(reply->error() == reply->ProtocolUnknownError)
			emit downloadInfoComplete("Invalid Downlaod URL!", reply->error());
		else if(reply->error() != reply->OperationCanceledError)
			emit downloadInfoComplete(reply->errorString(), reply->error());
		else {
			this->fileName = checkFileName(fileName, DefaultDirs::DEFAULT_TEMP);
			this->fileName = checkFileName(fileName, DefaultDirs::DEFAULT_SAVE);
			this->file.setFileName(DefaultDirs::DEFAULT_TEMP+fileName);
			emit downloadInfoComplete(reply->errorString(), 0);
		}
	});
}

/**
 * check if file name already exists, if so append add a number to make it unique
 * DefaultDirs::DEFAULT_TEMP
 */
QString Download::checkFileName(QString fName, QString dir) {
	if(fileExists(dir+fName)) {
		int i = 1;
		QStringList list = fName.split(".");
		QString name = list.at(0);
		QString ext;
		for(int i = 1; i < list.size(); ++i) {
			ext.append("."+list.at(i));
		}
		while(fileExists(dir+QString::number(i) + "-" +
				name + ext)) {
			++i;
		}
		fName = QString::number(i)+"-"+fName;
	}
	return fName;
}

bool Download::fileExists(QString path) {
	QFileInfo check(path);
	#ifdef DEBUG
		qDebug() << path;
	#endif
	return check.exists() && check.isFile();
}

QString Download::convertSizeTString(qint64 size) {
 	if(size < 0) {
		 return QString("UN"); //unknown
	 }
	int i = 0;

	float fsize = size;

	while(fsize>1) {
		if(i==5) break;
		fsize/=1024;
		++i;
	}
	fsize *= 1024;
	switch(i) {
		case 1:
			return QString::number(fsize,'f',2)+"B";
		case 2:
			return QString::number(fsize,'f',2)+"KB";
		case 3:
			return QString::number(fsize,'f',2)+"MB";
		case 4:
			return QString::number(fsize,'f',2)+"GB";
		default:
			return QString::number(fsize,'f',2)+"TB";
	}
}