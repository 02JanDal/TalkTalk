#include "IrcClientConnection.h"

#include <IrcCommandParser>
#include <IrcConnection>
#include <IrcBuffer>
#include <IrcBufferModel>
#include <IrcUserModel>
#include <IrcUser>

#include "common/Json.h"
#include "core/SyncableList.h"
#include "IrcMessageFormatter.h"

Q_LOGGING_CATEGORY(IRC, "core.irc")

IrcWrappedChannel::IrcWrappedChannel(IrcBuffer *buffer, const QString &parentId, QObject *parent)
	: QObject(parent), m_buffer(buffer), m_parentId(parentId)
{
}
QObject *IrcWrappedChannel::buffer() const
{
	return m_buffer;
}
QString IrcWrappedChannel::type() const
{
	return m_buffer->isChannel() ? "pound" : "user";
}

class SyncableUsersList : public SyncableQObjectList
{
	Q_OBJECT
public:
	explicit SyncableUsersList(IrcUserModel *model, const QString &bufferId)
		: SyncableQObjectList("chat:channel:" + bufferId, "users", "id", flags_noExternal(), model)
	{
		addPropertyMapping("away", "status");
		addPropertyMapping("mode", "mode");
		addPropertyMapping("name", "name");
		addPropertyMapping("objectName", "id");

		connect(model, &IrcUserModel::added, this, &SyncableUsersList::addedUser);
		connect(model, &IrcUserModel::removed, this, &SyncableUsersList::removedUser);

		blockSignals(true);
		for (IrcUser *user : model->users())
		{
			addedUser(user);
		}
		blockSignals(false);
	}

private:
	QVariant transformFromList(const QString &property, const QVariant &value) const override
	{
		if (property == "status")
		{
			return value.toBool() ? "away" : "normal";
		}
		else if (property == "mode")
		{
			if (value.toString() == "o")
			{
				return "operator";
			}
			else if (value.toString() == "v")
			{
				return "voice";
			}
			return value;
		}
		else
		{
			return value;
		}
	}

private slots:
	void addedUser(IrcUser *user)
	{
		user->setObjectName(QUuid::createUuid().toString());
		add(user);
	}
	void removedUser(IrcUser *user)
	{
		remove(user);
	}
};

IrcServer::IrcServer(const QString &displayName, const QString &host, QObject *parent)
	: AbstractClientConnection(parent)
{
	m_parser = new IrcCommandParser(this);
	m_parser->addCommand(IrcCommand::Join, "JOIN <#channel> (<key>)");
	m_parser->addCommand(IrcCommand::Part, "PART (<#channel>) (<message...>)");
	m_parser->addCommand(IrcCommand::Kick, "KICK (<#channel>) <nick> (<reason...>)");
	m_parser->addCommand(IrcCommand::CtcpAction, "ME [target] <message...>");
	m_parser->setTolerant(true);
	m_parser->setTriggers(QStringList() << "/");

	m_channelsList = new SyncableQObjectList("chat:channels", "", "id", SyncableList::flags_noExternal(), this);
	m_channelsList->addPropertyMapping("id", "id");
	m_channelsList->addPropertyMapping("parentId", "parent");
	m_channelsList->addPropertyMapping("buffer", "title", "name");
	m_channelsList->addPropertyMapping("buffer", "active", "active");
	m_channelsList->addPropertyMapping("type", "type");

	m_connection = new IrcConnection(host, this);
	m_connection->setDisplayName(displayName);
	m_connection->setHost(host);
	connect(m_connection, &IrcConnection::connecting, [this]()
	{ emit broadcast("irc:server:" + id().toString(), "connecting"); });
	connect(m_connection, &IrcConnection::connected, [this]()
	{ emit broadcast("irc:server:" + id().toString(), "connected"); });
	connect(m_connection, &IrcConnection::disconnected, [this]()
	{ emit broadcast("irc:server:" + id().toString(), "disconnected"); });
	connect(m_connection, &IrcConnection::statusChanged, this, &IrcServer::statusChanged);

	m_bufferModel = new IrcBufferModel(m_connection);
	connect(m_bufferModel, &IrcBufferModel::added, this, &IrcServer::addedBuffer);
	connect(m_bufferModel, &IrcBufferModel::removed, this, &IrcServer::removedBuffer);
	connect(m_bufferModel, &IrcBufferModel::channelsChanged, m_parser, &IrcCommandParser::setChannels);

	IrcBuffer *serverBuffer = m_bufferModel->add(m_connection->host());
	connect(m_bufferModel, &IrcBufferModel::messageIgnored, serverBuffer, &IrcBuffer::receiveMessage);

	subscribeTo("chat:channel:" + id().toString());
}

QObject *IrcServer::connection() const
{
	return m_connection;
}
QString IrcServer::status() const
{
	switch (m_connection->status())
	{
	case IrcConnection::Inactive: return "inactive";
	case IrcConnection::Waiting: return "waiting";
	case IrcConnection::Connecting: return "connecting";
	case IrcConnection::Connected: return "connected";
	case IrcConnection::Closing: return "closing";
	case IrcConnection::Closed: return "closed";
	case IrcConnection::Error: return "error";
	}
}

void IrcServer::ready()
{
	emit newConnection(m_channelsList);
}

void IrcServer::disconnectFromHost(const QUuid &uuid)
{
	if (m_connection->isConnected())
	{
		qCDebug(IRC) << "Closing connection to" << m_connection->displayName();
		m_connection->close();
	}
	else if (!uuid.isNull())
	{
		emit broadcast("irc:server:" + id().toString(), "disconnect:error", {{"error", "Not connected"}}, uuid);
	}
}
void IrcServer::connectToHost(const QUuid &uuid)
{
	if (!m_connection->isConnected())
	{
		qCDebug(IRC) << "Connecting to" << m_connection->displayName() << "...";
		m_connection->open();
	}
	else if (!uuid.isNull())
	{
		emit broadcast("irc:server:" + id().toString(), "connect:error", {{"error", "Already connected"}}, uuid);
	}
}

void IrcServer::toClient(const QJsonObject &obj)
{
	using namespace Json;
	const QString channel = ensureString(obj, "channel");
	const QString cmd = ensureString(obj, "cmd");
	const QUuid msgId = ensureUuid(obj, "msgId");

	if (channel.startsWith("chat:channel:") && m_bufferIds.values().contains(QString(channel).remove("chat:channel:")))
	{
		const QString bufferId = QString(channel).remove("chat:channel:");
		IrcBuffer *buffer = m_bufferIds.key(bufferId);
		if (cmd == "send")
		{
			const QString message = ensureString(obj, "msg");
			m_parser->setTarget(buffer->title());
			IrcCommand *cmd = m_parser->parse(message);
			if (cmd)
			{
				m_connection->sendCommand(cmd);

				if (cmd->type() == IrcCommand::Join)
				{
					qDebug() << cmd->parameters().first();
					m_bufferModel->add(cmd->parameters().first());
				}

				if (cmd->type() == IrcCommand::Message || cmd->type() == IrcCommand::CtcpAction)
				{
					IrcMessage *msg = cmd->toMessage(m_connection->nickName(), m_connection);
					msg->setProperty("buffer", QVariant::fromValue<IrcBuffer *>(buffer));
					messageReceived(msg);
					delete msg;
				}
			}
			else
			{
				QString error;
				QString command = message.mid(1).split(" ", QString::SkipEmptyParts).value(0).toUpper();
				if (m_parser->commands().contains(command))
					error = tr("Syntax: %1").arg(m_parser->syntax(command));
				else
					error = tr("Unknown command: %1").arg(command);
				emit broadcast("chat:channel:" + bufferId, "message", {
								   {"content", error},
								   {"from", "ERROR"},
								   {"type", "notice"},
								   {"timestamp", QDateTime::currentMSecsSinceEpoch()}
							   });
			}
		}
	}
}

void IrcServer::addedBuffer(IrcBuffer *buffer)
{
	Q_ASSERT(!m_bufferIds.contains(buffer));

	const QString parent = buffer->title() == m_connection->host() ? "" : m_bufferIds[m_bufferModel->find(m_connection->host())];
	IrcWrappedChannel *channel = new IrcWrappedChannel(buffer, parent, buffer);
	qCDebug(IRC) << "Added buffer" << channel->id() << "with parent" << channel->parentId();

	connect(buffer, &IrcBuffer::messageReceived, this, &IrcServer::messageReceived);
	m_bufferIds.insert(buffer, channel->id().toString());

	m_syncedUserModels.insert(buffer, new SyncableUsersList(new IrcUserModel(buffer), m_bufferIds[buffer]));

	m_channelsList->add(channel);

	subscribeTo("chat:channel:" + m_bufferIds[buffer]);
}
void IrcServer::removedBuffer(IrcBuffer *buffer)
{
	m_channelsList->remove(m_channelsList->findIndex(m_bufferIds[buffer]));
	m_bufferIds.remove(buffer);
	delete m_userModels.take(buffer);
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
					   {"timestamp", QString::number(msg->timeStamp().toMSecsSinceEpoch())}
				   });
}

IrcClientConnection::IrcClientConnection(QObject *parent)
	: AbstractClientConnection(parent)
{
	m_servers = new SyncableQObjectList("irc:servers", QString(), "id", SyncableList::Flags(SyncableList::AllFlags) & ~SyncableList::AllowExternalRemove, this);
	m_servers->addPropertyMapping("connection", "connected", "connected");
	m_servers->addPropertyMapping("connection", "displayName", "name");
	m_servers->addPropertyMapping("connection", "host", "host");
	m_servers->addPropertyMapping("connection", "nickName", "nickName");
	m_servers->addPropertyMapping("connection", "nickNames", "nickNames");
	m_servers->addPropertyMapping("connection", "password", "password");
	m_servers->addPropertyMapping("connection", "port", "port");
	m_servers->addPropertyMapping("connection", "realName", "realName");
	m_servers->addPropertyMapping("connection", "secure", "useSecure");
	m_servers->addPropertyMapping("connection", "servers", "servers");
	m_servers->addPropertyMapping("connection", "userName", "userName");
	m_servers->addPropertyMapping("status", "status");
	m_servers->addPropertyMapping("id", "id");
	m_servers->addCommandMapping("connect", "connectToHost(const QUuid&)");
	m_servers->addCommandMapping("disconnect", "disconnectFromHost(const QUuid&)");
	subscribeTo("irc:servers");
}

void IrcClientConnection::ready()
{
	emit newConnection(m_servers);
	qCDebug(IRC) << "IRC enabled!";
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
			const QString displayName = ensureString(obj, "displayName");
			const QString host = ensureString(obj, "host");
			IrcServer *server = new IrcServer(displayName, host, this);
			emit newConnection(server);
			m_servers->add(server);
		}
		else if (cmd == "remove")
		{
			const QString id = ensureString(obj, "id");
			const int index = m_servers->findIndex(id);
			if (index != -1)
			{
				IrcServer *server = qobject_cast<IrcServer *>(m_servers->get(index));
				if (qobject_cast<IrcConnection *>(server->connection())->isConnected())
				{
					server->disconnectFromHost();
				}
				m_servers->remove(index, msgId);
			}
			else
			{
				emit broadcast("irc:servers", "remove:error", {{"error", "Unknown server"}}, msgId);
			}
		}
	}
}

#include "IrcClientConnection.moc"
