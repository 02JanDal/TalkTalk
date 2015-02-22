#pragma once

#include <QAbstractItemModel>

#include "SyncedList.h"

class ChannelsModel : public QAbstractItemModel
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

private slots:
	void added(const QVariant &id);
	void removed(const QVariant &id);
	void changed(const QVariant &id, const QString &property);

private:
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
			User,
			Invalid
		} type;
		static Type typeFromString(const QString &str);
	};
	QList<Channel *> m_rootChannels;
	QHash<QString, Channel *> m_channels;
	QHash<QString, QList<Channel *>> m_channelsWithoutParents;
	SyncedList *m_list;

	QModelIndex index(Channel *channel) const;
};
