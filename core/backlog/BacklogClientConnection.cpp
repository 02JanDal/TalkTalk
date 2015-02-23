#include "BacklogClientConnection.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QThread>

#include "common/Json.h"
#include "SqlHelpers.h"

Q_LOGGING_CATEGORY(Backlog, "core.backlog")

static constexpr const int LATEST_SCHEMA_VERSION = 1;

BacklogClientConnection::BacklogClientConnection(const Options &options, QObject *parent)
	: AbstractClientConnection(parent)
{
	subscribeTo("chat:channels");

	QSqlDatabase db = QSqlDatabase::addDatabase(options.driver, "backlog");
	db.setHostName(options.host);
	db.setPort(options.port);
	db.setDatabaseName(options.dbName);
	db.setUserName(options.userName);
	db.setPassword(options.password);
	db.setConnectOptions(options.connectionOptions);
}

void BacklogClientConnection::ready()
{
	QSqlDatabase db = getDB();
	if (!db.open())
	{
		qCWarning(Backlog) << "Unable to connect to database:" << db.lastError().text();
		thread()->exit(1);
	}
	else
	{
		qCDebug(Backlog) << "Successfully connected to" << (db.hostName() + ':' + QString::number(db.port())) << "(DB" << db.databaseName() << ")";
	}

	createTables();
	migrateDatabase();

	QSqlQuery q = Sql::SELECT("id", "uuid").FROM("chat_channels").exec(db);
	while (q.next())
	{
		m_channelMapping.insert(q.value(1).toString(), q.value(0).toInt());
	}

	emit broadcast("chat:channels", "list");
}

void BacklogClientConnection::toClient(const QJsonObject &obj)
{
	using namespace Json;
	const QString channel = ensureString(obj, "channel");
	const QString cmd = ensureString(obj, "cmd");
	const QString msgId = ensureString(obj, "msgId");

	QSqlDatabase db = getDB();

	// ensure that we have the channel in the database, potentially setting/updating the name
	auto check = [db, this](const QJsonObject &data)
	{
		const QString id = ensureString(data, "id");
		QSqlQuery query = Sql::SELECT("id").FROM("chat_channels").WHERE("uuid", "=", id).exec(db);
		if (!query.isValid())
		{
			const QString id = ensureString(data, "id");
			Sql::INSERT().INTO("chat_channels").COLUMNS("uuid").VALUES(id).exec(db);
			QSqlQuery q = Sql::SELECT("id").FROM("chat_channels").WHERE("uuid", "=", id).execAndNext(db);
			m_channelMapping.insert(id, q.value(0).toInt());
			subscribeTo("chat:channel:" + id);

			if (!data.contains("name"))
			{
				emit broadcast("chat:channels", "get", {{"id", id}});
			}
		}
		if (data.contains("name"))
		{
			Sql::UPDATE("chat_channels").SET("name", ensureString(data, "name")).WHERE("uuid", "=", id);
		}
	};

	if (channel == "chat:channels")
	{
		if (cmd == "items")
		{
			for (const QJsonObject &item : ensureIsArrayOf<QJsonObject>(obj, "items"))
			{
				check(item);
			}
		}
		else if (cmd == "added" || cmd == "removed" || cmd == "changed" || cmd == "item")
		{
			const QString id = ensureString(obj, "id");
			if (cmd == "added")
			{
				check(obj);
			}
			else if (cmd == "removed")
			{
				// do nothing
				unsubscribeFrom("chat:channel:" + id);
			}
			else if (cmd == "changed")
			{
				if (obj.contains("name"))
				{
					check(obj);
				}
			}
			else if (cmd == "item")
			{
				check(obj);
			}
		}
	}
	else if (channel.startsWith("chat:channel:"))
	{
		const QString id = QString(channel).remove("chat:channel:");
		if (cmd == "message")
		{
			check({{"id", id}});
			Sql::INSERT().INTO("chat_messages").COLUMNS("channel", "source", "type", "content", "timestamp")
					.VALUES(m_channelMapping[id],
							ensureString(obj, "from"),
							ensureString(obj, "type"),
							ensureString(obj, "content"),
							ensureString(obj, "timestamp").toULongLong())
					.exec(db);
		}
		else if (cmd == "more")
		{
			const unsigned long long max = ensureString(obj, "max", "0").toULongLong();
			const unsigned long long min = ensureString(obj, "min", "0").toULongLong();
			const int amount = ensureInteger(obj, "amount", 20);
			qCDebug(Backlog) << "Got a request for" << amount << "lines of backlog from" << min << "to" << max;
		}
	}
}

void BacklogClientConnection::createTables()
{
	QSqlDatabase db = getDB();
	if (!db.tables().contains("settings"))
	{
		qCDebug(Backlog) << "Creating database...";
		Sql::CREATE_TABLE("settings")
				.COLUMN("category", Sql::VARCHAR(64)).NOT_NULL()
				.COLUMN("key", Sql::VARCHAR(64)).NOT_NULL()
				.COLUMN("value", Sql::VARCHAR(256))
				.exec(db);
		Sql::CREATE_TABLE("chat_channels")
				.COLUMN("id", Sql::INTEGER).PRIMARY_KEY().NOT_NULL()
				.COLUMN("name", Sql::VARCHAR(128))
				.COLUMN("uuid", Sql::UUID).NOT_NULL()
				.exec(db);
		Sql::CREATE_TABLE("chat_messages")
				.COLUMN("id", Sql::INTEGER).PRIMARY_KEY().NOT_NULL()
				.COLUMN("channel", Sql::INTEGER).NOT_NULL().foreignReference("chat_channels", "id")
				.COLUMN("source", Sql::VARCHAR(128)).NOT_NULL()
				.COLUMN("type", Sql::VARCHAR(32)).NOT_NULL()
				.COLUMN("content", Sql::VARCHAR(512)).NOT_NULL()
				.COLUMN("timestamp", Sql::TIMESTAMP).NOT_NULL()
				.exec(db);
		Sql::INSERT().INTO("settings").COLUMNS("category", "key", "value").VALUES("backlog", "schema_version", LATEST_SCHEMA_VERSION).exec(db);
		qCDebug(Backlog) << "Done";
	}
}
void BacklogClientConnection::migrateDatabase()
{
	QSqlDatabase db = getDB();
	QSqlQuery q = Sql::SELECT("value").FROM("settings").WHERE("category", "=", "backlog").AND("key", "=", "schema_version").execAndNext(db);
	int current = q.value(0).toInt();
	while (current != LATEST_SCHEMA_VERSION)
	{
		const int from = current;
		const int to = current + 1;
		qCDebug(Backlog) << "Migrating database from" << from << "to" << to;
		// do migrations here
	}
}

QSqlDatabase BacklogClientConnection::getDB() const
{
	return QSqlDatabase::database(QStringLiteral("backlog"), false);
}
