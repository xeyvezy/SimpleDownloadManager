#include "downloadmodel.h"
#include <algorithm>
#include <QMessageBox>
 
const QString DownloadModel::HEADERS[COL_CNT] = {"Id", "Name", "Progress", "Size"}; 

DownloadModel::DownloadModel(QObject* parent):
	QAbstractTableModel(parent), downloadCount(0) {
	  
}
 
int DownloadModel::rowCount(const QModelIndex& parent) const {
	return downloadCount;
}  

int DownloadModel::columnCount(const QModelIndex& parent) const {
	return COL_CNT;
}

QVariant DownloadModel::data(const QModelIndex& index, int role) const {
	if(!index.isValid()) return QVariant();
	if(role == Qt::TextAlignmentRole)  
		return int(Qt::AlignVCenter|Qt::AlignRight); 
	else if(role == Qt::DisplayRole) {
		Download* download = downloads.at(index.row());
		if(!download) return QVariant();
		switch(index.column()) {
			case 0:
				return QVariant(download->getId()+1);
			case 1:
				return QVariant(download->getFileName());
			case 2:
				return QVariant(download->getProgress()); 
			case 3:
				return QVariant(download->getFileSize());
		} 
	}
	return QVariant();
} 

QVariant DownloadModel::headerData(int section, Qt::Orientation orientation, 
		int role) const {
	
	if(role != Qt::DisplayRole || (orientation != Qt::Orientation::Horizontal)) 
		return QVariant();
	
	switch(section) {
		case 0:
			return QVariant(HEADERS[0]);
		case 1:
			return QVariant(HEADERS[1]);
		case 2:
			return QVariant(HEADERS[2]);
		case 3:
			return QVariant(HEADERS[3]);
	}
	return QVariant();
}

/**
 * QVariant
 * 0 - progressbar
 * 1 - speed
 */
bool DownloadModel::setData(const QModelIndex& index, const QVariant& value,
	int role) {

	if(!index.isValid() || role != Qt::EditRole) return 0;

	int cond = value.toInt();

	QModelIndex index2;

	switch(cond) {
		case PROGRESS:
			index2 = QAbstractItemModel::createIndex(index.row(), 2);
			break;
		case SIZE:
			index2 = QAbstractItemModel::createIndex(index.row(), 3);
			break;
	}
 
	if(index2.isValid()) {  
		emit dataChanged(index2, index2);
		return 1;
	} 

	return 0;
}

void DownloadModel::sort(int column, Qt::SortOrder order) {
	//sort by id only for now
	if(column != 0) return;
	std::sort(downloads.begin(), downloads.end(), 
		[&](const Download* a, const Download* b)->bool {
		
		if(order & Qt::SortOrder::DescendingOrder) return a->getId() > b->getId();
		else return a->getId() < b->getId();
	});
	//update the whole view 
	emit layoutChanged();
} 

bool DownloadModel::insertRows(int row, int count, const QModelIndex& parent) {
	beginInsertRows(parent, row, row+(count-1));
	endInsertRows();
	return true;
}

void DownloadModel::addDownload(Download* download) {

	downloads.append(download);
	insertRow(downloadCount++);
} 