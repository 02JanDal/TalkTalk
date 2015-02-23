#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QJsonArray>
#include <QThread>
#include <QtPlugin>

#include "ConnectionManager.h"
#include "AbstractClientConnection.h"

#ifdef TALKTALK_CORE_TCP
# include "tcp/TcpPlugin.h"
#endif

#ifdef TALKTALK_CORE_WEBSOCKETS
# include "websockets/WebSocketsPlugin.h"
#endif

#ifdef TALKTALK_CORE_IRC
# include "irc/IrcPlugin.h"
#endif

#ifdef TALKTALK_CORE_BACKLOG
# include "backlog/BacklogPlugin.h"
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

	QList<Plugin *> plugins;
	plugins
		#ifdef TALKTALK_CORE_WEBSOCKETS
			<< new WebSocketsPlugin
		   #endif
		   #ifdef TALKTALK_CORE_TCP
			<< new TcpPlugin
		   #endif
		   #ifdef TALKTALK_CORE_IRC
			<< new IrcPlugin
		   #endif
		   #ifdef TALKTALK_CORE_BACKLOG
			<< new BacklogPlugin
		   #endif
			   ;

	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addVersionOption();
	for (const Plugin *plugin : plugins)
	{
		parser.addOptions(plugin->cliOptions());
	}
	parser.process(app);

	for (const Plugin *plugin : plugins)
	{
		if (!plugin->handleArguments(parser))
		{
			return 0;
		}
	}

	ConnectionManager *mngr = new ConnectionManager;

	for (const Plugin *plugin : plugins)
	{
		for (AbstractClientConnection *client : plugin->clients(parser))
		{
			setupMainClient(mngr, client);
		}
	}

	return app.exec();
}

#include "main.moc"
