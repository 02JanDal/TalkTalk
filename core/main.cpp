#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QJsonArray>
#include <QThread>

#include "tcp/TcpServer.h"
#include "ConnectionManager.h"
#include "AbstractClientConnection.h"

#ifdef TALKTALK_CORE_WEBSOCKETS
# include "websockets/WebSocketServer.h"
#endif

#ifdef TALKTALK_CORE_IRC
# include "irc/IrcClientConnection.h"
#endif

#ifdef Q_OS_UNIX
#include <signal.h>

static void handleSignal(int sig, siginfo_t *si, void *unused)
{
	if (sig == SIGSEGV)
	{
		qCritical() << "Caught segfault at" << si->si_addr;
	}
	abort();
}
#endif

static void setupMainClient(ConnectionManager *mngr, AbstractClientConnection *client)
{
	//mngr->newConnection(client);

	QThread *thread = new QThread;
	client->moveToThread(thread);
	QObject::connect(thread, &QThread::started, client, [mngr, client](){mngr->newConnection(client);});
	QObject::connect(thread, &QThread::finished, client, &AbstractClientConnection::deleteLater);
	QObject::connect(client, &AbstractClientConnection::destroyed, thread, &QThread::deleteLater);
	thread->start();
}

int main(int argc, char **argv)
{
	qSetMessagePattern("[%{time hh:mm:ss.zzz}][%{category}][%{if-debug}DEBUG%{endif}%{if-warning}WARNING%{endif}%{if-critical}CRITICAL%{endif}%{if-fatal}FATAL%{endif}] %{message}%{if-critical}\n\t%{backtrace depth=10 separator=\"\n\t\"}%{endif}");

#ifdef Q_OS_UNIX
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = handleSignal;
	sigaction(SIGSEGV, &sa, NULL);
#endif

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

	ConnectionManager *mngr = new ConnectionManager;

	setupMainClient(mngr, new TcpServer(QHostAddress(parser.value("tcp-listen")), parser.value("tcp-port").toULong()));

#ifdef TALKTALK_CORE_WEBSOCKETS
	setupMainClient(mngr, new WebSocketServer(QHostAddress(parser.value("ws-listen")), parser.value("ws-port").toULong()));
#endif

#ifdef TALKTALK_CORE_IRC
	setupMainClient(mngr, new IrcClientConnection);
#endif

	return app.exec();
}

#include "main.moc"
