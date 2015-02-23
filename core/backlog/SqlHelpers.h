#pragma once

#include <QStringList>
#include <QMap>
#include <QVariant>
#include <QSqlQuery>
#include <QLoggingCategory>

namespace Sql
{
enum Type
{
	SMALLINT,
	INTEGER,
	BIGINT,
	DOUBLE,
	DATE,
	TIME,
	TIMESTAMP,
	DATETIME,
	TINYBLOB,
	BLOB,
	LONGBLOB,
	TEXT,
	LONGTEXT,
	UUID
};
QString type(const Type type);
QString VARCHAR(int size);
}

namespace SqlHelpers
{
class BaseQueryBuilder
{
public:
	virtual QSqlQuery prepare(const QSqlDatabase &db);
	QSqlQuery exec(const QSqlDatabase &db);
	QSqlQuery execAndNext(const QSqlDatabase &db);

	virtual QString stringify(const QString &dialect) const = 0;

protected:
	void checkError(const QSqlQuery &query, const QString &verb);
};
template<typename Super>
class ConditionalQueryBuilder : public BaseQueryBuilder
{
public:
	Super AND(const QString &left, const QString &op, const QVariant &right)
	{
		return WHERE(left, op, right);
	}
	Super WHERE(const QString &left, const QString &op, const QVariant right)
	{
		m_wheres.append(Where{left, op, right});
		return *static_cast<Super *>(this);
	}

	QSqlQuery prepare(const QSqlDatabase &db) override
	{
		QSqlQuery q = BaseQueryBuilder::prepare(db);
		for (int i = 0; i < m_valuesToBind.size(); ++i)
		{
			q.bindValue(":val_" + QString::number(i), m_valuesToBind.at(i));
		}
		return q;
	}

protected:
	QString stringifyWhere(const QString &dialect) const
	{
		QStringList wheres;
		for (const Where &where : m_wheres)
		{
			wheres.append(where.left + ' ' + where.op + " :val_" + QString::number(m_valuesToBind.size()));
			m_valuesToBind.append(where.right);
		}
		if (!wheres.isEmpty())
		{
			return "WHERE " + wheres.join(" AND ");
		}
		else
		{
			return "";
		}
	}

private:
	struct Where
	{
		QString left;
		QString op;
		QVariant right;
	};
	QList<Where> m_wheres;
	mutable QVariantList m_valuesToBind;
};

class SelectQueryBuilder : public ConditionalQueryBuilder<SelectQueryBuilder>
{
public:
	SelectQueryBuilder fields(const QStringList &fields)
	{
		m_fields = fields;
		return *this;
	}
	SelectQueryBuilder FROM(const QString &table)
	{
		m_table = table;
		return *this;
	}

private:
	QString stringify(const QString &dialect) const override;
	QStringList m_fields;
	QString m_table;
};
class InsertQueryBuilder : public BaseQueryBuilder
{
public:
	QSqlQuery prepare(const QSqlDatabase &db) override;

	InsertQueryBuilder INTO(const QString &table)
	{
		m_table = table;
		return *this;
	}
	template<typename... Cols>
	InsertQueryBuilder COLUMNS(Cols... cols)
	{
		m_columns = QStringList({cols...});
		return *this;
	}
	template<typename... T>
	InsertQueryBuilder VALUES(T... values)
	{
		m_values.append(QVariantList({values...}));
		return *this;
	}

private:
	QString stringify(const QString &dialect) const override;
	QString m_table;
	QStringList m_columns;
	QList<QVariantList> m_values;
	mutable QList<QVariantList> m_valuesToBind;
};
class UpdateQueryBuilder : public ConditionalQueryBuilder<UpdateQueryBuilder>
{
public:
	QSqlQuery prepare(const QSqlDatabase &db) override;

	UpdateQueryBuilder table(const QString &table)
	{
		m_table = table;
		return *this;
	}
	UpdateQueryBuilder SET(const QString &column, const QVariant &value)
	{
		m_values.insert(column, value);
		return *this;
	}

private:
	QString stringify(const QString &dialect) const override;

	QString m_table;
	QVariantMap m_values;
};

class CreateTableQueryBuilder : public BaseQueryBuilder
{
public:
	QSqlQuery prepare(const QSqlDatabase &db) override;

	CreateTableQueryBuilder temporary()
	{
		m_temporary = true;
		return *this;
	}
	CreateTableQueryBuilder IF_NOT_EXISTS()
	{
		m_ifNotExists = true;
		return *this;
	}
	CreateTableQueryBuilder named(const QString &name)
	{
		m_name = name;
		return *this;
	}
	CreateTableQueryBuilder COLUMN(const QString &name, const QString &type)
	{
		Column col;
		col.name = name;
		col.type = type;
		m_columns.append(col);
		return *this;
	}
	CreateTableQueryBuilder COLUMN(const QString &name, const Sql::Type type)
	{
		return COLUMN(name, Sql::type(type));
	}
	CreateTableQueryBuilder PRIMARY_KEY(const bool autoincrement = true)
	{
		Q_ASSERT(!m_columns.isEmpty());
		m_columns[m_columns.size() - 1].primaryKey = true;
		m_columns[m_columns.size() - 1].autoIncrement = autoincrement;
		return *this;
	}
	CreateTableQueryBuilder NOT_NULL()
	{
		Q_ASSERT(!m_columns.isEmpty());
		m_columns[m_columns.size() - 1].notNull = true;
		return *this;
	}
	CreateTableQueryBuilder defaultValue(const QVariant &def)
	{
		Q_ASSERT(!m_columns.isEmpty());
		m_columns[m_columns.size() - 1].default_ = def;
		return *this;
	}
	CreateTableQueryBuilder foreignReference(const QString &table, const QString &column)
	{
		Q_ASSERT(!m_columns.isEmpty());
		m_columns[m_columns.size() - 1].foreignReferences.insert(table, column);
		return *this;
	}

private:
	QString stringify(const QString &dialect) const override;
	bool m_temporary = false;
	bool m_ifNotExists = false;
	QString m_name;
	struct Column
	{
		QString name;
		QString type;
		bool primaryKey = false;
		bool autoIncrement = false;
		bool notNull = false;
		QVariant default_;
		QMap<QString, QString> foreignReferences;
	};
	QList<Column> m_columns;
};
}

namespace Sql
{
template<typename... Fields>
inline SqlHelpers::SelectQueryBuilder SELECT(Fields... f)
{
	return SqlHelpers::SelectQueryBuilder().fields(QStringList({f...}));
}
inline SqlHelpers::CreateTableQueryBuilder CREATE_TABLE(const QString &name)
{
	return SqlHelpers::CreateTableQueryBuilder().named(name);
}
inline SqlHelpers::CreateTableQueryBuilder CREATE_TEMPORARY_TABLE(const QString &name)
{
	return SqlHelpers::CreateTableQueryBuilder().temporary().named(name);
}
inline SqlHelpers::InsertQueryBuilder INSERT()
{
	return SqlHelpers::InsertQueryBuilder();
}
inline SqlHelpers::UpdateQueryBuilder UPDATE(const QString &table)
{
	return SqlHelpers::UpdateQueryBuilder().table(table);
}

}

Q_DECLARE_LOGGING_CATEGORY(SqlLog)
