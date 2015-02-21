#include "ChannelsModel.h"

#include "common/Json.h"

ChannelsModel::ChannelsModel(ServerConnection *server, QObject *parent)
	: QAbstractItemModel(parent), AbstractConsumer(server)
{
	subscribeTo("chat:channels");
	emitMsg("chat:channels", "list");
}

int ChannelsModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
	{
		return static_cast<Channel *>(parent.internalPointer())->children.size();
	}
	else
	{
		return m_rootChannels.size();
	}
}
int ChannelsModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}
QModelIndex ChannelsModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
	{
		return QModelIndex();
	}

	if (parent.isValid())
	{
		Channel *parentChannel = static_cast<Channel *>(parent.internalPointer());
		return createIndex(row, column, parentChannel->children.at(row));
	}
	else
	{
		return createIndex(row, column, m_rootChannels.at(row));
	}
}
QModelIndex ChannelsModel::parent(const QModelIndex &child) const
{
	if (!child.isValid())
	{
		return QModelIndex();
	}

	Channel *childChannel = static_cast<Channel *>(child.internalPointer());
	Channel *parentChannel = childChannel->parent;
	if (parentChannel)
	{
		return index(parentChannel);
	}
	else
	{
		return QModelIndex();
	}
}
QVariant ChannelsModel::data(const QModelIndex &index, int role) const
{
	Channel *channel = static_cast<Channel *>(index.internalPointer());
	if (index.column() == 0 && role == Qt::DisplayRole)
	{
		return channel->name.isEmpty() ? channel->id : channel->name;
	}
	else
	{
		return QVariant();
	}
}

QString ChannelsModel::channelId(const QModelIndex &index) const
{
	if (!index.isValid() || index.model() != this)
	{
		return QString();
	}
	return static_cast<Channel *>(index.internalPointer())->id;
}

void ChannelsModel::consume(const QString &channel, const QString &cmd, const QJsonObject &data)
{
	using namespace Json;

	if (channel == "chat:channels")
	{
		if (cmd == "all")
		{
			const QList<QJsonObject> objs = ensureIsArrayOf<QJsonObject>(data, "channels");
			QMap<QString, QString> channels;
			for (const QJsonObject &obj : objs)
			{
				channels.insert(ensureString(obj, "id"), ensureString(obj, "parent"));
			}
			for (const QString &channel : channels.keys())
			{
				if (!m_channels.contains(channel))
				{
					channelAdded(channels[channel], channel);
				}
			}
			for (const QString &channel : m_channels.keys())
			{
				if (!channels.contains(channel))
				{
					channelRemoved(channel);
				}
			}
		}
		else if (cmd == "added")
		{
			const QString channel = ensureString(data, "id");
			const QString parent = ensureString(data, "parent");
			if (!m_channels.contains(channel))
			{
				channelAdded(parent, channel);
			}
		}
		else if (cmd == "removed")
		{
			const QString channel = ensureString(data, "id");
			if (m_channels.contains(channel))
			{
				channelRemoved(channel);
			}
		}
	}
	else if (channel.startsWith("chat:channel:") && m_channels.contains(QString(channel).remove("chat:channel:")))
	{
		if (cmd == "info")
		{
			const QString channelId = QString(channel).remove("chat:channel:");
			Channel *channel = m_channels[channelId];

			const QString name = ensureString(data, "title");
			const bool active = ensureBoolean(data, QStringLiteral("active"));
			const QString type = ensureString(data, "type");

			channel->name = name;
			channel->active= active;
			if (type == "pound")
			{
				channel->type = Channel::Pound;
			}
			else if (type == "user")
			{
				channel->type = Channel::User;
			}
			emit dataChanged(index(channel), index(channel));
		}
	}
}

QModelIndex ChannelsModel::index(ChannelsModel::Channel *channel) const
{
	if (channel->parent)
	{
		return createIndex(channel->parent->children.indexOf(channel), 0, channel);
	}
	else
	{
		return createIndex(m_rootChannels.indexOf(channel), 0, channel);
	}
}

void ChannelsModel::channelAdded(const QString &parent, const QString &id)
{
	Channel *channel = new Channel;
	channel->id = id;
	if (!parent.isEmpty() && m_channels.contains(parent))
	{
		channel->parent = m_channels[parent];

		const int row = channel->parent->children.size();
		beginInsertRows(index(channel->parent), row, row);
		channel->parent->children.append(channel);
		m_channels.insert(id, channel);
		endInsertRows();
	}
	else
	{
		const int row = m_rootChannels.size();
		beginInsertRows(QModelIndex(), row, row);
		m_rootChannels.append(channel);
		m_channels.insert(id, channel);
		if (!parent.isEmpty())
		{
			m_channelsWithoutParents[parent].append(channel);
		}
		endInsertRows();
	}

	if (m_channelsWithoutParents.contains(parent))
	{
		for (Channel *c : m_channelsWithoutParents.value(parent))
		{
			const QModelIndex source = index(c);
			beginMoveRows(source.parent(), source.row(), source.row(), index(channel), channel->children.size());
			m_rootChannels.removeAll(c);
			channel->children.append(c);
			c->parent = channel;
			endMoveRows();
		}
	}

	subscribeTo("chat:channel:" + id);
	emitMsg("chat:channel:" + id, "wantInfo");
}
void ChannelsModel::channelRemoved(const QString &id)
{
	unsubscribeFrom("chat:channel:" + id);

	Channel *channel = m_channels[id];
	Q_ASSERT(channel->parent); // we can't remove the root element
	const int row = channel->parent->children.indexOf(channel);

	beginRemoveRows(index(channel), row, row);
	channel->parent->children.removeAt(row);
	m_channels.remove(id);
	endRemoveRows();

	delete channel;
}
