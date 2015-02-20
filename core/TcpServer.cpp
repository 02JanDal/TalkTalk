#include "TcpServer.h"

#include <QTcpSocket>
#include <QThread>

#include "TcpClientConnection.h"

TcpServer::TcpServer(QObject *parent)
	: QTcpServer(parent)
{
}

void TcpServer::incomingConnection(qintptr handle)
{
	TcpClientConnection *connection = new TcpClientConnection(handle);
	QThread *thread = new QThread;
	connection->moveToThread(thread);
	connect(connection, &TcpClientConnection::destroyed, thread, [thread](){thread->exit();});
	QMetaObject::invokeMethod(connection, "setup", Qt::QueuedConnection);

	emit newConnection(connection);
}
