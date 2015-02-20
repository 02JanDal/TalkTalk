#pragma once

#include "AbstractClientConnection.h"

#include <QHash>

class IrcCommandParser;
class IrcConnection;
class IrcBufferModel;
class IrcBuffer;
class IrcUserModel;
class IrcMessage;

class IrcServer : public AbstractClientConnection
{
	Q_OBJECT
public:
	explicit IrcServer(const QJsonObject &settings, QObject *parent = nullptr);

	void disconnectFromHost();

protected:
	void toClient(const QJsonObject &obj) override;

private slots:
	void addedBuffer(IrcBuffer *buffer);
	void removedBuffer(IrcBuffer *buffer);
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

signals:
	void newIrcServer(IrcServer *server);

protected:
	void toClient(const QJsonObject &obj) override;

private:
	QHash<QString, IrcServer *> m_servers;
};
