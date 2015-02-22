#include "UsersModel.h"

#include "common/Json.h"
#include "SyncedList.h"

UsersModel::UsersModel(ServerConnection *server, const QString &channel, QObject *parent)
	: QAbstractItemModel(parent), m_channelId(channel), m_syncedList(new SyncedList(server, "chat:channel:" + channel, "users", "id", this))
{
	connect(m_syncedList, &SyncedList::added, this, &UsersModel::added);
	connect(m_syncedList, &SyncedList::removed, this, &UsersModel::removed);
}

int UsersModel::rowCount(const QModelIndex &parent) const
{
	if (!parent.isValid())
	{
		return m_groups.size();
	}
	else if (!parent.parent().isValid())
	{
		return static_cast<Group *>(parent.internalPointer())->users.size();
	}
	else
	{
		return 0;
	}
}
int UsersModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}
QModelIndex UsersModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!parent.isValid())
	{
		return createIndex(row, column, m_groups.at(row));
	}
	else
	{
		return createIndex(row, column, static_cast<Group *>(parent.internalPointer())->users.at(row));
	}
}
QModelIndex UsersModel::parent(const QModelIndex &child) const
{
	if (!child.isValid())
	{
		return QModelIndex();
	}
	if (m_groups.contains(static_cast<Group *>(child.internalPointer())))
	{
		// groups are toplevel and thus don't have a parent
		return QModelIndex();
	}
	else
	{
		User *user = static_cast<User *>(child.internalPointer());
		return createIndex(m_groups.indexOf(user->group), 0, user->group);
	}
}
QVariant UsersModel::data(const QModelIndex &index, int role) const
{
	if (index.parent().isValid())
	{
		// has parent = user
		User *user = static_cast<User *>(index.internalPointer());
		if (role == Qt::DisplayRole)
		{
			return user->name;
		}
	}
	else
	{
		Group *group = static_cast<Group *>(index.internalPointer());
		if (role == Qt::DisplayRole)
		{
			return group->title;
		}
	}
	return QVariant();
}

void UsersModel::added(const QVariant &id)
{
	const QString status = m_syncedList->get(id, "status").toString();
	const QString mode = m_syncedList->get(id, "mode").toString();
	const QString name = m_syncedList->get(id, "name").toString();
	if (!m_groupMapping.contains(mode))
	{
		Group *group = new Group{mode};
		beginInsertRows(QModelIndex(), m_groups.size(), m_groups.size());
		m_groupMapping.insert(mode, group);
		m_groups.append(group);
		endInsertRows();
	}
	Group *group = m_groupMapping[mode];
	User *user = new User;
	user->name = name;
	user->status = status;
	user->group = group;
	beginInsertRows(index(group), group->users.size(), group->users.size());
	group->users.append(user);
	m_users.insert(id.toString(), user);
	endInsertRows();
}

void UsersModel::removed(const QVariant &id)
{
	const QString idString = id.toString();
	if (!m_users.contains(idString))
	{
		return;
	}
	User *user = m_users[idString];
	Group *group = user->group;
	const int row = group->users.indexOf(user);
	beginRemoveRows(index(group), row, row);
	group->users.removeAt(row);
	m_users.remove(idString);
	endRemoveRows();
	delete user;

	if (group->users.isEmpty())
	{
		const int groupRow = m_groups.indexOf(group);
		beginRemoveRows(QModelIndex(), groupRow, groupRow);
		m_groups.removeAt(groupRow);
		m_groupMapping.remove(group->title);
		endRemoveRows();
		delete group;
	}
}

QModelIndex UsersModel::index(UsersModel::Group *group) const
{
	return createIndex(m_groups.indexOf(group), 0, group);
}
