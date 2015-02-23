#include "SqlHelpers.h"

#include <QSqlError>

Q_LOGGING_CATEGORY(SqlLog, "core.sql")

QString Sql::VARCHAR(int size)
{
	return QString("VARCHAR(%1)").arg(size);
}
QString Sql::type(const Sql::Type type)
{
	switch (type)
	{
	case Sql::SMALLINT:
		return "SMALLINT";
	case Sql::INTEGER:
		return "INTEGER";
	case Sql::BIGINT:
		return "BIGINT";
	case Sql::DOUBLE:
		return "DOUBLE";
	case Sql::DATE:
		return "DATE";
	case Sql::TIME:
		return "TIME";
	case Sql::TIMESTAMP:
		return "TIMESTAMP";
	case Sql::DATETIME:
		return "DATETIME";
	case Sql::TINYBLOB:
		return "TINYBLOB";
	case Sql::BLOB:
		return "BLOB";
	case Sql::LONGBLOB:
		return "LONGBLOB";
	case Sql::TEXT:
		return "TEXT";
	case Sql::LONGTEXT:
		return "LONGTEXT";
	case Sql::UUID:
		return "CHAR(39)";
	}
}


QSqlQuery SqlHelpers::BaseQueryBuilder::prepare(const QSqlDatabase &db)
{
	QSqlQuery q(db);
	q.prepare(stringify(db.driverName()));
	checkError(q, "prepare");
	return q;
}
QSqlQuery SqlHelpers::BaseQueryBuilder::exec(const QSqlDatabase &db)
{
	QSqlQuery q = prepare(db);
	if (q.lastError().isValid())
	{
		return q;
	}
	q.exec();
	checkError(q, "execute");
	return q;
}
QSqlQuery SqlHelpers::BaseQueryBuilder::execAndNext(const QSqlDatabase &db)
{
	QSqlQuery q = exec(db);
	if (!q.lastError().isValid())
	{
		q.next();
	}
	checkError(q, "seek in");
	return q;
}

void SqlHelpers::BaseQueryBuilder::checkError(const QSqlQuery &query, const QString &verb)
{
	if (query.lastError().isValid())
	{
		qCCritical(SqlLog) << ("Unable to " + verb + " query") << query.lastQuery() << ":" << query.lastError().text();
	}
}


QString SqlHelpers::SelectQueryBuilder::stringify(const QString &dialect) const
{
	QString out = "SELECT ";
	out += m_fields.join(',');
	out += " FROM ";
	out += m_table;
	out += ' ' + stringifyWhere(dialect);
	return out;
}


QSqlQuery SqlHelpers::CreateTableQueryBuilder::prepare(const QSqlDatabase &db)
{
	QSqlQuery q = BaseQueryBuilder::prepare(db);
	for (const Column &col : m_columns)
	{
		q.bindValue(':' + col.name, col.default_);
	}
	return q;
}
QString SqlHelpers::CreateTableQueryBuilder::stringify(const QString &dialect) const
{
	QString out = "CREATE ";
	if (m_temporary)
	{
		out += "TEMPORARY ";
	}
	out += "TABLE ";
	if (m_ifNotExists)
	{
		out += "IF NOT EXISTS ";
	}
	out += m_name;
	out += " (";
	for (const Column &col : m_columns)
	{
		out += col.name + ' ' + col.type + ' ';
		if (col.notNull)
		{
			out += "NOT NULL ";
		}
		if (col.default_.isValid())
		{
			out += "DEFAULT :" + col.name;
		}
		if (col.primaryKey)
		{
			out += "PRIMARY KEY ";
			if (col.autoIncrement)
			{
				if (dialect.contains("MYSQL"))
				{
					out += "AUTO_INCREMENT ";
				}
				else
				{
					out += "AUTOINCREMENT ";
				}
			}
		}
		for (auto it = col.foreignReferences.constBegin(); it != col.foreignReferences.constEnd(); ++it)
		{
			out += "REFERENCES " + it.key() + "(" + it.value() + ") ";
		}
		out += ',';
	}
	out.remove(out.size() - 1, 1);
	out += ")";
	return out;
}


QSqlQuery SqlHelpers::InsertQueryBuilder::prepare(const QSqlDatabase &db)
{
	QSqlQuery q = BaseQueryBuilder::prepare(db);
	for (int i = 0; i < m_valuesToBind.size(); ++i)
	{
		const QVariantList list = m_valuesToBind.at(i);
		for (int j = 0; j < list.size(); ++j)
		{
			if (list.at(j).isValid())
			{
				q.bindValue(QString(":val_%1_%2").arg(i).arg(j), list.at(j));
			}
		}
	}
	return q;
}
QString SqlHelpers::InsertQueryBuilder::stringify(const QString &dialect) const
{
	m_valuesToBind.clear();

	QString out;
	out += "INSERT INTO " + m_table + " ";
	if (!m_columns.isEmpty())
	{
		out += "(" + m_columns.join(',') + ") ";
	}
	out += "VALUES ";
	QStringList values;
	for (int i = 0; i < m_values.size(); ++i)
	{
		const QVariantList vals = m_values.at(i);

		QStringList val;
		QVariantList bindVals;
		for (int j = 0; j < vals.size(); ++j)
		{
			const QVariant value = vals.at(j);
			if (value.type() == QVariant::String && value.toString().startsWith(':'))
			{
				val.append(value.toString());
				bindVals.append(QVariant());
			}
			else
			{
				val.append(QString(":val_%1_%2").arg(i).arg(j));
				bindVals.append(value);
			}
		}
		m_valuesToBind.append(bindVals);
		values.append("(" + val.join(',') + ")");
	}
	out += values.join(", ");
	return out;
}


QSqlQuery SqlHelpers::UpdateQueryBuilder::prepare(const QSqlDatabase &db)
{
	QSqlQuery q = ConditionalQueryBuilder<UpdateQueryBuilder>::prepare(db);
	for (auto it = m_values.constBegin(); it != m_values.constEnd(); ++it)
	{
		q.bindValue(':' + it.key(), it.value());
	}
	return q;
}
QString SqlHelpers::UpdateQueryBuilder::stringify(const QString &dialect) const
{
	QString out = "UPDATE " + m_table + " SET ";
	QStringList sets;
	for (auto it = m_values.constBegin(); it != m_values.constEnd(); ++it)
	{
		sets.append(it.key() + "=:" + it.key());
	}
	out += sets.join(',');
	out += ' ' + stringifyWhere(dialect);
	return out;
}
