#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QEventLoop>
#include <QApplication> 
#include <QStorageInfo>

class Download : public QObject {

Q_OBJECT

public: 
	enum DownloadState {
		NOTHING = 0x0000,
		DOWNLOADING = 0x0001,
		PAUSED = 0x0002,
		COMPLETED = 0x0003
	};

	explicit Download(uint id, QUrl url, bool newDownload, QObject* parent = 0);

	void updateDownloadInfo();
	void startDownload();

private:
	//Members
	uint id;
	qint64 fileSize;
	int progress;
	QUrl url;
	QString fileName;
	QFile file;
	DownloadState state;
	QNetworkReply* reply;
	QNetworkRequest request;
	static QNetworkAccessManager* manager;
	
	//Methods
	static void initNetworkAccessManager();
	void getDownloadInfo();
	bool fileExists(QString path);


signals:
	void downloadInfoComplete(QString message, bool error);
	void fileSizeUpdate(qint64 size);
	void progressUpdate(int percent);

	//Getters
public:
	uint getId() const {
		return id;
	}

	int getProgress() const {
		return progress;
	}

	qint64 getFileSize() const {
		return fileSize;
	}

	QUrl getUrl() const {
		return url;
	}

	QString getFileName() const {
		return fileName;
	}

	DownloadState getState() const {
		return state;
	}

};
