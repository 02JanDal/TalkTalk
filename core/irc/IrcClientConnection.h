#pragma once

#include "core/AbstractClientConnection.h"

#include <QHash>

class IrcCommandParser;
class IrcConnection;
class IrcBufferModel;
class IrcBuffer;
class IrcUserModel;
class IrcMessage;
class IrcUser;

class IrcServer : public AbstractClientConnection
{
	Q_OBJECT
public:
	explicit IrcServer(const QJsonObject &settings, QObject *parent = nullptr);

	void ready() override;

	void disconnectFromHost();

protected:
	void toClient(const QJsonObject &obj) override;

private slots:
	void addedBuffer(IrcBuffer *buffer);
	void removedBuffer(IrcBuffer *buffer);
	void addedUser(IrcUser *user);
	void removedUser(IrcUser *user);
	void messageReceived(IrcMessage *msg);

private:
	QString m_host;
	IrcCommandParser *m_parser;
	IrcConnection *m_connection;
	IrcBufferModel *m_bufferModel;
	QHash<IrcBuffer *, IrcUserModel *> m_userModels;
	QHash<IrcBuffer *, QString> m_bufferIds;
};

class IrcClientConnection : public AbstractClientConnection
{
	Q_OBJECT
public:
	explicit IrcClientConnection(QObject *parent = nullptr);

protected:
	void toClient(const QJsonObject &obj) override;

private:
	QHash<QString, IrcServer *> m_servers;
};
