#pragma once

#include <QMainWindow>
#include <QHash>

namespace Ui {
class MainWindow;
}

class ServerConnection;
class MonitorModel;
class ChannelsModel;
class ChannelModel;
class UsersModel;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void connectToServer();
	void disconnectFromServer();

	void showSettings();
	void showAbout();
	void showHelp();

	void messageEntered();
	void channelClicked(const QModelIndex &index);

private:
	Ui::MainWindow *ui;
	ServerConnection *m_server = nullptr;
	MonitorModel *m_monitorModel;
	ChannelsModel *m_channelsModel;

	QHash<QString, ChannelModel *> m_channels;
	QHash<QString, UsersModel *> m_users;
	ChannelModel *m_currentChannel = nullptr;
	UsersModel *m_currentUsers = nullptr;
};
