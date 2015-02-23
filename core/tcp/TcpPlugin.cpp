#include "TcpPlugin.h"

#include "TcpServer.h"

QList<QCommandLineOption> TcpPlugin::cliOptions() const
{
	return QList<QCommandLineOption>()
			<< QCommandLineOption("tcp-listen", "The IP address to listen on for TCP connections, 0.0.0.0 for all", "IP", "0.0.0.0")
			<< QCommandLineOption("tcp-port", "The port to listen on for TCP connections", "PORT", "11101");
}

QList<AbstractClientConnection *> TcpPlugin::clients(const QCommandLineParser &parser) const
{
	return QList<AbstractClientConnection *>() << new TcpServer(QHostAddress(parser.value("tcp-listen")), parser.value("tcp-port").toULong());
}
