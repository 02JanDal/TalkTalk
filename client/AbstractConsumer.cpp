#include "AbstractConsumer.h"

#include "ServerConnection.h"

AbstractConsumer::~AbstractConsumer()
{
	if (m_serverConnection)
	{
		m_serverConnection->unregisterConsumer(this);
	}
}

void AbstractConsumer::setServerConnection(ServerConnection *serverConnection)
{
	m_serverConnection = serverConnection;
}

void AbstractConsumer::subscribeTo(const QString &channel)
{
	if (m_serverConnection)
	{
		m_serverConnection->subscribeConsumerTo(this, channel);
	}
	m_channels.append(channel);
}
void AbstractConsumer::unsubscribeFrom(const QString &channel)
{
	if (m_serverConnection)
	{
		m_serverConnection->unsubscribeConsumerFrom(this, channel);
	}
	m_channels.removeAll(channel);
}
void AbstractConsumer::emitMsg(const QString &channel, const QString &msg, const QJsonObject &data, const QUuid &replyTo)
{
	if (m_serverConnection)
	{
		m_serverConnection->sendFromConsumer(channel, msg, data, replyTo);
	}
}
