#include "ChannelModel.h"

#include <QColor>

#include "common/Json.h"

ChannelModel::ChannelModel(ServerConnection *server, const QString &channelId, QObject *parent)
	: QAbstractListModel(parent), AbstractConsumer(server), m_channelId(channelId)
{
	subscribeTo("chat:channel:" + m_channelId);
}

int ChannelModel::rowCount(const QModelIndex &parent) const
{
	return m_messages.size();
}
int ChannelModel::columnCount(const QModelIndex &parent) const
{
	return 3;
}
QVariant ChannelModel::data(const QModelIndex &index, int role) const
{
	Message msg = m_messages.at(index.row());
	if (role == Qt::DisplayRole)
	{
		switch (index.column())
		{
		case 0: return msg.timestamp.toString("hh:mm:ss");
		case 1:
			switch (msg.type)
			{
			case Message::Normal:
				return '<' + msg.source + '>';
			case Message::Notice:
				return '[' + msg.source + ']';
			case Message::Special:
			case Message::Action:
				return msg.source;
			}
		case 2: return msg.message;
		}
	}
	else if (role == Qt::ForegroundRole)
	{
		if (index.column() > 0)
		{
			switch (msg.type)
			{
			case Message::Normal:
				return QColor(Qt::black);
			case Message::Notice:
				return QColor(Qt::darkYellow);
			case Message::Special:
				return QColor(Qt::darkMagenta);
			case Message::Action:
				return QColor(Qt::darkBlue);
			}
		}
	}
	return QVariant();
}

void ChannelModel::sendMessage(const QString &message)
{
	emitMsg("chat:channel:" + m_channelId, "send", {{"msg", message}});
}

void ChannelModel::consume(const QString &channel, const QString &cmd, const QJsonObject &data)
{
	using namespace Json;

	if (channel == "chat:channel:" + m_channelId)
	{
		if (cmd == "message")
		{
			Message msg;
			msg.source = ensureString(data, "from");
			msg.message = ensureString(data, "content");
			msg.timestamp = QDateTime::fromMSecsSinceEpoch(ensureInteger(data, "timestamp"));

			const QString type = ensureString(data, "type");
			if (type == "normal")
			{
				msg.type = Message::Normal;
			}
			else if (type == "notice")
			{
				msg.type = Message::Notice;
			}
			else if (type == "special")
			{
				msg.type = Message::Special;
			}
			else if (type == "action")
			{
				msg.type = Message::Action;
			}

			beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
			m_messages.append(msg);
			endInsertRows();
		}
		else if (cmd == "info")
		{
			const QString name = ensureString(data, "title");
			const bool active = ensureBoolean(data, QStringLiteral("active"));
			const QString type = ensureString(data, "type");
			// TODO do stuff
		}
	}
}
