#include "IRCServersPage.h"
#include "ui_IRCServersPage.h"

#include <QDataWidgetMapper>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include <KColumnResizer>
#include <KPageWidget>

IRCServersPage::IRCServersPage(ServerConnection *server, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::IRCServersPage)
{
	ui->setupUi(this);

	connect(ui->applyBtn, &QPushButton::clicked, this, &IRCServersPage::applyClicked);
	connect(ui->removeBtn, &QPushButton::clicked, this, &IRCServersPage::removeClicked);
	connect(ui->newBtn, &QPushButton::clicked, this, &IRCServersPage::newClicked);
	connect(ui->connectBtn, &QPushButton::clicked, this, &IRCServersPage::connectClicked);
	connect(ui->disconnectBtn, &QPushButton::clicked, this, &IRCServersPage::disconnectClicked);

	m_model = new IrcServersModel(server, this);

	m_mapper = new QDataWidgetMapper(this);
	m_mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
	m_mapper->setModel(m_model);
	m_mapper->addMapping(ui->statusLabel, IrcServersModel::StatusSection, "text");
	m_mapper->addMapping(ui->nameEdit, IrcServersModel::NameSection, "text");
	m_mapper->addMapping(ui->realNameEdit, IrcServersModel::RealNameSection, "text");
	m_mapper->addMapping(ui->nickEdit, IrcServersModel::NickSection, "text");
	m_mapper->addMapping(ui->nicksEdit, IrcServersModel::NicksSection, "items");
	m_mapper->addMapping(ui->hostEdit, IrcServersModel::HostSection, "text");
	m_mapper->addMapping(ui->portBox, IrcServersModel::PortSection, "value");
	m_mapper->addMapping(ui->usernameEdit, IrcServersModel::UsernameSection, "text");
	m_mapper->addMapping(ui->passwordEdit, IrcServersModel::PasswordSection, "text");
	m_mapper->addMapping(ui->serversEdit, IrcServersModel::ServersSection, "items");

	connect(ui->nameEdit, &QLineEdit::textChanged, ui->serverBox, &QGroupBox::setTitle);

	ui->serversList->setModel(m_model);
	ui->serverBox->setEnabled(false);
	connect(ui->serversList->selectionModel(), &QItemSelectionModel::currentChanged, m_mapper, &QDataWidgetMapper::setCurrentModelIndex);
	connect(ui->serversList->selectionModel(), &QItemSelectionModel::currentChanged, this, &IRCServersPage::updateConnectButtons);
	connect(m_model, &IrcServersModel::connectedChanged, this, &IRCServersPage::updateConnectButtons);
	connect(m_model, &IrcServersModel::connectedChanged, this, &IRCServersPage::updateConnectButtons);
}

IRCServersPage::~IRCServersPage()
{
	delete ui;
}

KPageWidgetItem *IRCServersPage::createPage(ServerConnection *server)
{
	KPageWidgetItem *item = new KPageWidgetItem(new IRCServersPage(server));
	item->setHeader("IRC Servers");
	item->setName("IRC Servers");
	return item;
}

void IRCServersPage::applyClicked()
{
	m_mapper->submit();
}
void IRCServersPage::removeClicked()
{
	const QModelIndex index = ui->serversList->currentIndex();
	if (!index.isValid())
	{
		return;
	}
	int res = QMessageBox::question(this, tr("Remove"), tr("This will remove %1. You will be disconnected from the network, and all related data will be removed. This can NOT be undone!\n\nDo you really want to continue?").arg(index.sibling(index.row(), IrcServersModel::NameSection).data().toString()),
									QMessageBox::No, QMessageBox::Yes);
	if (res == QMessageBox::No)
	{
		return;
	}
	m_model->remove(ui->serversList->currentIndex());
}

void IRCServersPage::newClicked()
{
	const QString host = QInputDialog::getText(this, tr("New IRC Server"), tr("Please enter the host for the new IRC network you want to create"));
	if (!host.isEmpty())
	{
		m_model->add(host, host);
	}
}

void IRCServersPage::connectClicked()
{
	m_model->connectServer(ui->serversList->currentIndex());
}
void IRCServersPage::disconnectClicked()
{
	m_model->disconnectServer(ui->serversList->currentIndex());
}

void IRCServersPage::updateConnectButtons()
{
	const bool connected = m_model->isConnected(ui->serversList->currentIndex());
	ui->connectBtn->setEnabled(!connected);
	ui->disconnectBtn->setEnabled(connected);
	ui->serverBox->setEnabled(ui->serversList->currentIndex().isValid());
}
