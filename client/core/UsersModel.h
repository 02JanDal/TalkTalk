#pragma once

#include <QAbstractItemModel>
#include <QHash>
#include "AbstractConsumer.h"

class UsersModel : public QAbstractItemModel, public AbstractConsumer
{
	Q_OBJECT
public:
	explicit UsersModel(ServerConnection *server, const QString &channel, QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent) const override;
	QModelIndex parent(const QModelIndex &child) const override;
	QVariant data(const QModelIndex &index, int role) const override;

private:
	void consume(const QString &channel, const QString &cmd, const QJsonObject &data) override;
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

	QModelIndex index(Group *group) const;
};
