#pragma once

#include "core/AbstractClientConnection.h"

class QSqlDatabase;
class QSqlDriver;

class BacklogClientConnection : public AbstractClientConnection
{
	Q_OBJECT
public:
	struct Options
	{
		QSqlDriver *driver;
		QString host;
		int port;
		QString dbName;
		QString userName;
		QString password;
		QString connectionOptions;
	};

	explicit BacklogClientConnection(const Options &options, QObject *parent = nullptr);

	void ready() override;

private:
	void toClient(const QJsonObject &obj) override;

	QMap<QString, int> m_channelMapping;

	void createTables();
	void migrateDatabase();
	QSqlDatabase getDB() const;
};

Q_DECLARE_LOGGING_CATEGORY(Backlog)
