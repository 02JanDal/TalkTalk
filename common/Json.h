// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QUrl>
#include <QDir>
#include <QUuid>

#include "Exception.h"

namespace Json
{
DECLARE_EXCEPTION(Json);

enum Requirement
{
	Required
};

void write(const QJsonDocument &doc, const QString &filename);
void write(const QJsonObject &object, const QString &filename);
void write(const QJsonArray &array, const QString &filename);
QByteArray toBinary(const QJsonObject &obj);
QByteArray toBinary(const QJsonArray &array);
QByteArray toText(const QJsonObject &obj);
QByteArray toText(const QJsonArray &array);

QJsonDocument ensureDocument(const QByteArray &data);
QJsonDocument ensureDocument(const QString &filename);
QJsonObject ensureObject(const QJsonDocument &doc, const QString &what = "Document");
QJsonArray ensureArray(const QJsonDocument &doc, const QString &what = "Document");

template<typename T>
QJsonValue toJson(const T &t)
{
	return QJsonValue(t);
}
template<>
QJsonValue toJson<QUrl>(const QUrl &url);
template<>
QJsonValue toJson<QByteArray>(const QByteArray &data);
template<>
QJsonValue toJson<QDateTime>(const QDateTime &datetime);
template<>
QJsonValue toJson<QDir>(const QDir &dir);
template<>
QJsonValue toJson<QUuid>(const QUuid &uuid);
template<>
QJsonValue toJson<QVariant>(const QVariant &variant);

template<typename T>
QJsonArray toJsonArray(const QList<T> &container)
{
	QJsonArray array;
	for (const T item : container)
	{
		array.append(toJson<T>(item));
	}
	return array;
}

// template magic!

// all of the following templated functions that start with std::enabled_if... are the backbone
// of the conversions. they are called by the higher-level functions (see further down) and
// handle type-specific conversions. if you want to add support for another type, take a look at
// the following functions
template <typename T>
typename std::enable_if<std::is_same<T, QString>::value, T>::type
ensureIsType(const QJsonValue &value, const Requirement = Required,
			 const QString &what = "Value")
{
	if (!value.isString())
	{
		throw JsonException(what + " is not a string");
	}
	return value.toString();
}

template <typename T>
typename std::enable_if<std::is_same<T, bool>::value, T>::type
ensureIsType(const QJsonValue &value, const Requirement = Required,
			 const QString &what = "Value")
{
	if (!value.isBool())
	{
		throw JsonException(what + " is not a bool");
	}
	return value.toBool();
}

template <typename T>
typename std::enable_if<std::is_same<T, double>::value, T>::type
ensureIsType(const QJsonValue &value, const Requirement = Required,
			 const QString &what = "Value")
{
	if (!value.isDouble())
	{
		throw JsonException(what + " is not a double");
	}
	return value.toDouble();
}

template <typename T>
typename std::enable_if<std::is_same<T, int>::value, T>::type
ensureIsType(const QJsonValue &value, const Requirement requirement = Required,
			 const QString &what = "Value")
{
	const double doubl = ensureIsType<double>(value, requirement, what);
	if (fmod(doubl, 1) != 0)
	{
		throw JsonException(what + " is not an integer");
	}
	return int(doubl);
}

template <typename T>
typename std::enable_if<std::is_same<T, QDateTime>::value, T>::type
ensureIsType(const QJsonValue &value, const Requirement requirement = Required,
			 const QString &what = "Value")
{
	const QString string = ensureIsType<QString>(value, requirement, what);
	const QDateTime datetime = QDateTime::fromString(string, Qt::ISODate);
	if (!datetime.isValid())
	{
		throw JsonException(what + " is not a ISO formatted date/time value");
	}
	return datetime;
}

template <typename T>
typename std::enable_if<std::is_same<T, QUrl>::value, T>::type
ensureIsType(const QJsonValue &value, const Requirement requirement = Required,
			 const QString &what = "Value")
{
	const QString string = ensureIsType<QString>(value, requirement, what);
	if (string.isEmpty())
	{
		return QUrl();
	}
	const QUrl url = QUrl(string, QUrl::StrictMode);
	if (!url.isValid())
	{
		throw JsonException(what + " is not a correctly formatted URL");
	}
	return url;
}

template <typename T>
typename std::enable_if<std::is_same<T, QByteArray>::value, T>::type
ensureIsType(const QJsonValue &value, const Requirement requirement = Required,
			 const QString &what = "Value")
{
	const QString string = ensureIsType<QString>(value, requirement, what);
	// ensure that the string can be safely cast to Latin1
	if (string != QString::fromLatin1(string.toLatin1()))
	{
		throw JsonException(what + " is not encodable as Latin1");
	}
	return QByteArray::fromHex(string.toLatin1());
}

template <typename T>
typename std::enable_if<std::is_same<T, QDir>::value, T>::type
ensureIsType(const QJsonValue &value, const Requirement requirement = Required, const QString &what = "Value")
{
	const QString string = ensureIsType<QString>(value, requirement, what);
	return QDir::current().absoluteFilePath(string);
}

template <typename T>
typename std::enable_if<std::is_same<T, QUuid>::value, T>::type
ensureIsType(const QJsonValue &value, const Requirement requirement = Required, const QString &what = "Value")
{
	const QString string = ensureIsType<QString>(value, requirement, what);
	const QUuid uuid = QUuid(string);
	if (uuid.toString() != string) // converts back => valid
	{
		throw JsonException(what + " is not a valid UUID");
	}
	return uuid;
}

template <typename T>
typename std::enable_if<std::is_same<T, QJsonObject>::value, T>::type
ensureIsType(const QJsonValue &value, const Requirement requirement = Required, const QString &what = "Value")
{
	if (!value.isObject())
	{
		throw JsonException(what + " is not an object");
	}
	return value.toObject();
}

template <typename T>
typename std::enable_if<std::is_same<T, QJsonArray>::value, T>::type
ensureIsType(const QJsonValue &value, const Requirement requirement = Required, const QString &what = "Value")
{
	if (!value.isArray())
	{
		throw JsonException(what + " is not an array");
	}
	return value.toArray();
}

template <typename T>
typename std::enable_if<std::is_same<T, QVariant>::value, T>::type
ensureIsType(const QJsonValue &value, const Requirement requirement = Required, const QString &what = "Value")
{
	if (value.isNull() || value.isUndefined())
	{
		throw JsonException(what + " is null or undefined");
	}
	return value.toVariant();
}


// the following functions are higher level functions, that make use of the above functions for
// type conversion
template <typename T>
T ensureIsType(const QJsonValue &value, const T default_, const QString &what = "Value")
{
	if (value.isUndefined())
	{
		return default_;
	}
	return ensureIsType<T>(value, Required, what);
}
template <typename T>
T ensureIsType(const QJsonObject &parent, const QString &key,
			   const Requirement requirement = Required,
			   const QString &what = "__placeholder__")
{
	const QString localWhat = QString(what).replace("__placeholder__", '\'' + key + '\'');
	if (!parent.contains(key))
	{
		throw JsonException(localWhat + "s parent does not contain " + localWhat);
	}
	return ensureIsType<T>(parent.value(key), requirement, localWhat);
}
template <typename T>
T ensureIsType(const QJsonObject &parent, const QString &key, const T default_,
			   const QString &what = "__placeholder__")
{
	const QString localWhat = QString(what).replace("__placeholder__", '\'' + key + '\'');
	if (!parent.contains(key))
	{
		return default_;
	}
	return ensureIsType<T>(parent.value(key), default_, localWhat);
}

template <typename T>
QList<T> ensureIsArrayOf(const QJsonValue &value, const Requirement = Required,
						 const QString &what = "Value")
{
	const QJsonArray array = ensureIsType<QJsonArray>(value, Required, what);
	QList<T> out;
	for (const QJsonValue val : array)
	{
		out.append(ensureIsType<T>(val, Required, what));
	}
	return out;
}
template <typename T>
QList<T> ensureIsArrayOf(const QJsonValue &value, const T default_,
						 const QString &what = "Value")
{
	if (value.isUndefined())
	{
		return default_;
	}
	return ensureIsArrayOf<T>(value, Required, what);
}
template <typename T>
QList<T> ensureIsArrayOf(const QJsonObject &parent, const QString &key,
						 const Requirement requirement = Required,
						 const QString &what = "__placeholder__")
{
	const QString localWhat = QString(what).replace("__placeholder__", '\'' + key + '\'');
	if (!parent.contains(key))
	{
		throw JsonException(localWhat + "s parent does not contain " + localWhat);
	}
	return ensureIsArrayOf<T>(parent.value(key), requirement, localWhat);
}
template <typename T>
QList<T> ensureIsArrayOf(const QJsonObject &parent, const QString &key,
						 const QList<T> &default_, const QString &what = "__placeholder__")
{
	const QString localWhat = QString(what).replace("__placeholder__", '\'' + key + '\'');
	if (!parent.contains(key))
	{
		return default_;
	}
	return ensureIsArrayOf<T>(parent.value(key), default_, localWhat);
}

// this macro part could be replaced by variadic functions that just pass on their arguments, but that wouldn't work well with IDE helpers
#define JSON_HELPERFUNCTIONS(NAME, TYPE) \
	inline TYPE ensure##NAME(const QJsonValue &value, const Requirement requirement = Required, const QString &what = "Value") \
{ return ensureIsType<TYPE>(value, requirement, what); } \
	inline TYPE ensure##NAME(const QJsonValue &value, const TYPE default_, const QString &what = "Value") \
{ return ensureIsType<TYPE>(value, default_, what); } \
	inline TYPE ensure##NAME(const QJsonObject &parent, const QString &key, const Requirement requirement = Required, const QString &what = "__placeholder__") \
{ return ensureIsType<TYPE>(parent, key, requirement, what); } \
	inline TYPE ensure##NAME(const QJsonObject &parent, const QString &key, const TYPE default_, const QString &what = "__placeholder") \
{ return ensureIsType<TYPE>(parent, key, default_, what); }

JSON_HELPERFUNCTIONS(Array, QJsonArray)
JSON_HELPERFUNCTIONS(Object, QJsonObject)
JSON_HELPERFUNCTIONS(String, QString)
JSON_HELPERFUNCTIONS(Boolean, bool)
JSON_HELPERFUNCTIONS(Double, double)
JSON_HELPERFUNCTIONS(Integer, int)
JSON_HELPERFUNCTIONS(DateTime, QDateTime)
JSON_HELPERFUNCTIONS(Url, QUrl)
JSON_HELPERFUNCTIONS(ByteArray, QByteArray)
JSON_HELPERFUNCTIONS(Dir, QDir)
JSON_HELPERFUNCTIONS(Uuid, QUuid)
JSON_HELPERFUNCTIONS(Variant, QVariant)

#undef JSON_HELPERFUNCTIONS

}
