#include "ui_mainwindow.h"
#include "downloadmodel.h" 
#include <QResource>

class MainWindow : public QMainWindow {

public:
	explicit MainWindow(QWidget* parent = 0);

private:
	Ui::MainWindow* ui;
	DownloadModel *model;

	//calls setData
	bool updateModel(Download* download, 
		DownloadModel::UPDATE_FIELD field) const;

private slots:
	//New Action
	void newDownload(); 
}; 