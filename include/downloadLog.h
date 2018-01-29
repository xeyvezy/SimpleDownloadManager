#pragma once

#include <QFile>
#include <stdexcept>
#include "downloadmodel.h"

class DownloadLog {

public:
	DownloadLog();
	~DownloadLog();

	void loadPreviousDownloads(DownloadModel *model);
	void addNewDownload(Download *download);
	/**
	 * if remove is true delete the fileName entry from log
	 * if remove is false update the status of fileName entry
	 */
	void updateLog(QString fName, bool remove);

private:
	QFile file;
	void openFile(QString fName, QFile &f);
	void writeTFile(QStringList &list, QFile &f);
	static const QString fileName;

};