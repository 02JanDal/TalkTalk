#pragma once

#include "core/Plugin.h"

class TcpPlugin : public Plugin
{
public:
	QList<QCommandLineOption> cliOptions() const override;
	bool handleArguments(const QCommandLineParser &parser) const override { return true; }
	QList<AbstractClientConnection *> clients(const QCommandLineParser &parser) const override;
};
