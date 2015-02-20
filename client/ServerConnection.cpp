#include "ServerConnection.h"

#include <QTcpSocket>

#include "common/Json.h"
#include "common/TcpUtils.h"
#include "AbstractConsumer.h"

ServerConnection::ServerConnection(const QString &host, const quint16 port, QObject *parent)
	: QObject(parent), m_host(host), m_port(port)
{
	m_socket = new QTcpSocket(this);
	connect(m_socket, &QTcpSocket::stateChanged, this, &ServerConnection::socketChangedState);
	connect(m_socket, static_cast<void(QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), this, &ServerConnection::socketError);
	connect(m_socket, &QTcpSocket::readyRead, this, &ServerConnection::socketDataReady);
}

void ServerConnection::registerConsumer(AbstractConsumer *consumer)
{
	m_consumers.append(consumer);
}
void ServerConnection::unregisterConsumer(AbstractConsumer *consumer)
{
	m_consumers.removeAll(consumer);
	for (const QString &channel : m_subscriptions.keys())
	{
		m_subscriptions[channel].removeAll(consumer);
	}
}

void ServerConnection::connectToHost()
{
	m_socket->connectToHost(m_host, m_port);
}

void ServerConnection::subscribeConsumerTo(AbstractConsumer *consumer, const QString &channel)
{
	m_subscriptions[channel].append(consumer);
}
void ServerConnection::unsubscribeConsumerFrom(AbstractConsumer *consumer, const QString &channel)
{
	m_subscriptions[channel].removeAll(consumer);
}
void ServerConnection::sendFromConsumer(const QString &channel, const QString &cmd, const QJsonObject &data, const QUuid &replyTo)
{
	if (m_socket->state() != QTcpSocket::ConnectedState)
	{
		return;
	}

	QJsonObject obj = data;
	obj["channel"] = channel;
	obj["cmd"] = cmd;
	obj["msgId"] = Json::toJson(QUuid::createUuid());
	if (!replyTo.isNull())
	{
		obj["replyTo"] = Json::toJson(replyTo);
	}

	const QByteArray raw = Json::toBinary(obj);
	QDataStream str(m_socket);
	str.setByteOrder(QDataStream::LittleEndian);
	str << (quint32) raw.size();
	m_socket->write(raw);
}

void ServerConnection::socketChangedState()
{
	switch (m_socket->state())
	{
	case QAbstractSocket::UnconnectedState:
		emit message(tr("Lost connection to host"));
		emit disconnected();
		break;
	case QAbstractSocket::HostLookupState:
		emit message(tr("Looking up host..."));
		break;
	case QAbstractSocket::ConnectingState:
		emit message(tr("Connecting to host..."));
		break;
	case QAbstractSocket::ConnectedState:
		emit message(tr("Connected!"));
		emit connected();
		break;
	case QAbstractSocket::BoundState:
		break;
	case QAbstractSocket::ListeningState:
		break;
	case QAbstractSocket::ClosingState:
		break;
	}
}
void ServerConnection::socketError()
{
	emit message(tr("Socket error: %1").arg(m_socket->errorString()));
}
void ServerConnection::socketDataReady()
{
	using namespace Json;
	QString channel;
	QUuid messageId;
	try
	{
		const QJsonObject obj = ensureObject(ensureDocument(TcpUtils::readPacket(m_socket)));
		channel = ensureIsType<QString>(obj, "channel");
		messageId = ensureIsType<QUuid>(obj, "msgId");
		const QString cmd = ensureIsType<QString>(obj, "cmd");

		for (AbstractConsumer *consumer : m_subscriptions[channel])
		{
			consumer->consume(channel, cmd, obj);
		}
	}
	catch (Exception &e)
	{
		sendFromConsumer(channel, "error", {{"error", e.message()}}, messageId);
	}
}
