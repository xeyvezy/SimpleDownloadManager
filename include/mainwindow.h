#include <QResource>
#include <QSortFilterProxyModel>
#include <QMessageBox>
#include <QSettings>
#include <QStandardItemModel>
#include "ui_mainwindow.h"
#include "downloadmodel.h"
#include "downloadLog.h"

class MainWindow : public QMainWindow {

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private:
	Ui::MainWindow *ui;
	DownloadModel *model;
	QSortFilterProxyModel *pmodel;
	DownloadLog downloadLog;

	//Connect download signals to appropriate slots
	void connectSignals(Download *download);
	//calls setData
	bool updateModel(Download* download,
		DownloadModel::UPDATE_FIELD field) const;
	void deleteFile(QString fileName, QFile &file);
	void setupTableView();
	void setupListView();
	void saveState();
	void restoreState();

private slots:
	//New Action
	void newDownload();
	void deleteDownload();
};