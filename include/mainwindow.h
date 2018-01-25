#include "ui_mainwindow.h"
#include "downloadmodel.h" 
#include <QResource>
#include <QSortFilterProxyModel>
#include <QStandardItemModel> 
 
class MainWindow : public QMainWindow {

public:
	explicit MainWindow(QWidget* parent = 0);
	~MainWindow();

private:
	Ui::MainWindow* ui;
	DownloadModel *model;
	QSortFilterProxyModel* pmodel;

	//calls setData
	bool updateModel(Download* download, 
		DownloadModel::UPDATE_FIELD field) const;
	void setupTableView();
	void setupListView();

private slots:
	//New Action
	void newDownload(); 
	void deleteDownload();
}; 