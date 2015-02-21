#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "client/core/ServerConnection.h"
#include "client/core/MonitorModel.h"
#include "client/core/ChannelsModel.h"
#include "client/core/ChannelModel.h"
#include "client/core/UsersModel.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	connect(ui->action_Quit, &QAction::triggered, qApp, &QApplication::quit);
	connect(ui->actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt);
	connect(ui->action_Connect_to_server, &QAction::triggered, this, &MainWindow::connectToServer);
	connect(ui->action_Disconnect_from_server, &QAction::triggered, this, &MainWindow::disconnectFromServer);
	connect(ui->action_Settings, &QAction::triggered, this, &MainWindow::showSettings);
	connect(ui->action_About, &QAction::triggered, this, &MainWindow::showAbout);
	connect(ui->action_Help, &QAction::triggered, this, &MainWindow::showHelp);
	connect(ui->lineEdit, &QLineEdit::returnPressed, this, &MainWindow::messageEntered);
	connect(ui->channelsView, &QTreeView::clicked, this, &MainWindow::channelClicked);

	QMetaObject::invokeMethod(this, "connectToServer", Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::connectToServer()
{
	if (m_server)
	{
		return;
	}
	m_server = new ServerConnection("localhost", 11101, this);

	ui->monitorView->setModel(m_monitorModel = new MonitorModel(m_server, m_server));
	ui->channelsView->setModel(m_channelsModel = new ChannelsModel(m_server, m_server));
	ui->channelsView->expandAll();
	connect(m_channelsModel, &QAbstractItemModel::rowsInserted, ui->channelsView, &QTreeView::expandAll);
	connect(m_channelsModel, &QAbstractItemModel::rowsRemoved, ui->channelsView, &QTreeView::expandAll);
	connect(m_channelsModel, &QAbstractItemModel::rowsMoved, ui->channelsView, &QTreeView::expandAll);
	connect(m_channelsModel, &QAbstractItemModel::modelReset, ui->channelsView, &QTreeView::expandAll);

	connect(m_server, &ServerConnection::connected, this, [this]()
	{
		ui->monitorDock->setEnabled(true);
		ui->channelsDock->setEnabled(true);
		ui->usersDock->setEnabled(true);
		ui->centralwidget->setEnabled(true);
		ui->action_Disconnect_from_server->setEnabled(true);
		ui->action_Connect_to_server->setEnabled(false);
	});
	connect(m_server, &ServerConnection::disconnected, this, [this]()
	{
		ui->monitorDock->setEnabled(false);
		ui->channelsDock->setEnabled(false);
		ui->usersDock->setEnabled(false);
		ui->centralwidget->setEnabled(false);
		ui->action_Disconnect_from_server->setEnabled(false);
		ui->action_Connect_to_server->setEnabled(true);
		for (ChannelModel *model : m_channels.values())
		{
			delete model;
		}
		m_channels.clear();
		m_server->deleteLater();
		m_server = nullptr;
		m_currentChannel = nullptr;
	});

	m_server->connectToHost();
}
void MainWindow::disconnectFromServer()
{
	if (!m_server)
	{
		return;
	}
	m_server->disconnectFromHost();
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

void MainWindow::messageEntered()
{
}

void MainWindow::channelClicked(const QModelIndex &index)
{
	const QString id = m_channelsModel->channelId(index);
	if (id.isNull())
	{
		return;
	}
	if (!m_channels.contains(id))
	{
		m_channels.insert(id, new ChannelModel(m_server, id, this));
	}
	m_currentChannel = m_channels[id];
	ui->chatView->setModel(m_currentChannel);

	if (!m_users.contains(id))
	{
		m_users.insert(id, new UsersModel(m_server, id, this));
	}
	m_currentUsers = m_users[id];
	ui->usersView->setModel(m_currentUsers);
}
