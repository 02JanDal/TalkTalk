#pragma once

#include <QTcpServer>

class AbstractClientConnection;

class TcpServer : public QTcpServer
{
	Q_OBJECT
public:
	explicit TcpServer(QObject *parent = nullptr);

signals:
	void newConnection(AbstractClientConnection *connection);

protected:
	void incomingConnection(qintptr handle) override;
};
