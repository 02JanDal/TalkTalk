#pragma once

#include <QAbstractItemModel>
#include <QHash>

class SyncedList;
class ServerConnection;

class UsersModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit UsersModel(ServerConnection *server, const QString &channel, QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent) const override;
	QModelIndex parent(const QModelIndex &child) const override;
	QVariant data(const QModelIndex &index, int role) const override;

private slots:
	void added(const QVariant &id);
	void removed(const QVariant &id);

private:
	QString m_channelId;

	struct Group;
	struct User
	{
		QString name;
		QString status;
		Group *group = nullptr;
	};
	struct Group
	{
		QString title;
		QList<User *> users;
	};
	QList<Group *> m_groups;
	QHash<QString, Group *> m_groupMapping;
	QHash<QString, User *> m_users;
	SyncedList *m_syncedList;

	QModelIndex index(Group *group) const;
};
