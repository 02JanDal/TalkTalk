#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include "AbstractConsumer.h"

class ChannelModel : public QAbstractListModel, public AbstractConsumer
{
	Q_OBJECT
public:
	explicit ChannelModel(ServerConnection *server, const QString &channelId, QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;

	void sendMessage(const QString &message);

private:
	void consume(const QString &channel, const QString &cmd, const QJsonObject &data) override;

	struct Message
	{
		// for color
		enum Type
		{
			Normal,
			Notice,
			Special, // joins, parts, quits etc.
			Action
		} type;

		QDateTime timestamp;
		QString source;
		QString message;
	};
	QList<Message> m_messages;

	QString m_channelId;
};
