#pragma once

#include <QWidget>

#include "client/core/IrcServersModel.h"

namespace Ui {
class IRCServersPage;
}

class QDataWidgetMapper;
class KPageWidgetItem;

class IRCServersPage : public QWidget
{
	Q_OBJECT
public:
	explicit IRCServersPage(ServerConnection *server, QWidget *parent = 0);
	~IRCServersPage();

	static KPageWidgetItem *createPage(ServerConnection *server);

private slots:
	void applyClicked();
	void removeClicked();
	void newClicked();
	void connectClicked();
	void disconnectClicked();

	void updateConnectButtons();

private:
	Ui::IRCServersPage *ui;

	IrcServersModel *m_model;

	QDataWidgetMapper *m_mapper;
};
