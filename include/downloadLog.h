#pragma once

#include <QFile>
#include <stdexcept>
#include "downloadmodel.h"

class DownloadLog {

public:
	enum LogUpdate {
		REMOVE = 0x0000,
		STATE = 0x0001,
		FSIZE = 0x0002
	};
	DownloadLog();
	~DownloadLog();

	void loadPreviousDownloads(DownloadModel *model);
	void addNewDownload(Download *download);
	/**
	 * if remove is true delete the fileName entry from log
	 * if remove is false update the status of fileName entry
	 */
	void updateLog(Download *download, LogUpdate update);

private:
	QFile file;
	void openFile(QString fName, QFile &f);
	void writeTFile(QStringList &list, QFile &f);
	static const QString fileName;

};