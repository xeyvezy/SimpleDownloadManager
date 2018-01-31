#include "downloadLog.h"

const QString DownloadLog::fileName = DefaultDirs::DEFAULT_LOG + "downloads.log";

DownloadLog::DownloadLog() {
	openFile(fileName, file);
}

DownloadLog::~DownloadLog() {
	file.close();
}

void DownloadLog::loadPreviousDownloads(DownloadModel *model) {
	/**
	 * Log file structure:
	 * FileName URL Size(total) Status
	 */
	file.seek(0);
	while(!file.atEnd()) {
		QString line = file.readLine();
		QStringList list = line.split(" ");

		if(list.size() < 4) return;
		#ifdef DEBUG
			qDebug() << "Loading(log): " << list << endl;
		#endif

		QString fileName = list.at(0);
		QUrl url = list.at(1);
		qint64 fileSize = list.at(2).toInt();
		Download::DownloadState state = (Download::DownloadState)
			list.at(3).toInt();

		if(state != Download::COMPLETED) {
			state = Download::PAUSED;
		}

		Download *download = new Download(fileName, url, fileSize,
			state, model);
		model->addNewDownload(download);
	}
}

void DownloadLog::addNewDownload(Download *download) {
	QStringList list;
	list.append(download->getFileName());
	list.append(download->getUrl().toString());
	list.append(QString::number(download->getFileSize()));
	list.append(QString::number(download->getState()).append('\n'));

	#ifdef DEBUG
		qDebug() << "Writing: " << list << endl;
	#endif

	writeTFile(list, file);
	file.write("\n");
}

void DownloadLog::updateLog(QString fName, bool remove) {
	QFile newFile;
	openFile(this->fileName+"-tmp", newFile);

	file.seek(0);
	while(!file.atEnd()) {
		QString line = file.readLine();
		QStringList list = line.split(" ");

		if(list.size() < 4)
			continue;

		QString name = list.at(0);
		if(name == fName) {
			if(remove) continue;
			else
				list[3] = QString::number(Download::COMPLETED);
		}
		#ifdef DEBUG
			qDebug() << "Copying to temp: " << list << endl;
		#endif
		writeTFile(list, newFile);
	}
	file.close();
	if(!file.remove()) qDebug() << "Failed To Delete: " << file.fileName();
	newFile.rename(fileName);
	newFile.close();
	openFile(fileName, file);
}

void DownloadLog::openFile(QString fName, QFile &f) {
	f.setFileName(fName);
	if(!f.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append)) {
		qDebug() << f.errorString() << endl;
		return;
	}
}

void DownloadLog::writeTFile(QStringList &list, QFile &f) {
	QTextStream out(&f);
	f.seek(f.size());
	for(int i = 0; i < list.size(); ++i) {
		if(i != list.size()-1)
			out << list.at(i) + " ";
		else
			out << list.at(i);
	}
}