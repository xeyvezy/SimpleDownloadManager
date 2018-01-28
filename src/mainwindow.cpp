#include "mainwindow.h"
#include "ui_inputdialog.h"
#include <QSplitter> 
#include <QItemDelegate> 
#include <QPainter>
#include <QCheckBox>
#include <QDesktopServices>
#include <QDesktopWidget>

class TableDelegate : public QItemDelegate {
	public:
		explicit TableDelegate(QObject* parent=0):QItemDelegate(parent) {}

		void paint(QPainter* painter, const QStyleOptionViewItem& option,
			const QModelIndex& index) const override {
			
			QStyleOptionViewItem option2 = option;
			option2.state = option.state & (~QStyle::State_HasFocus);

			if(index.column() != 2) {
				QItemDelegate::paint(painter, option2, index);
				return;
			}

			//Draw ProgressBar
			QStyleOptionProgressBar progressbar;
			progressbar.state = option2.state;
			progressbar.rect = option2.rect;
			progressbar.minimum = 0;
			progressbar.maximum = 100;
			progressbar.textAlignment = Qt::AlignVCenter|Qt::AlignRight;
			progressbar.textVisible = true;
			progressbar.palette = option2.palette;

			//highlight on slection
			if(progressbar.state & QStyle::State_Selected) {
				if(progressbar.state & QStyle::State_Active)
					painter->fillRect(progressbar.rect, option2.palette.highlight());
				else {
					painter->fillRect(progressbar.rect, 
						option2.palette.brush(QPalette::Inactive, QPalette::Highlight));
				}
			}
			//set progressbar value & text
			progressbar.progress = index.data().toInt(); 
			progressbar.text = index.data().toString()+"%";
 
			//draw
			qApp->style()->drawControl(QStyle::CE_ProgressBar,  
				&progressbar, painter);
		}
};

class InputDialog : public QDialog {

public:
	explicit InputDialog(QString title, QString label, QWidget* parent = 0):
		QDialog(parent), ui(new  Ui::Dialog) {
		
		ui->setupUi(this);
		this->setWindowTitle(title);
		this->setModal(true);
		ui->label->setText(label);
	}

	QString getVal() {
		return ui->lineEdit->text();
	}

	~InputDialog() {
		delete ui;
	}

private:
	Ui::Dialog* ui;

};

MainWindow::MainWindow(QWidget* parent):
	QMainWindow(parent), ui(new Ui::MainWindow) {
	
	QResource::registerResource("resource.rcc");
	ui->setupUi(this);
	setWindowTitle("SimpleDownloadManager");

	model = new DownloadModel(this);  
	pmodel = new QSortFilterProxyModel(this);
	pmodel->setSourceModel(model);
	pmodel->setFilterKeyColumn(5); 

	setupTableView();
	setupListView();
	//Restore previous window state
	restoreState();

	//Filter Downloads with listview
	connect(ui->listView, &QListView::clicked, this, [this]{
		QModelIndex index = ui->listView->currentIndex();
		switch(index.row()) {
			case 0:
				pmodel->setFilterFixedString("");
				return;
			case 1:
				pmodel->setFilterFixedString("DOWNLOADING");
				return;
			case 2:
				pmodel->setFilterFixedString("PAUSED");
				return;
			case 3:
				pmodel->setFilterFixedString("COMPLETED");
				return;
			default:
				return;
		}
	});
	
	//Create a new download
	connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newDownload);
	
	//pause download
	connect(ui->actionPause, &QAction::triggered, this, [this]{
		int row = ui->tableView->currentIndex().row();
		if(row == -1) return;
		Download* download = model->getDownload(row);
		Download::DownloadState state = download->getState();
	
	
		if(state != Download::PAUSED && state != Download::COMPLETED)
			download->pauseDownload();
	});

	//resume download
	connect(ui->actionResume, &QAction::triggered, this, [this]{
		int row = ui->tableView->currentIndex().row();
		if(row == -1) return;
		Download* download = model->getDownload(row);
		Download::DownloadState state = download->getState();
		if(state == Download::PAUSED && state != Download::COMPLETED)
			download->resumeDownload();
	});

	//delete download
	//If the download is incomplete the file is deleted 
	//else the user gets option to delete it
	connect(ui->actionDelete, &QAction::triggered, this, 
		&MainWindow::deleteDownload);
}

MainWindow::~MainWindow() {
	//save window state before exit
	saveState();
	delete ui;
	Download::deleteManager();
}         

void MainWindow::setupTableView() {
	TableDelegate *delegate = new TableDelegate(this);
	ui->tableView->setModel(pmodel);
	ui->tableView->setItemDelegate(delegate);
	ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableView->horizontalHeader()->setHighlightSections(false); 
	ui->tableView->verticalHeader()->setVisible(false);
	ui->tableView->setShowGrid(false);
	ui->tableView->setSortingEnabled(false);
	ui->tableView->resizeColumnToContents(0);
}

void MainWindow::setupListView() {
	QStandardItemModel* smodel = new QStandardItemModel(4, 1, this);
	smodel->setData(smodel->index(0,0), "All");
	smodel->setData(smodel->index(1,0), "Downloading");
	smodel->setData(smodel->index(2,0), "Paused");
	smodel->setData(smodel->index(3,0), "Completed");

	ui->listView->setModel(smodel);
	ui->listView->setSpacing(2);
	ui->listView->setCurrentIndex(smodel->index(0,0));
}

void MainWindow::newDownload() {
	InputDialog* dialog = new InputDialog("New Download", 
		"Enter Download URL: ", this);
		
	dialog->show();
	connect(dialog, &QDialog::accepted, this, [=]{
		QUrl url = dialog->getVal();
		//add new download to model
		Download* download = new Download(url, 1, model);
		//add a new row with empty download - (to display getting info)
		model->insertRow(model->rowCount());

		#ifdef DEBUG
			qDebug() << "New Download: " << endl
				<< "Url: " << url << " Row:" << model->rowCount() << endl;
		#endif

		bool start = 1;
		QEventLoop loop;
		connect(download, &Download::downloadInfoComplete, this, 
			[=,&loop,&start](QString message, bool error){
			if(error) {
				QMessageBox::warning(this, "Error!", message);
				download->deleteLater();
				//remove newly added row on error
				model->removeRow(model->rowCount()-1);
				return;
			} 
			
			//update the empty download with new download
			model->addDownload(download);
			updateModel(download, DownloadModel::ALL);

			#ifdef DEBUG
				qDebug() << "New download added to model" << endl;
			#endif

			//updating progressbar
			connect(download, &Download::progressUpdate, this, [=]{
				updateModel(download, DownloadModel::PROGRESS);
			});
			//update speed
			connect(download, &Download::downloadSpeed, this, [=]{
				updateModel(download, DownloadModel::SPEED);
			});
			//update size if updateDownloadInfo fails
			connect(download, &Download::fileSizeUpdate, this, [=]{
				updateModel(download, DownloadModel::SIZE);
			});
			//update download state
			connect(download, &Download::stateChanged, this, [=]{
				updateModel(download, DownloadModel::STATE);
			});
			//errors while downloading
			connect(download, &Download::downloadError, this, 
				[=](QString message){
				download->pauseDownload();
				QMessageBox::warning(this, "Error!", message);
			});
			start = 0;
			loop.exit();
		});
		
		download->getDownloadInfo(); 
		loop.exec();
		//Don't start download till downloadInfoComplete slot is finished
		//Don't start download on error
		if(!start) download->startDownload(); 
	});
}

void MainWindow::deleteDownload() {
	int row = ui->tableView->currentIndex().row(); 
	if(row == -1) return;
	
	Download* download = model->getDownload(row);
	if(!download) return; 
	download->pauseDownload();
	Download::DownloadState state = download->getState();
		
	QMessageBox msgBox(this); 
	QCheckBox cbox;
	cbox.setChecked(false);
	if(state == Download::COMPLETED) { 
		cbox.setText("Delete the downloaded file.");		
		msgBox.setCheckBox(&cbox); 
		cbox.setFocusPolicy(Qt::FocusPolicy::NoFocus);
	}
	msgBox.setText("Do you really want to delete the download?");
	msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);	
	msgBox.setIcon(QMessageBox::Question);
	int selection =	msgBox.exec();
	if(selection == QMessageBox::Yes) {
		model->removeRow(row);
		download->deleteLater();
		if(state == Download::COMPLETED) 
			if(!cbox.isChecked()) return;
			
		QFile file(download->getFileName());		
		if(file.exists())
			file.remove();
	} else 
		download->resumeDownload();
}
   
bool MainWindow::updateModel(Download* download, 
	DownloadModel::UPDATE_FIELD field) const { 
	
	int row = model->getDownloadIndex(download);
	QModelIndex index = model->index(row, 0);
	model->setData(index, QVariant(field));
} 

void MainWindow::saveState() {
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, 
		"AB", "Simple Download Manager"); 
	QByteArray table_state = ui->tableView->horizontalHeader()->saveState();
	QByteArray msplitter_state = ui->splitter_2->saveState();
	QByteArray ssplitter_state = ui->splitter->saveState();

	settings.beginGroup("MainWindow");
	settings.setValue("size", size());
	settings.setValue("pos", pos());
	settings.setValue("table_state", table_state);
	settings.setValue("msplitter_state", msplitter_state);
	settings.setValue("ssplitter_state", ssplitter_state);
	settings.endGroup();
}

void MainWindow::restoreState() {
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, 
		"AB", "Simple Download Manager"); 
	settings.beginGroup("MainWindow");
	//key, default data
	resize(settings.value("size", QSize(800, 400)).toSize());
	move(settings.value("pos", QPoint(200, 200)).toPoint());
	
	ui->tableView->horizontalHeader()->restoreState(settings.value("table_state").toByteArray());
	ui->splitter_2->restoreState(settings.value("msplitter_state").toByteArray());
	ui->splitter->restoreState(settings.value("ssplitter_state").toByteArray());

	settings.endGroup();
}

 