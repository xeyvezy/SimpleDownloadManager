#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QEventLoop>
#include <QApplication>
#include <QStorageInfo>
#include <chrono>
#include "defaultDirs.h"

// #define DEBUG

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

	//Constructor for creating brand new download
	explicit Download(QUrl url, QObject *parent = 0);
	//constructor loading downloads from the log
	explicit Download(QString fileName, QUrl url, qint64 filesize,
		DownloadState state, QObject *parent = 0);

	void getDownloadInfo();
	void startDownload();
	void pauseDownload();
	void resumeDownload();
	static void deleteManager();

private:
	//Members
	int progress;
	int speed;
	std::chrono::steady_clock::time_point etime;
	std::chrono::steady_clock::time_point stime;
	qint64 lastReceived;
	qint64 fileSize;
	qint64 sizeAtPause;
	QUrl url;
	QString fileName;
	QString fileSizeString;
	QString speedString;
	QString etaString;
	QFile file;
	DownloadState state;
	QNetworkReply* reply;
	QNetworkRequest request;
	static QNetworkAccessManager* manager;

	//Methods
	static void initNetworkAccessManager();
	bool fileExists(QString path);
	QString checkFileName(QString name, QString dir);
	void emitStateChanged() {
		emit stateChanged();
	}
	QString calcEta();
	QString convertSizeTString(qint64 size);


signals:
	void downloadError(QString message);
	void downloadInfoComplete(QString message, bool error);
	void fileSizeUpdate();
	void progressUpdate();
	void stateChanged();
	void downloadSpeed();
	void downloadEta();

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

	QString getFilePath() const {
		return file.fileName();
	}

	DownloadState getState() const {
		return state;
	}

	QString getStateString() const {
		switch(state) {
			case DOWNLOADING:
				return QString("Downloading");
			case COMPLETED:
				return QString("Completed");
			case PAUSED:
				return QString("Paused");
			case FAILED:
				return QString("Failed");
			default:
				return QString(".....");
		}
	}

	QString getFileSizeString() const {
		return fileSizeString;
	}

	QString getSpeedString() const {
		return speedString;
	}

	QString getEtaString() const {
		return etaString;
	}
};
