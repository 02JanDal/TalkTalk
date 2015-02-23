#include "IrcPlugin.h"

#include "IrcClientConnection.h"

QList<QCommandLineOption> IrcPlugin::cliOptions() const
{
	return {};
}

QList<AbstractClientConnection *> IrcPlugin::clients(const QCommandLineParser &parser) const
{
	return QList<AbstractClientConnection *>() << new IrcClientConnection;
}
