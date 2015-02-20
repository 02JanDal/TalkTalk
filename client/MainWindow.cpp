#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	connect(ui->action_Quit, &QAction::triggered, qApp, &QApplication::quit);
	connect(ui->actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt);
	connect(ui->action_Settings, &QAction::triggered, this, &MainWindow::showSettings);
	connect(ui->action_Connect_to_server, &QAction::triggered, this, &MainWindow::connectToServer);
	connect(ui->action_Disconnect_from_server, &QAction::triggered, this, &MainWindow::disconnectFromServer);
	connect(ui->action_About, &QAction::triggered, this, &MainWindow::showAbout);
	connect(ui->action_Help, &QAction::triggered, this, &MainWindow::showHelp);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::connectToServer()
{
}
void MainWindow::disconnectFromServer()
{
}
void MainWindow::showSettings()
{
}
void MainWindow::showAbout()
{
}
void MainWindow::showHelp()
{
}
