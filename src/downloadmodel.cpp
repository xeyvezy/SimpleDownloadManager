#include "downloadmodel.h"
#include <algorithm>
#include <QMessageBox>

const QString DownloadModel::HEADERS[COL_CNT] = {"Id", "Name", "Progress",
	"Speed/s", "Eta", "Size", "State"};
const QString DownloadModel::DEFAULT_TEXT = "Getting Download Info....";

DownloadModel::DownloadModel(QObject* parent):
	QAbstractTableModel(parent), downloadCount(0) {}

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
		if(!download) { //Display getting info if download in nullptr
			if(index.column() == 1)
				return QVariant(DEFAULT_TEXT);
		} else {
			switch(index.column()) {
				case 0:
					return QVariant(index.row()+1);
				case 1:
					return QVariant(download->getFileName());
				case 2:
					return QVariant(download->getProgress());
				case 3:
					return QVariant(download->getSpeedString());
				case 4:
					return QVariant(download->getEtaString());
				case 5:
					return QVariant(download->getFileSizeString());
				case 6:
					return QVariant(download->getStateString());
			}
		}
	}
	return QVariant();
}

QVariant DownloadModel::headerData(int section, Qt::Orientation orientation,
		int role) const {

	if(role != Qt::DisplayRole || (orientation != Qt::Orientation::Horizontal))
		return QVariant();

	if(section > COL_CNT) return QVariant();
	else return QVariant(HEADERS[section]);
}

bool DownloadModel::setData(const QModelIndex& index, const QVariant& value,
	int role) {

	if(!index.isValid() || role != Qt::EditRole) return 0;

	int cond = value.toInt();

	QModelIndex index1;
	QModelIndex index2;

	//progress bar start(index1)=row,2 & end(index2)=row,3 so on..
	switch(cond) {
		case PROGRESS:
			index1 = QAbstractItemModel::createIndex(index.row(), 2);
			index2 = QAbstractItemModel::createIndex(index.row(), 3);
			break;
		case SPEED:
			index1 = index2;
			index2 = QAbstractItemModel::createIndex(index.row(), 4);
			break;
		case ETA:
			index1 = index2;
			index2 = QAbstractItemModel::createIndex(index.row(), 5);
		case SIZE:
			index1 = index2;
			index2 = QAbstractItemModel::createIndex(index.row(), 6);
			break;
		case STATE:
			index1 = index2;
			index2 = QAbstractItemModel::createIndex(index.row(), 7);
			break;
		case ALL:
			index2 = QAbstractItemModel::createIndex(index.row(), 7);
			index1 = QAbstractItemModel::createIndex(index.row(), 0);
			break;
	}

	emit dataChanged(index1, index2);
	return 1;
}

bool DownloadModel::insertRows(int row, int count, const QModelIndex& parent) {
	beginInsertRows(parent, row, row+(count-1));
	endInsertRows();
	Download* download = Q_NULLPTR;
	downloads.append(download);
	downloadCount++;
	return true;
}

bool DownloadModel::removeRows(int row, int count, const QModelIndex &parent) {
	if(row > downloadCount) return false;

	beginRemoveRows(parent, row, row+(count-1));
	endRemoveRows();
	downloads.removeAt(row);
	downloadCount--;
	return true;
}

void DownloadModel::replaceDownloadLRow(Download* download) {

	Download **dwnLRow = &downloads[downloadCount-1];
	if(*dwnLRow) {
		qDebug() << "Warning a valid download exists." << endl;
		(*dwnLRow)->deleteLater();
		dwnLRow = Q_NULLPTR;
	}
	*dwnLRow = download;
}