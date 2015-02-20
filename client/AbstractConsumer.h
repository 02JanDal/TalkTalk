#pragma once

#include <QJsonObject>
#include <QUuid>
#include <QStringList>

class ServerConnection;

class AbstractConsumer
{
public:
	virtual ~AbstractConsumer();

	virtual void consume(const QString &channel, const QString &cmd, const QJsonObject &data) = 0;

	void setServerConnection(ServerConnection *serverConnection);
	QStringList channels() const { return m_channels; }

protected:
	void subscribeTo(const QString &channel);
	void unsubscribeFrom(const QString &channel);
	void emitMsg(const QString &channel, const QString &msg, const QJsonObject &data = QJsonObject(), const QUuid &replyTo = QUuid());

private:
	ServerConnection *m_serverConnection;
	QStringList m_channels;
};
