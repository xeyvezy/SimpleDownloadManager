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
		COMPLETED = 0x0003,
		FAILED = 0x0004
	};

	explicit Download(QUrl url, bool newDownload, QObject* parent = 0);

	void getDownloadInfo();
	void startDownload();
	void pauseDownload();
	void resumeDownload();

private:
	//Members
	int progress;
	qint64 fileSize;
	qint64 sizeAtPause;
	QUrl url;
	QString fileName;
	QString fileSizeString;
	QFile file;
	DownloadState state;
	QNetworkReply* reply;
	QNetworkRequest request;
	static QNetworkAccessManager* manager;
	
	//Methods
	static void initNetworkAccessManager();
	bool fileExists(QString path);
	void updateDownloadInfo();
	void emitStateChanged() {
		emit stateChanged(state);
	}
	void setFileSizeString();


signals:
	void downloadError(QString message);
	void downloadInfoComplete(QString message, bool error);
	void fileSizeUpdate(qint64 size);
	void progressUpdate(int percent);
	void stateChanged(DownloadState state);

	//Getters
public:
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

	QString getStateString() const {
		switch(state) {
			case DOWNLOADING:
				return QString("DOWNLOADING");
			case COMPLETED:
				return QString("COMPLETED");
			case PAUSED:
				return QString("PAUSED");
			case FAILED:
				return QString("FAILED");
			default:
				return QString("NOTHING");
		}
	}

	QString getFileSizeString() const {
		return fileSizeString;
	}
};
