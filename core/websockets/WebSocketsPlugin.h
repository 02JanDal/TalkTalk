#pragma once

#include "core/Plugin.h"

class WebSocketsPlugin : public Plugin
{
public:
	QList<QCommandLineOption> cliOptions() const override;
	bool handleArguments(const QCommandLineParser &parser) const override { return true; }
	QList<AbstractClientConnection *> clients(const QCommandLineParser &parser) const override;
};
