#include <QResource>
#include <QSortFilterProxyModel>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QStandardItemModel>
#include "ui_mainwindow.h"
#include "downloadmodel.h"
#include "downloadLog.h"

class MainWindow : public QMainWindow {

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

protected:
	void changeEvent(QEvent *e) override;
	void closeEvent(QCloseEvent *e) override;

private:
	Ui::MainWindow *ui;
	DownloadModel *model;
	QSortFilterProxyModel *pmodel;
	DownloadLog downloadLog;

	QSystemTrayIcon *tray;
	QMenu *trayMenu;
	QAction *showAction;

	//Connect download signals to appropriate slots
	void connectSignals(Download *download);
	//calls setData
	bool updateModel(Download* download,
		DownloadModel::UPDATE_FIELD field) const;
	void deleteFile(Download *download, QFile &file, bool checked);
	void setupTableView();
	void setupListView();
	void setupTrayIcon();
	void saveState();
	void restoreState();

private slots:
	//New Action
	void newDownload();
	void deleteDownload();
};