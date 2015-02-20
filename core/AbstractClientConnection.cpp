#include "AbstractClientConnection.h"

#include <QDebug>

#include "common/Json.h"

AbstractClientConnection::AbstractClientConnection(QObject *parent)
	: QObject(parent)
{
}

void AbstractClientConnection::receive(const QString &channel, const QString &cmd, const QJsonObject &data, const QUuid &replyTo)
{
	if (!m_channels.contains(channel) && !m_monitor)
	{
		return;
	}
	if (replyTo.isNull())
	{
		qDebug() << this << "Got" << cmd << "on" << channel << ":" << data;
	}
	else
	{
		qDebug() << this << "Got" << cmd << "on" << channel << ":" << data << "as a reply to" << replyTo.toString();
	}
	QJsonObject obj = data;
	obj["channel"] = channel;
	obj["cmd"] = cmd;
	obj["msgId"] = Json::toJson(QUuid::createUuid());
	if (!replyTo.isNull())
	{
		obj["replyTo"] = Json::toJson(replyTo);
	}
	toClient(obj);
}

void AbstractClientConnection::fromClient(const QJsonObject &obj)
{
	using namespace Json;

	const QString channel = ensureIsType<QString>(obj, "channel", "");
	const QString cmd = ensureIsType<QString>(obj, "cmd");

	if (cmd == "ping")
	{
		toClient({{"cmd", "pong"}, {"channel", ""}, {"timestamp", ensureIsType<int>(obj, "timestamp")}});
	}
	else if (cmd == "subscribe")
	{
		subscribeTo(channel);
	}
	else if (cmd == "unsubscribe")
	{
		unsubscribeFrom(channel);
	}
	else if (cmd == "monitor")
	{
		setMonitor(ensureIsType<bool>(obj, QStringLiteral("value")));
	}
	else
	{
		emit broadcast(channel, cmd, ensureObject(obj, "data"));
	}
}

void AbstractClientConnection::subscribeTo(const QString &channel)
{
	m_channels.append(channel);
}
void AbstractClientConnection::unsubscribeFrom(const QString &channel)
{
	m_channels.removeAll(channel);
}
void AbstractClientConnection::setMonitor(const bool monitor)
{
	m_monitor = monitor;
}
