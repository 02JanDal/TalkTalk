#pragma once

#include <QWebSocketServer>

class AbstractClientConnection;

class WebSocketServer : public QWebSocketServer
{
	Q_OBJECT
public:
	explicit WebSocketServer(QObject *parent = nullptr);

signals:
	void newConnection(AbstractClientConnection *connection);

private slots:
	void newConnectionReceived();
};
