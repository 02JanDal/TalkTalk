#include "ConnectionManager.h"

#include <QDebug>

#include "AbstractClientConnection.h"

ConnectionManager::ConnectionManager(QObject *parent)
	: QObject(parent)
{
}

void ConnectionManager::newConnection(AbstractClientConnection *connection)
{
	connect(connection, &QObject::destroyed, this, &ConnectionManager::connectionDestroyed);
	for (auto other : m_connections)
	{
		connect(connection, &AbstractClientConnection::broadcast, other, &AbstractClientConnection::receive);
		connect(other, &AbstractClientConnection::broadcast, connection, &AbstractClientConnection::receive);
	}
	m_connections.append(connection);
}

void ConnectionManager::connectionDestroyed(QObject *connection)
{
	m_connections.removeAll(qobject_cast<AbstractClientConnection *>(connection));
}
