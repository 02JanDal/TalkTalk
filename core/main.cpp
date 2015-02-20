#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QJsonArray>

#include "TcpServer.h"
#include "ConnectionManager.h"
#include "AbstractClientConnection.h"

#ifdef TALKTALK_CORE_WEBSOCKETS
# include "WebSocketServer.h"
#endif

#ifdef TALKTALK_CORE_IRC
# include "IrcClientConnection.h"
#endif

class DummyClient : public AbstractClientConnection
{
	Q_OBJECT
public:
	explicit DummyClient()
		: AbstractClientConnection(nullptr)
	{
		setMonitor(true);
	}

public slots:
	void run()
	{
		emit broadcast("irc:servers", "add", {{"host", "irc.esper.net"},
											  {"realName", "TalkTalk"},
											  {"nickNames", QJsonArray::fromStringList(QStringList() << "TalkTalk")},
											  {"displayName", "TalkTalk"},
											  {"userName", "TalkTalk"}});
		emit broadcast("chat:channel:irc.esper.net", "send", {{"msg", "/join #jan_test_channel"}});
		emit broadcast("irc:server:irc.esper.net", "connect");
	}

private:
	void toClient(const QJsonObject &obj) override {}
};

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	app.setApplicationName("TalkTalkCore");
	app.setOrganizationName("Jan Dalheimer");

	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addOption(QCommandLineOption("tcp-listen", "The IP address to listen on for TCP connections, 0.0.0.0 for all", "IP", "0.0.0.0"));
	parser.addOption(QCommandLineOption("tcp-port", "The port to listen on for TCP connections", "PORT", "11101"));
	parser.addOption(QCommandLineOption("ws-listen", "The IP address to listen on for WebSocket connections, 0.0.0.0 for all", "IP", "0.0.0.0"));
	parser.addOption(QCommandLineOption("ws-port", "The port to listen on for WebSocket connections", "PORT", "11102"));
	parser.process(app);

	ConnectionManager mngr;

	TcpServer tcpServer;
	QObject::connect(&tcpServer, &TcpServer::newConnection, &mngr, &ConnectionManager::newConnection);
	if (!tcpServer.listen(QHostAddress(parser.value("tcp-listen")), parser.value("tcp-port").toULong()))
	{
		qWarning() << "Unable to start TCP server:" << tcpServer.errorString();
		return -1;
	}

#ifdef TALKTALK_CORE_WEBSOCKETS
	WebSocketServer wsServer;
	QObject::connect(&wsServer, &WebSocketServer::newConnection, &mngr, &ConnectionManager::newConnection);
	if (!wsServer.listen(QHostAddress(parser.value("ws-listen")), parser.value("ws-port").toULong()))
	{
		qWarning() << "Unable to start WebSocket server:" << wsServer.errorString();
		return -1;
	}
#endif

#ifdef TALKTALK_CORE_IRC
	IrcClientConnection ircClient;
	mngr.newConnection(&ircClient);
	QObject::connect(&ircClient, &IrcClientConnection::newIrcServer, &mngr, &ConnectionManager::newConnection);
#endif

	DummyClient client;
	mngr.newConnection(&client);
	QMetaObject::invokeMethod(&client, "run", Qt::QueuedConnection);

	return app.exec();
}

#include "main.moc"
