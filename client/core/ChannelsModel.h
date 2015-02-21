#pragma once

#include <QAbstractItemModel>
#include "AbstractConsumer.h"

class ChannelsModel : public QAbstractItemModel, public AbstractConsumer
{
	Q_OBJECT
public:
	explicit ChannelsModel(ServerConnection *server, QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent) const override;
	QModelIndex parent(const QModelIndex &child) const override;
	QVariant data(const QModelIndex &index, int role) const override;

	QString channelId(const QModelIndex &index) const;

private:
	void consume(const QString &channel, const QString &cmd, const QJsonObject &data) override;

	void channelAdded(const QString &parent, const QString &id);
	void channelRemoved(const QString &id);

	struct Channel
	{
		Channel *parent = nullptr;
		QList<Channel *> children;
		QString id;
		QString name;
		bool active;
		enum Type
		{
			Pound, // IRC channels etc.
			User
		} type;
	};
	QList<Channel *> m_rootChannels;
	QHash<QString, Channel *> m_channels;
	QHash<QString, QList<Channel *>> m_channelsWithoutParents;

	QModelIndex index(Channel *channel) const;
};
