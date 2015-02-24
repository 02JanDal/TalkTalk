#include "IrcClientConnection.h"

#include <QSignalBlocker>
#include <IrcConnection>
#include <IrcCommand>

#include "common/Json.h"
#include "common/FileSystem.h"
#include "core/SyncableList.h"
#include "IrcServer.h"

Q_LOGGING_CATEGORY(IRC, "core.irc")

IrcClientConnection::IrcClientConnection(QObject *parent)
	: AbstractClientConnection(parent), BaseConfigObject("irc.json", this)
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

	connect(m_servers, &SyncableQObjectList::changed, this, [this](){scheduleSave();});
}

void IrcClientConnection::ready()
{
	emit newConnection(m_servers);
	qCDebug(IRC) << "Loading existing servers/channels...";
	if (FS::exists("irc.json"))
	{
		using namespace Json;
		const QList<QJsonObject> servers = ensureIsArrayOf<QJsonObject>(ensureDocument(QStringLiteral("irc.json")).array());
		for (const QJsonObject &obj : servers)
		{
			qCDebug(IRC) << "Loading" << ensureString(obj, "name");
			QJsonObject tmp = obj;
			tmp.remove("channels");
			tmp.remove("connected");
			tmp.insert("channel", "irc:servers");
			tmp.insert("cmd", "add");
			tmp.insert("msgId", toJson(QUuid::createUuid()));
			toClient(tmp);

			IrcServer *server = m_servers->get<IrcServer>(m_servers->find("id", ensureUuid(obj, "id")));
			Q_ASSERT(server);
			for (const QJsonObject &obj : ensureIsArrayOf<QJsonObject>(obj, "channels"))
			{
				const QUuid uuid = ensureUuid(obj, "id");
				const QString channel = ensureString(obj, "name");
				const bool connected = ensureBoolean(obj, QStringLiteral("connected"));
				server->addChannel(channel, uuid, connected);
			}

			if (ensureBoolean(obj, QStringLiteral("connected")))
			{
				server->connectToHost();
			}
		}
	}

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
			const QString displayName = ensureString(obj, "name");
			const QString host = ensureString(obj, "host");
			IrcServer *server = new IrcServer(displayName, host, ensureUuid(obj, "id", QUuid::createUuid()), this);
			emit newConnection(server);
			m_servers->add(server);
			connect(server, &IrcServer::buffersChanged, this, [this]() {scheduleSave();});
			connect(server, &IrcServer::statusChanged, this, [this]() {scheduleSave();});
			scheduleSave();

			const int index = m_servers->find("id", server->id());
			const QSignalBlocker blocker(m_servers);
			for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)
			{
				const QString key = it.key();
				if (key == "channel" || key == "cmd" || key == "msgId" || key == "name" || key == "host" || key == "id")
				{
					continue;
				}
				if (m_servers->keys().contains(key))
				{
					m_servers->set(index, key, it.value().toVariant(), QUuid());
				}
			}
		}
		else if (cmd == "remove")
		{
			const QString id = ensureString(obj, "id");
			const int index = m_servers->findIndex(id);
			if (index != -1)
			{
				IrcServer *server = m_servers->get<IrcServer>(index);
				if (qobject_cast<IrcConnection *>(server->connection())->isConnected())
				{
					server->disconnectFromHost();
				}
				m_servers->remove(index, msgId);
				scheduleSave();
			}
			else
			{
				emit broadcast("irc:servers", "remove:error", {{"error", "Unknown server"}}, msgId);
			}
		}
	}
}


QByteArray IrcClientConnection::doSave() const
{
	QJsonArray array;
	for (int i = 0; i < m_servers->size(); ++i)
	{
		IrcServer *server = m_servers->get<IrcServer>(i);
		QJsonObject obj = QJsonObject::fromVariantMap(m_servers->getAll(i));
		obj.insert("connected", qobject_cast<IrcConnection *>(server->connection())->isConnected());

		QJsonArray channels;
		for (int c = 0; c < server->channelsList()->size(); ++c)
		{
			QJsonObject obj;
			obj.insert("id", Json::toJson(server->channelsList()->get(c, "id").toUuid()));
			obj.insert("connected", server->channelsList()->get(c, "active").toBool());
			obj.insert("name", server->channelsList()->get(c, "name").toString());
			channels.append(obj);
		}
		obj.insert("channels", channels);
		array.append(obj);
	}
	return Json::toText(array);
}
void IrcClientConnection::doLoad(const QByteArray &data)
{
}
