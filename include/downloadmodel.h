#pragma once

#include "download.h"
#include <QAbstractTableModel>
#include <QModelIndex>

//Custom Model
class DownloadModel : public QAbstractTableModel {

public:  
	static constexpr int COL_CNT = 4;
	static const QString HEADERS[COL_CNT];

	enum UPDATE_FIELD {
		PROGRESS = 0x0000,
		SPEED = 0x0001,
		SIZE = 0x0002
	}; 

	DownloadModel(QObject* parent = 0);

	//overriding virtual methods from QAbstractDownloadModel
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, 
		int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orienttation, 
		int role = Qt::DisplayRole) const override;
	void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
	bool setData(const QModelIndex& index, const QVariant& value, 
		int role = Qt::EditRole) override;
	bool insertRows(int row, int count, 
		const QModelIndex& parent = QModelIndex()) override;
	void addDownload(Download* download);

	Download* getDownload(int i) const {
		if(i < downloads.size()) return downloads.at(i);
		return nullptr;
	}

	int getDownloadIndex(Download* download) const {
		int index = downloads.indexOf(download);
	}

private:
	QList<Download*> downloads; 
	int downloadCount; //row count
};