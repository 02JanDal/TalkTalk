#pragma once

#include <QCommandLineParser>

class AbstractClientConnection;

class Plugin
{
public:
	virtual ~Plugin() {}

	virtual QList<QCommandLineOption> cliOptions() const = 0;
	virtual bool handleArguments(const QCommandLineParser &parser) const = 0;
	virtual QList<AbstractClientConnection *> clients(const QCommandLineParser &parser) const = 0;
};
