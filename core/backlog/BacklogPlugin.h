#pragma once

#include "core/Plugin.h"

class BacklogPlugin : public Plugin
{
public:
	QList<QCommandLineOption> cliOptions() const override;
	bool handleArguments(const QCommandLineParser &parser) const override;
	QList<AbstractClientConnection *> clients(const QCommandLineParser &parser) const override;
};
