#pragma once

#include <QObject>
#include <QHash>
#include <QVector>

class QTcpSocket;
class QHostAddress;
class AbstractConsumer;

class ServerConnection : public QObject
{
	Q_OBJECT
public:
	explicit ServerConnection(const QString &host, const quint16 port, QObject *parent);

	void registerConsumer(AbstractConsumer *consumer);
	void unregisterConsumer(AbstractConsumer *consumer);

public slots:
	void connectToHost();

signals:
	void message(const QString &msg);
	void disconnected();
	void connected();

private:
	friend class AbstractConsumer;
	void subscribeConsumerTo(AbstractConsumer *consumer, const QString &channel);
	void unsubscribeConsumerFrom(AbstractConsumer *consumer, const QString &channel);
	void sendFromConsumer(const QString &channel, const QString &cmd, const QJsonObject &data, const QUuid &replyTo);

private slots:
	void socketChangedState();
	void socketError();
	void socketDataReady();

private:
	QString m_host;
	quint16 m_port;
	QTcpSocket *m_socket;
	QHash<QString, QVector<AbstractConsumer *>> m_subscriptions;
	QList<AbstractConsumer *> m_consumers;
};
