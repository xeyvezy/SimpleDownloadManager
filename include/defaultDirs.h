#pragma once

#include <QString>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

namespace DefaultDirs {

	#ifdef __unix
		static const QString HOME_DIR =
			QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
		static const QString DEFAULT_TEMP =
			HOME_DIR + "/.SimpleDownloadManager/Downloads/";
		static const QString DEFAULT_SAVE = HOME_DIR + "/Downloads/";
		static const QString DEFAULT_LOG = HOME_DIR + "/.SimpleDownloadManager/";
	#elif __WIN32
		static const QString DEFAULT_TEMP =
			QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
			"/.SimpleDownloadManager/Downloads/";
		static const QString DEFAULT_SAVE =
			QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
		static const QString DEFAULT_LOG = DEFAULT_TEMP;
	#endif

	static void initDefaultDirs() {
		if(!QDir(DEFAULT_TEMP).exists()) {
			QDir().mkpath(DEFAULT_TEMP);
		}
		if(!QDir(DEFAULT_SAVE).exists()) {
			QDir().mkdir(DEFAULT_LOG);
		}
	}
};