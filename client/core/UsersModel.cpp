#include "UsersModel.h"

#include "common/Json.h"

UsersModel::UsersModel(ServerConnection *server, const QString &channel, QObject *parent)
	: QAbstractItemModel(parent), AbstractConsumer(server), m_channelId(channel)
{
	subscribeTo("chat:channel:" + m_channelId);
	emitMsg("chat:channel:" + m_channelId, "wantUsers");
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

void UsersModel::consume(const QString &channel, const QString &cmd, const QJsonObject &data)
{
	using namespace Json;
	if (channel == "chat:channel:" + m_channelId)
	{
		if (cmd == "users")
		{
			QHash<QString, Group *> groups;
			for (const QJsonObject &obj : ensureIsArrayOf<QJsonObject>(data, "users"))
			{
				const QString status = ensureString(obj, "status");
				const QString mode = ensureString(obj, "mode");
				const QString name = ensureString(obj, "name");

				if (!groups.contains(mode))
				{
					groups.insert(mode, new Group{mode});
				}
				User *user = new User;
				user->name = name;
				user->group = groups[mode];
				user->status = status;
				groups[mode]->users.append(user);
			}

			beginResetModel();
			qDeleteAll(m_groups);
			qDeleteAll(m_users.values());
			m_groups = groups.values();
			m_groupMapping = groups;
			m_users.clear();
			for (const Group *group : m_groups)
			{
				for (User *user : group->users)
				{
					m_users.insert(user->name, user);
				}
			}
			endResetModel();
		}
		else if (cmd == "users:added")
		{
			const QString status = ensureString(data, "status");
			const QString mode = ensureString(data, "mode");
			const QString name = ensureString(data, "name");
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
			m_users.insert(name, user);
			endInsertRows();
		}
		else if (cmd == "users:removed")
		{
			const QString name = ensureString(data, "name");
			if (!m_users.contains(name))
			{
				return;
			}
			User *user = m_users[name];
			Group *group = user->group;
			const int row = group->users.indexOf(user);
			beginRemoveRows(index(group), row, row);
			group->users.removeAt(row);
			m_users.remove(name);
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
	}
}

QModelIndex UsersModel::index(UsersModel::Group *group) const
{
	return createIndex(m_groups.indexOf(group), 0, group);
}
