#include "WebSocketClientConnection.h"

#include <QWebSocket>

#include "common/Json.h"

WebSocketClientConnection::WebSocketClientConnection(QWebSocket *socket, QObject *parent)
	: AbstractClientConnection(parent), m_socket(socket)
{
	m_socket->setParent(this);

	connect(m_socket, &QWebSocket::binaryMessageReceived, this, &WebSocketClientConnection::binaryReceived);
	connect(m_socket, &QWebSocket::textMessageReceived, this, &WebSocketClientConnection::textReceived);
	connect(m_socket, &QWebSocket::disconnected, this, &WebSocketClientConnection::disconnected);
}

void WebSocketClientConnection::binaryReceived(const QByteArray &msg)
{
	QString channel;
	int messageId;
	try
	{
		const QJsonObject obj = Json::ensureObject(Json::ensureDocument(msg));
		channel = Json::ensureIsType<QString>(obj, "channel");
		messageId = Json::ensureIsType<int>(obj, "messageId");
		fromClient(obj);
	}
	catch (Exception &e)
	{
		toClient(QJsonObject({{"cmd", "error"}, {"error", e.message()}, {"channel", channel}, {"messageId", messageId}}));
	}
}
void WebSocketClientConnection::textReceived(const QString &msg)
{
	binaryReceived(msg.toUtf8());
}

void WebSocketClientConnection::disconnected()
{
	m_socket = nullptr;
	deleteLater();
}

void WebSocketClientConnection::toClient(const QJsonObject &obj)
{
	if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState)
	{
		m_socket->sendTextMessage(Json::toText(obj));
	}
}

