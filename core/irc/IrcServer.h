#pragma once

#include <QObject>
#include "core/ObjectWithId.h"
#include "core/AbstractClientConnection.h"

#include <QHash>

class IrcBuffer;
class IrcCommandParser;
class IrcConnection;
class IrcBufferModel;
class IrcMessage;
class IrcUserModel;
class SyncableQObjectList;

class IrcWrappedChannel : public QObject, public ObjectWithId
{
	Q_OBJECT
	OBJECTWITHID
	Q_PROPERTY(QObject *buffer READ buffer CONSTANT)
	Q_PROPERTY(QString parentId READ parentId CONSTANT)
	Q_PROPERTY(QString type READ type CONSTANT)
public:
	explicit IrcWrappedChannel(IrcBuffer *buffer, const QString &parentId, QObject *parent = nullptr);
	explicit IrcWrappedChannel(IrcBuffer *buffer, const QString &parentId, const QUuid &id, QObject *parent = nullptr);

	QString parentId() const { return m_parentId; }
	QObject *buffer() const;
	QString type() const;

private:
	IrcBuffer *m_buffer;
	QString m_parentId;
};

class IrcServer : public AbstractClientConnection, public ObjectWithId
{
	Q_OBJECT
	OBJECTWITHID
	Q_PROPERTY(QObject *connection READ connection CONSTANT)
	Q_PROPERTY(QString status READ status NOTIFY statusChanged)
public:
	explicit IrcServer(const QString &displayName, const QString &host, const QUuid &uuid, QObject *parent = nullptr);

	QObject *connection() const;
	QString status() const;

	void ready() override;

	Q_INVOKABLE void connectToHost(const QUuid &uuid = QUuid());
	Q_INVOKABLE void disconnectFromHost(const QUuid &uuid = QUuid());

	void addChannel(const QString &title, const QUuid &id, const bool connected);
	SyncableQObjectList *channelsList() const { return m_channelsList; }

signals:
	void statusChanged();
	void buffersChanged();

protected:
	void toClient(const QJsonObject &obj) override;

private slots:
	void addedBuffer(IrcBuffer *buffer);
	void removedBuffer(IrcBuffer *buffer);
	void messageReceived(IrcMessage *msg);

private:
	IrcCommandParser *m_parser;
	IrcConnection *m_connection;
	IrcBufferModel *m_bufferModel;
	SyncableQObjectList *m_channelsList;
	IrcWrappedChannel *m_serverChannel;
	QHash<IrcBuffer *, IrcUserModel *> m_userModels;
	QHash<IrcBuffer *, SyncableQObjectList *> m_syncedUserModels;
	QHash<IrcBuffer *, QString> m_bufferIds;
	QHash<QString, QUuid> m_predefinedBufferIds;
};

Q_DECLARE_LOGGING_CATEGORY(IRC)
