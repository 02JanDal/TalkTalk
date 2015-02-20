// Licensed under the Apache-2.0 license. See README.md for details.

#include "Json.h"

#include <QFile>
#include <QSaveFile>

#include "FileSystem.h"

void Json::write(const QJsonDocument &doc, const QString &filename)
{
	FS::write(filename, doc.toJson());
}
void Json::write(const QJsonObject &object, const QString &filename)
{
	write(QJsonDocument(object), filename);
}
void Json::write(const QJsonArray &array, const QString &filename)
{
	write(QJsonDocument(array), filename);
}

QByteArray Json::toBinary(const QJsonObject &obj)
{
	return QJsonDocument(obj).toBinaryData();
}
QByteArray Json::toBinary(const QJsonArray &array)
{
	return QJsonDocument(array).toBinaryData();
}
QByteArray Json::toText(const QJsonObject &obj)
{
	return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
QByteArray Json::toText(const QJsonArray &array)
{
	return QJsonDocument(array).toJson(QJsonDocument::Compact);
}

static bool isBinaryJson(const QByteArray &data)
{
	decltype(QJsonDocument::BinaryFormatTag) tag = QJsonDocument::BinaryFormatTag;
	return memcmp(data.constData(), &tag, sizeof(QJsonDocument::BinaryFormatTag)) == 0;
}
QJsonDocument Json::ensureDocument(const QByteArray &data)
{
	if (isBinaryJson(data))
	{
		QJsonDocument doc = QJsonDocument::fromBinaryData(data);
		if (doc.isNull())
		{
			throw JsonException("Invalid JSON (binary JSON detected)");
		}
		return doc;
	}
	else
	{
		QJsonParseError error;
		QJsonDocument doc = QJsonDocument::fromJson(data, &error);
		if (error.error != QJsonParseError::NoError)
		{
			throw JsonException("Error parsing JSON: " + error.errorString());
		}
		return doc;
	}
}
QJsonDocument Json::ensureDocument(const QString &filename)
{
	return Json::ensureDocument(FS::read(filename));
}
QJsonObject Json::ensureObject(const QJsonDocument &doc, const QString &what)
{
	if (!doc.isObject())
	{
		throw JsonException(what + " is not an object");
	}
	return doc.object();
}
QJsonArray Json::ensureArray(const QJsonDocument &doc, const QString &what)
{
	if (!doc.isArray())
	{
		throw JsonException(what + " is not an array");
	}
	return doc.array();
}

QJsonObject Json::ensureObject(const QJsonValue &value, const QString &what)
{
	if (!value.isObject())
	{
		throw JsonException(what + " is not an object");
	}
	return value.toObject();
}
QJsonObject Json::ensureObject(const QJsonObject &parent, const QString &key,
							   const QString &what)
{
	if (!parent.contains(key))
	{
		throw JsonException(what + "s parent does not contain " + what);
	}
	return Json::ensureObject(parent.value(key), what);
}

QJsonArray Json::ensureArray(const QJsonValue &value, const QString &what)
{
	if (!value.isArray())
	{
		throw JsonException(what + " is not an array");
	}
	return value.toArray();
}
QJsonArray Json::ensureArray(const QJsonObject &parent, const QString &key, const QString &what)
{
	if (!parent.contains(key))
	{
		throw JsonException(what + "s parent does not contain " + what);
	}
	return Json::ensureArray(parent.value(key), what);
}

template<>
QJsonValue Json::toJson<QUrl>(const QUrl &url)
{
	return QJsonValue(url.toString(QUrl::FullyEncoded));
}
template<>
QJsonValue Json::toJson<QByteArray>(const QByteArray &data)
{
	return QJsonValue(QString::fromLatin1(data.toHex()));
}
template<>
QJsonValue Json::toJson<QDateTime>(const QDateTime &datetime)
{
	return QJsonValue(datetime.toString(Qt::ISODate));
}
template<>
QJsonValue Json::toJson<QDir>(const QDir &dir)
{
	return QDir::current().relativeFilePath(dir.absolutePath());
}
template<>
QJsonValue Json::toJson<QUuid>(const QUuid &uuid)
{
	return uuid.toString();
}
template<>
QJsonValue Json::toJson<QVariant>(const QVariant &variant)
{
	return QJsonValue::fromVariant(variant);
}
