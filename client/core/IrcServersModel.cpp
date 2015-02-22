#include "IrcServersModel.h"

#include <QColor>
#include <QDebug>

IrcServersModel::IrcServersModel(ServerConnection *server, QObject *parent)
	: SyncedListModel(server, "irc:servers", "", "id", parent)
{
	addEditable(0, "name");
	addMapping(IdSection, "id");
	addEditable(NameSection, "name");
	addMapping(StatusSection, "status");
	addEditable(SecureSection, "useSecure");
	addEditable(HostSection, "host");
	addEditable(PortSection, "port");
	addEditable(ServersSection, "servers");
	addEditable(UsernameSection, "userName");
	addEditable(PasswordSection, "password");
	addEditable(NickSection, "nickName");
	addEditable(NicksSection, "nickNames");
	addEditable(RealNameSection, "realName");
	addMapping(ConnectedSection, "connected");

	connect(m_list, &SyncedList::changed, this, &IrcServersModel::propertyChanged);
}

QVariant IrcServersModel::data(const QModelIndex &index, int role) const
{
	const QVariant d = SyncedListModel::data(index, role);
	if (index.column() == StatusSection)
	{
		const QString status = d.toString();
		if (role == Qt::DisplayRole || role == Qt::EditRole)
		{
			QString out = status;
			out[0] = out[0].toUpper();
			return out;
		}
		else if (role == Qt::ForegroundRole)
		{
			if (status == "inactive")
			{
				return QColor(Qt::gray);
			}
			else if (status == "waiting" || status == "connecting" || status == "closing")
			{
				return QColor(Qt::yellow);
			}
			else if (status == "connected")
			{
				return QColor(Qt::darkGreen);
			}
			else if (status == "closed")
			{
				return QColor(Qt::black);
			}
			else if (status == "error")
			{
				return QColor(Qt::darkRed);
			}
			else
			{
				return QVariant();
			}
		}
		else
		{
			return QVariant();
		}
	}
	else if (index.column() == 0 && (role == Qt::StatusTipRole || role == Qt::ToolTipRole))
	{
		return data(index.sibling(index.row(), StatusSection), Qt::DisplayRole);
	}
	return d;
}

QHash<int, QByteArray> IrcServersModel::roleNames() const
{
	QHash<int, QByteArray> out = SyncedListModel::roleNames();
	out.insert(IdSection, "id");
	out.insert(NameSection, "name");
	out.insert(StatusSection, "status");
	out.insert(SecureSection, "secure");
	out.insert(HostSection, "host");
	out.insert(PortSection, "port");
	out.insert(ServersSection, "servers");
	out.insert(UsernameSection, "userName");
	out.insert(PasswordSection, "password");
	out.insert(NickSection, "nickName");
	out.insert(NicksSection, "nickNames");
	out.insert(RealNameSection, "realName");
	return out;
}

bool IrcServersModel::isConnected(const QModelIndex &index) const
{
	if (!index.isValid())
	{
		return false;
	}
	return m_list->get(this->index(index.row(), IdSection).data(), "connected").toBool();
}

void IrcServersModel::connectServer(const QModelIndex &index)
{
	if (!index.isValid())
	{
		return;
	}
	const QString id = index.sibling(index.row(), IdSection).data().toString();
	emitMsg("irc:servers:" + id, "connect");
}
void IrcServersModel::disconnectServer(const QModelIndex &index)
{
	if (!index.isValid())
	{
		return;
	}
	const QString id = index.sibling(index.row(), IdSection).data().toString();
	emitMsg("irc:servers:" + id, "disconnect");
}

void IrcServersModel::add(const QString &displayName, const QString &host)
{
	emitMsg("irc:servers", "add", {{"displayName", displayName}, {"host", host}, {"userName", "TalkTalk"}, {"nickName", "TalkTalk"}, {"realName", "TalkTalk"}});
}
void IrcServersModel::remove(const QModelIndex &index)
{
	if (!index.isValid())
	{
		return;
	}
	emitMsg("irc:servers", "remove", {{"id", QJsonValue::fromVariant(this->index(index.row(), IdSection).data())}});
}

void IrcServersModel::propertyChanged(const QVariant &id, const QString &property)
{
	if (property == "connected")
	{
		emit connectedChanged(index(m_indices.indexOf(id), 0));
	}
}
