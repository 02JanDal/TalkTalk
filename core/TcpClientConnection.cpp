#include "TcpClientConnection.h"

#include <QTcpSocket>

#include "common/Json.h"
#include "common/TcpUtils.h"

TcpClientConnection::TcpClientConnection(qintptr handle)
	: AbstractClientConnection(nullptr), m_handle(handle)
{
}

void TcpClientConnection::setup()
{
	m_socket = new QTcpSocket(this);
	connect(m_socket, &QTcpSocket::readyRead, this, &TcpClientConnection::readyRead);
	connect(m_socket, &QTcpSocket::disconnected, this, &TcpClientConnection::disconnected);
	m_socket->setSocketDescriptor(m_handle);
}

void TcpClientConnection::toClient(const QJsonObject &obj)
{
	TcpUtils::writePacket(m_socket, Json::toBinary(obj));
}

void TcpClientConnection::readyRead()
{
	QString channel;
	int messageId;
	try
	{
		const QJsonObject obj = Json::ensureObject(Json::ensureDocument(TcpUtils::readPacket(m_socket)));
		channel = Json::ensureIsType<QString>(obj, "channel");
		messageId = Json::ensureIsType<int>(obj, "messageId");
		fromClient(obj);
	}
	catch (Exception &e)
	{
		toClient(QJsonObject({{"cmd", "error"}, {"error", e.message()}, {"channel", channel}, {"messageId", messageId}}));
	}
}

void TcpClientConnection::disconnected()
{
	m_socket = nullptr;
	deleteLater();
}
