#include "WebSocketServer.h"

#include "WebSocketClientConnection.h"

WebSocketServer::WebSocketServer(QObject *parent)
	: QWebSocketServer("", NonSecureMode, parent)
{
	connect(this, &QWebSocketServer::newConnection, this, &WebSocketServer::newConnectionReceived);
}

void WebSocketServer::newConnectionReceived()
{
	while (hasPendingConnections())
	{
		emit newConnection(new WebSocketClientConnection(nextPendingConnection()));
	}
}
