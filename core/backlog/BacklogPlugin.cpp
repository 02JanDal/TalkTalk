#include "BacklogPlugin.h"

#include <QSqlDatabase>
#include <QPluginLoader>
#include <QLibraryInfo>
#include <QSqlDriverPlugin>
#include <QDir>
#include <QJsonArray>
#include <QRegularExpression>

#include "BacklogClientConnection.h"

static const char *libraryEnding =
		#if defined(Q_OS_OSX)
		".dylib"
		#elif defined(Q_OS_UNIX)
		".so"
		#elif defined(Q_OS_WIN)
		".dll"
		#endif
		;
static const QString sqlPluginPath = QLibraryInfo::location(QLibraryInfo::PluginsPath) + "/sqldrivers";

static QMap<QString, QPair<QString, QStaticPlugin>> getDrivers()
{
	static QMap<QString, QPair<QString, QStaticPlugin>> out;
	if (out.isEmpty())
	{
		for (const QFileInfo &info : QDir(sqlPluginPath).entryInfoList(QDir::Files))
		{
			if (QLibrary::isLibrary(info.absoluteFilePath()))
			{
				QPluginLoader loader(info.absoluteFilePath());
				if (loader.load())
				{
					for (const QJsonValue &item : loader.metaData().value("MetaData").toObject().value("Keys").toArray())
					{
						out.insert(item.toString(), qMakePair(info.absoluteFilePath(), QStaticPlugin()));
					}
				}
			}
		}
		for (const QStaticPlugin &plugin : QPluginLoader::staticPlugins())
		{
			if (plugin.metaData().value("IID").toString() == QSqlDriverFactoryInterface_iid)
			{
				for (const QJsonValue &item : plugin.metaData().value("MetaData").toObject().value("Keys").toArray())
				{
					out.insert(item.toString(), qMakePair(QString(), plugin));
				}
			}
		}
	}
	return out;
}

QList<QCommandLineOption> BacklogPlugin::cliOptions() const
{
	return QList<QCommandLineOption>()
			<< QCommandLineOption("backlog-db-driver", "Specify the SQL driver to use. Possible values: " + QStringList(getDrivers().keys()).join(", "), "DRIVER", "QSQLITE")
			<< QCommandLineOption("backlog-db-host", "Database host to use", "HOST", "localhost")
			<< QCommandLineOption("backlog-db-port", "Database port to use", "PORT", "")
			<< QCommandLineOption("backlog-db-dbname", "Database name to use", "NAME", "talktalk_backlog")
			<< QCommandLineOption("backlog-db-username", "Username to use for connecting to the database", "USERNAME", "talktalk")
			<< QCommandLineOption("backlog-db-password", "Password to use for connecting to the database", "PASSWORD", "talktalk")
			<< QCommandLineOption("backlog-db-options", "Options to use for connecting. See QSqlDatabase::setConnectionOptions for possible values", "OPTIONS", "")
			<< QCommandLineOption("backlog-list-drivers", "List available drivers and exit");
}

bool BacklogPlugin::handleArguments(const QCommandLineParser &parser) const
{
	if (parser.isSet("backlog-list-drivers"))
	{
		qCDebug(Backlog) << "Available drivers on this system:" << QStringList(getDrivers().keys()).join(", ").toUtf8().constData();
		return false;
	}
	if (!getDrivers().contains(parser.value("backlog-db-driver")))
	{
		qCWarning(Backlog) << "Driver" << parser.value("backlog-db-driver") << "is not available";
		return false;
	}
	return true;
}

QList<AbstractClientConnection *> BacklogPlugin::clients(const QCommandLineParser &parser) const
{
	const QString driver = parser.value("backlog-db-driver");
	const QPair<QString, QStaticPlugin> driverPath = getDrivers().value(driver);
	QSqlDriverPlugin *plugin;
	if (driverPath.first.isNull())
	{
		plugin = qobject_cast<QSqlDriverPlugin *>(driverPath.second.instance());
	}
	else
	{
		QPluginLoader loader(driverPath.first);
		if (!loader.load())
		{
			qCWarning(Backlog) << "Unable to load" << driverPath.first << ":" << loader.errorString();
			return {};
		}
		plugin = qobject_cast<QSqlDriverPlugin *>(loader.instance());
	}
	if (!plugin)
	{
		qCWarning(Backlog) << "There was an unknown error while loading" << driver;
		return {};
	}
	QSqlDriver *d = plugin->create(driver.toUpper());
	if (!d)
	{
		qCWarning(Backlog) << "Unable to create the SQL driver for" << driver;
		return {};
	}
	const auto options = BacklogClientConnection::Options{
			d,
			parser.value("backlog-db-host"),
			parser.value("backlog-db-port").toInt(),
			parser.value("backlog-db-dbname"),
			parser.value("backlog-db-username"),
			parser.value("backlog-db-password"),
			parser.value("backlog-db-options")
	};
	return QList<AbstractClientConnection *>() << new BacklogClientConnection(options);
}
