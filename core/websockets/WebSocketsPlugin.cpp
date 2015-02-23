#include "WebSocketsPlugin.h"

#include "WebSocketServer.h"

QList<QCommandLineOption> WebSocketsPlugin::cliOptions() const
{
	return QList<QCommandLineOption>()
			<< QCommandLineOption("ws-listen", "The IP address to listen on for WebSocket connections, 0.0.0.0 for all", "IP", "0.0.0.0")
			<< QCommandLineOption("ws-port", "The port to listen on for WebSocket connections", "PORT", "11102");
}

QList<AbstractClientConnection *> WebSocketsPlugin::clients(const QCommandLineParser &parser) const
{
	return QList<AbstractClientConnection *>() << new WebSocketServer(QHostAddress(parser.value("ws-listen")), parser.value("ws-port").toULong());
}
