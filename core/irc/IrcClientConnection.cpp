#include "IrcClientConnection.h"

#include <IrcCommandParser>
#include <IrcConnection>
#include <IrcBuffer>
#include <IrcBufferModel>
#include <IrcUserModel>
#include <IrcUser>

#include "common/Json.h"
#include "IrcMessageFormatter.h"

IrcServer::IrcServer(const QJsonObject &obj, QObject *parent)
	: AbstractClientConnection(parent)
{
	using namespace Json;
	m_host = ensureString(obj, "host");

	m_parser = new IrcCommandParser(this);
	m_parser->addCommand(IrcCommand::Join, "JOIN <#channel> (<key>)");
	m_parser->addCommand(IrcCommand::Part, "PART (<#channel>) (<message...>)");
	m_parser->addCommand(IrcCommand::Kick, "KICK (<#channel>) <nick> (<reason...>)");
	m_parser->addCommand(IrcCommand::CtcpAction, "ME [target] <message...>");
	m_parser->setTriggers(QStringList() << "/");

	using namespace Json;
	m_connection = new IrcConnection(m_host, this);
	m_connection->setDisplayName(ensureString(obj, "displayName", ""));
	m_connection->setRealName(ensureString(obj, "realName"));
	m_connection->setUserName(ensureString(obj, "userName"));
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
	subscribeTo("chat:channels");
	subscribeTo("chat:channel:" + m_host);
}

void IrcServer::ready()
{
	emit broadcast("chat:channels", "added", {{"parent", QJsonValue::Null}, {"id", m_host}});
}

void IrcServer::disconnectFromHost()
{
	m_connection->close();
}

void IrcServer::toClient(const QJsonObject &obj)
{
	using namespace Json;
	const QString channel = ensureString(obj, "channel");
	const QString cmd = ensureString(obj, "cmd");
	const QUuid msgId = ensureUuid(obj, "msgId");

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
	else if (channel.startsWith("chat:channel:") && m_bufferIds.values().contains(QString(channel).remove("chat:channel:")))
	{
		const QString bufferId = QString(channel).remove("chat:channel:");
		IrcBuffer *buffer = m_bufferIds.key(bufferId);
		if (cmd == "send")
		{
			m_parser->setTarget(buffer->title());
			IrcCommand *cmd = m_parser->parse(ensureString(obj, "msg"));
			m_connection->sendCommand(cmd);

			IrcMessage *msg = cmd->toMessage(m_connection->nickName(), m_connection);
			msg->setProperty("buffer", QVariant::fromValue<IrcBuffer *>(buffer));
			messageReceived(msg);
		}
		else if (cmd == "wantInfo")
		{
			emit broadcast("chat:channel:" + bufferId, "info", {{"title", buffer->title()},
																{"id", bufferId},
																{"parent", bufferId == m_host ? "" : m_host},
																{"active", buffer->isActive()},
																{"type", buffer->isChannel() ? "pound" : "user"}},
						   msgId);
		}
		else if (cmd == "wantUsers")
		{
			QJsonArray users;
			for (const IrcUser *user : m_userModels[buffer]->users())
			{
				QString mode;
				if (user->mode() == "o")
				{
					mode = "operator";
				}
				else if (user->mode() == "voice")
				{
					mode = "voice";
				}
				users.append(QJsonObject({
											 {"status", user->isAway() ? "away" : "normal"},
											 {"mode", mode},
											 {"name", user->name()}
										 }));
			}
			emit broadcast("chat:channel:" + bufferId, "users", {{"users", users}}, msgId);
		}
	}
	else if (channel == "chat:channels")
	{
		if (cmd == "list")
		{
			QJsonArray array;
			for (const QString &id : m_bufferIds.values())
			{
				array.append(QJsonObject({{"id", id},
										  {"parent", (m_host == id) ? "" : m_host}}));
			}
			emit broadcast("chat:channels", "all", {{"channels", array}}, msgId);
		}
	}
}

void IrcServer::addedBuffer(IrcBuffer *buffer)
{
	connect(buffer, &IrcBuffer::messageReceived, this, &IrcServer::messageReceived);

	IrcUserModel *users = new IrcUserModel(buffer);
	connect(users, &IrcUserModel::added, this, &IrcServer::addedUser);
	connect(users, &IrcUserModel::removed, this, &IrcServer::removedUser);
	m_userModels.insert(buffer, users);
	m_bufferIds.insert(buffer, QUuid::createUuid().toString());

	subscribeTo("chat:channel:" + m_bufferIds[buffer]);
	emit broadcast("chat:channels", "added", {{"parent", m_host}, {"id", Json::toJson(m_bufferIds[buffer])}});
}
void IrcServer::removedBuffer(IrcBuffer *buffer)
{
	emit broadcast("chat:" + m_bufferIds[buffer], "removed");
	emit broadcast("chat:channels", "removed", {{"id", m_bufferIds[buffer]}});
	m_bufferIds.remove(buffer);
	delete m_userModels.take(buffer);
}
void IrcServer::addedUser(IrcUser *user)
{
	QString mode;
	if (user->mode() == "o")
	{
		mode = "operator";
	}
	else if (user->mode() == "voice")
	{
		mode = "voice";
	}
	emit broadcast("chat:channel:" + m_bufferIds[m_userModels.key(qobject_cast<IrcUserModel *>(sender()))],
			"users:added", {{"status", user->isAway() ? "away" : "normal"},
			{"mode", mode},
			{"name", user->name()}
							});
}
void IrcServer::removedUser(IrcUser *user)
{
	emit broadcast("chat:channel:" + m_bufferIds[m_userModels.key(qobject_cast<IrcUserModel *>(sender()))],
			"users:removed", {{"name", user->name()}});
}
void IrcServer::messageReceived(IrcMessage *msg)
{
	IrcBuffer *buffer = msg->property("buffer").isNull() ? qobject_cast<IrcBuffer *>(sender())
														 : msg->property("buffer").value<IrcBuffer *>();
	Q_ASSERT(buffer);
	const QString type = IrcMessageFormatter::messageType(msg);
	if (type.isEmpty())
	{
		return;
	}
	emit broadcast("chat:channel:" + m_bufferIds[buffer], "message", {
					   {"content", IrcMessageFormatter::messageContent(msg)},
					   {"from", IrcMessageFormatter::messageSource(msg)},
					   {"type", type},
					   {"timestamp", msg->timeStamp().toMSecsSinceEpoch()}
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
	const QString channel = ensureString(obj, "channel");
	const QString cmd = ensureString(obj, "cmd");
	const QUuid msgId = ensureUuid(obj, "msgId");

	if (channel == "irc:servers")
	{
		if (cmd == "add")
		{
			const QString host = ensureString(obj, "host");
			if (m_servers.contains(host))
			{
				emit broadcast("irc:servers", "add:error", {{"error", "Host already exists"}}, msgId);
			}
			else
			{
				IrcServer *server = new IrcServer(obj, this);
				m_servers.insert(host, server);
				emit newConnection(server);
				emit broadcast("irc:servers", "added", {{"host", host}});
				emit broadcast("irc:servers", "add:success", QJsonObject(), msgId);
			}
		}
		else if (cmd == "remove")
		{
			const QString host = ensureString(obj, "host");
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
