#include <QSplitter>
#include <QItemDelegate>
#include <QPainter>
#include <QCheckBox>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include "mainwindow.h"
#include "ui_inputdialog.h"

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
	setupTrayIcon();

	model = new DownloadModel(this);
	pmodel = new QSortFilterProxyModel(this);
	pmodel->setSourceModel(model);
	pmodel->setFilterKeyColumn(5);

	//Load previous downloads and connect if not completed
	downloadLog.loadPreviousDownloads(model);
	for(int i = 0; i < model->rowCount(); ++i) {
		Download *download = model->getDownload(i);
		if(download->getState() != Download::COMPLETED)
			connectSignals(download);
	}

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
		if(!download) return;
		Download::DownloadState state = download->getState();
		if(state != Download::PAUSED && state != Download::COMPLETED)
			download->pauseDownload();
	});

	//resume download
	connect(ui->actionResume, &QAction::triggered, this, [this]{
		int row = ui->tableView->currentIndex().row();
		if(row == -1) return;
		Download* download = model->getDownload(row);
		if(!download) return;
		Download::DownloadState state = download->getState();
		if(state == Download::PAUSED && state != Download::COMPLETED)
			download->resumeDownload();
	});

	//delete download
	connect(ui->actionDelete, &QAction::triggered, this,
		&MainWindow::deleteDownload);

	//Menu
	connect(ui->actionQuit, &QAction::triggered, qApp, &QApplication::quit);

	//Hide/Show tray icon
	connect(ui->actionTrayIcon, &QAction::changed, this, [this] {
		if(ui->actionMinTray->isEnabled()) {
			ui->actionMinTray->setChecked(false);
			ui->actionMinTray->setDisabled(true);
			tray->hide();
		} else {
			ui->actionMinTray->setDisabled(false);
			tray->show();
		}
	});
}

MainWindow::~MainWindow() {
	//save window state before exit
	saveState();
	delete ui;
	Download::deleteManager();
}

void MainWindow::newDownload() {
	InputDialog* dialog = new InputDialog("New Download",
		"Enter Download URL: ", this);

	dialog->show();
	connect(dialog, &QDialog::accepted, this, [=]{
		QUrl url = dialog->getVal();
		//add new download to model
		Download* download = new Download(url, model);
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
			model->replaceDownloadLRow(download);
			updateModel(download, DownloadModel::ALL);
			downloadLog.addNewDownload(download);
			QModelIndex index = model->index(model->rowCount()-1, 0);
			ui->tableView->setCurrentIndex(index);
			ui->tableView->selectRow(model->rowCount()-1);

			#ifdef DEBUG
				qDebug() << "New download added to model" << endl;
			#endif

			connectSignals(download);

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

void MainWindow::connectSignals(Download *download) {
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
		//On completion
		if(download->getState() == Download::COMPLETED){
			downloadLog.updateLog(download->getFileName(), false);
			//Move file
			QDir dir;
			dir.rename(download->getFilePath(),
				DefaultDirs::DEFAULT_SAVE+download->getFileName());
		}
	});
	//errors while downloading
	connect(download, &Download::downloadError, this,
			[=](QString message){
		download->pauseDownload();
		QMessageBox::warning(this, "Error!", message);
	});
}

void MainWindow::deleteDownload() {
	int row = ui->tableView->currentIndex().row();
	if(row == -1) return;

	Download* download = model->getDownload(row);
	if(!download) return;
	//Dont resume if the download was paused before pressing delete
	Download::DownloadState stateBefore = download->getState();
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
		QFile file(download->getFilePath());
		if(state == Download::COMPLETED)
			if(!cbox.isChecked())
				downloadLog.updateLog(download->getFileName(), true);
			else {
				file.setFileName(DefaultDirs::DEFAULT_SAVE+
					download->getFileName());
				deleteFile(download->getFileName(), file);
			}
	} else {
		if(stateBefore != Download::PAUSED);
			download->resumeDownload();
	}
}

void MainWindow::deleteFile(QString fileName, QFile &file) {
	if(file.exists()) {
		if(!file.remove()) qDebug() << "Failed To Delete: "
			<< file.fileName();
	} else {
		QMessageBox::warning(this, "File Not Found!",
			"File was already deleted!");
	}
	downloadLog.updateLog(fileName, true);
}

bool MainWindow::updateModel(Download* download,
	DownloadModel::UPDATE_FIELD field) const {

	int row = model->getDownloadIndex(download);
	QModelIndex index = model->index(row, 0);
	model->setData(index, QVariant(field));
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

void MainWindow::setupTrayIcon() {
	trayMenu = new QMenu(this);
	showAction = new QAction("show", this);
	showAction->setEnabled(false);

	trayMenu->addAction(showAction);
	trayMenu->addAction(ui->actionQuit);

	tray = new QSystemTrayIcon(this);
	tray->setIcon(QIcon(":/icons/icons/MainWindow.ico"));
	tray->setContextMenu(trayMenu);
	tray->setToolTip("Simple Download Manager");
	tray->show();

	connect(showAction, &QAction::triggered, this, [this]{
		this->show();
		this->setWindowState(this->windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
	});

	connect(tray, &QSystemTrayIcon::activated, this,
			[this](QSystemTrayIcon::ActivationReason reason){
		if(reason == QSystemTrayIcon::Trigger) {
			if(this->isHidden()) {
				this->show();
				this->setWindowState(Qt::WindowActive);
			} else {
				this->hide();
			}
		}
	});
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

void MainWindow::closeEvent(QCloseEvent *e) {
	QMessageBox::StandardButton res =
		QMessageBox::question( this, "Simple Download Manager",
            tr("Do you want to quit?"), QMessageBox::No | QMessageBox::Yes,
        	QMessageBox::Yes);
    if (res != QMessageBox::Yes) {
        e->ignore();
    } else {
        e->accept();
    }
}

void MainWindow::changeEvent(QEvent *e) {
	if(e->type() == QEvent::WindowStateChange) {
		if(this->isMinimized() && ui->actionMinTray->isChecked()) {
			this->hide();
			this->showAction->setEnabled(true);
			e->ignore();
		} else {
			this->showAction->setEnabled(false);
		}
	}
	QMainWindow::changeEvent(e);
}