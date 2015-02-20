#include "IrcClientConnection.h"

#include <IrcCommandParser>
#include <IrcConnection>
#include <IrcBuffer>
#include <IrcBufferModel>
#include <IrcUserModel>

#include "common/Json.h"
#include "IrcMessageFormatter.h"

IrcServer::IrcServer(const QJsonObject &obj, QObject *parent)
	: AbstractClientConnection(parent)
{
	using namespace Json;
	m_host = ensureIsType<QString>(obj, "host");

	m_parser = new IrcCommandParser(this);
	m_parser->addCommand(IrcCommand::Join, "JOIN <#channel> (<key>)");
	m_parser->addCommand(IrcCommand::Part, "PART (<#channel>) (<message...>)");
	m_parser->addCommand(IrcCommand::Kick, "KICK (<#channel>) <nick> (<reason...>)");
	m_parser->addCommand(IrcCommand::CtcpAction, "ME [target] <message...>");
	m_parser->setTriggers(QStringList() << "/");

	using namespace Json;
	m_connection = new IrcConnection(m_host, this);
	m_connection->setDisplayName(ensureIsType<QString>(obj, "displayName", ""));
	m_connection->setRealName(ensureIsType<QString>(obj, "realName"));
	m_connection->setUserName(ensureIsType<QString>(obj, "userName"));
	m_connection->setNickNames(ensureIsArrayOf<QString>(obj, "nickNames"));
	connect(m_connection, &IrcConnection::connecting, [this]()
	{ emit broadcast("irc:server:" + m_host, "connecting"); });
	connect(m_connection, &IrcConnection::connected, [this]()
	{ emit broadcast("irc:server:" + m_host, "connected"); });
	connect(m_connection, &IrcConnection::disconnected, [this]()
	{ emit broadcast("irc:server:" + m_host, "disconnected"); });

	m_bufferModel = new IrcBufferModel(m_connection);
	connect(m_bufferModel, &IrcBufferModel::added, this, &IrcServer::addedBuffer);
	connect(m_bufferModel, &IrcBufferModel::removed, this, &IrcServer::removedBuffer);
	connect(m_bufferModel, &IrcBufferModel::channelsChanged, m_parser, &IrcCommandParser::setChannels);
	IrcBuffer *serverBuffer = m_bufferModel->add(m_connection->host());
	connect(m_bufferModel, &IrcBufferModel::messageIgnored, serverBuffer, &IrcBuffer::receiveMessage);
	m_bufferIds.insert(serverBuffer, m_host);

	subscribeTo("irc:server:" + m_host);
	subscribeTo("chat:channel:" + m_host);

	emit broadcast("chat", "channel:added", {{"parent", QJsonValue::Null}, {"id", m_host}});
}

void IrcServer::disconnectFromHost()
{
	m_connection->close();
}

void IrcServer::toClient(const QJsonObject &obj)
{
	using namespace Json;
	const QString channel = ensureIsType<QString>(obj, "channel");
	const QString cmd = ensureIsType<QString>(obj, "cmd");
	const QUuid msgId = ensureIsType<QUuid>(obj, "msgId");

	if (channel == ("irc:server:" + m_host))
	{
		if (cmd == "connect")
		{
			if (m_connection->isConnected())
			{
				emit broadcast("irc:server:" + m_host, "connect:error", {{"error", "Already connected"}}, msgId);
			}
			else
			{
				m_connection->open();
			}
		}
		else if (cmd == "disconnect")
		{
			if (m_connection->isConnected())
			{
				m_connection->close();
			}
			else
			{
				emit broadcast("irc:server:" + m_host, "disconnect:error", {{"error", "Not connected"}}, msgId);
			}
		}
	}
	else if (channel == ("chat:channel:" + m_host))
	{
		if (cmd == "send")
		{
			m_parser->setTarget(m_host);
			IrcCommand *cmd = m_parser->parse(ensureIsType<QString>(obj, "msg"));
			m_connection->sendCommand(cmd);
			IrcMessage *msg = cmd->toMessage(m_connection->nickName(), m_connection);
			msg->setProperty("buffer", QVariant::fromValue<IrcBuffer *>(m_bufferIds.key(m_host)));
			messageReceived(msg);
		}
	}
}

void IrcServer::addedBuffer(IrcBuffer *buffer)
{
	connect(buffer, &IrcBuffer::messageReceived, this, &IrcServer::messageReceived);

	IrcUserModel *users = new IrcUserModel(buffer);
	users->setSortMethod(Irc::SortByTitle);
	m_userModels.insert(buffer, users);
	m_bufferIds.insert(buffer, QUuid::createUuid().toString());

	emit broadcast("chat", "channel:added", {{"parent", m_host}, {"id", Json::toJson(m_bufferIds[buffer])}});
}
void IrcServer::removedBuffer(IrcBuffer *buffer)
{
	emit broadcast("chat:" + m_bufferIds[buffer], "removed");
	emit broadcast("chat", "channel:removed", {{"id", m_bufferIds[buffer]}});
	m_bufferIds.remove(buffer);
	delete m_userModels.take(buffer);
}
void IrcServer::messageReceived(IrcMessage *msg)
{
	IrcBuffer *buffer = msg->property("buffer").isNull() ? qobject_cast<IrcBuffer *>(sender())
														 : msg->property("buffer").value<IrcBuffer *>();
	Q_ASSERT(buffer);
	emit broadcast("chat:" + m_bufferIds[buffer], "message", {
					   {"content", IrcMessageFormatter::formatMessage(msg)},
					   {"from", msg->nick()}
				   });
}

IrcClientConnection::IrcClientConnection(QObject *parent)
	: AbstractClientConnection(parent)
{
	subscribeTo("irc:servers");
}

void IrcClientConnection::toClient(const QJsonObject &obj)
{
	using namespace Json;
	const QString channel = ensureIsType<QString>(obj, "channel");
	const QString cmd = ensureIsType<QString>(obj, "cmd");
	const QUuid msgId = ensureIsType<QUuid>(obj, "msgId");

	if (channel == "irc:servers")
	{
		if (cmd == "add")
		{
			const QString host = ensureIsType<QString>(obj, "host");
			if (m_servers.contains(host))
			{
				emit broadcast("irc:servers", "add:error", {{"error", "Host already exists"}}, msgId);
			}
			else
			{
				IrcServer *server = new IrcServer(obj, this);
				m_servers.insert(host, server);
				emit newIrcServer(server);
				emit broadcast("irc:servers", "added", {{"host", host}});
				emit broadcast("irc:servers", "add:success", QJsonObject(), msgId);
			}
		}
		else if (cmd == "remove")
		{
			const QString host = ensureIsType<QString>(obj, "host");
			if (m_servers.contains(host))
			{
				IrcServer *server = m_servers.take(host);
				server->disconnectFromHost();
				server->deleteLater();
				emit broadcast("irc:servers", "removed", {{"host", host}});
				emit broadcast("irc:servers", "remove:success", QJsonObject(), msgId);
			}
			else
			{
				emit broadcast("irc:servers", "remove:error", {{"error", "Unknown host"}}, msgId);
			}
		}
	}
}
