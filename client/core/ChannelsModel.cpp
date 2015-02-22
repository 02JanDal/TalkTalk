#include "ChannelsModel.h"

#include "common/Json.h"

ChannelsModel::ChannelsModel(ServerConnection *server, QObject *parent)
	: QAbstractItemModel(parent)
{
	m_list = new SyncedList(server, "chat:channels", "", "id", this);
	connect(m_list, &SyncedList::added, this, &ChannelsModel::added);
	connect(m_list, &SyncedList::removed, this, &ChannelsModel::removed);
	connect(m_list, &SyncedList::changed, this, &ChannelsModel::changed);
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

void ChannelsModel::added(const QVariant &id)
{
	Channel *channel = new Channel;
	channel->id = id.toString();
	channel->name = m_list->get(id, "name").toString();
	channel->active = m_list->get(id, "active").toBool();
	channel->type = Channel::typeFromString(m_list->get(id, "type").toString());
	const QString parent = m_list->get(id, "parent").toString();
	if (!parent.isEmpty() && m_channels.contains(parent))
	{
		channel->parent = m_channels[parent];

		const int row = channel->parent->children.size();
		beginInsertRows(index(channel->parent), row, row);
		channel->parent->children.append(channel);
		m_channels.insert(id.toString(), channel);
		endInsertRows();
	}
	else
	{
		const int row = m_rootChannels.size();
		beginInsertRows(QModelIndex(), row, row);
		m_rootChannels.append(channel);
		m_channels.insert(id.toString(), channel);
		if (!parent.isEmpty())
		{
			m_channelsWithoutParents[parent].append(channel);
		}
		endInsertRows();
	}

	if (m_channelsWithoutParents.contains(id.toString()))
	{
		for (Channel *c : m_channelsWithoutParents.value(id.toString()))
		{
			const QModelIndex source = index(c);
			beginMoveRows(source.parent(), source.row(), source.row(), index(channel), channel->children.size());
			m_rootChannels.removeAll(c);
			channel->children.append(c);
			c->parent = channel;
			endMoveRows();
		}
	}
}
void ChannelsModel::removed(const QVariant &id)
{
	Channel *channel = m_channels[id.toString()];
	Q_ASSERT(channel->parent); // we can't remove the root element
	const int row = channel->parent->children.indexOf(channel);

	beginRemoveRows(index(channel), row, row);
	channel->parent->children.removeAt(row);
	m_channels.remove(id.toString());
	endRemoveRows();

	delete channel;
}
void ChannelsModel::changed(const QVariant &id, const QString &property)
{
	Channel *channel = m_channels[id.toString()];
	if (property == "name")
	{
		emit dataChanged(index(channel), index(channel), QVector<int>() << Qt::DisplayRole);
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

ChannelsModel::Channel::Type ChannelsModel::Channel::typeFromString(const QString &str)
{
	if (str == "pound")
	{
		return Pound;
	}
	else if (str == "user")
	{
		return User;
	}
	else
	{
		return Invalid;
	}
}
