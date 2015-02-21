#include "IrcMessageFormatter.h"

#include <IrcTextFormat>
#include <IrcConnection>
#include <QTime>
#include <Irc>

QString IrcMessageFormatter::messageType(IrcMessage *message)
{
	switch (message->type())
	{
	case IrcMessage::Capability:
	case IrcMessage::Join:
	case IrcMessage::Kick:
	case IrcMessage::Mode:
	case IrcMessage::Nick:
	case IrcMessage::Part:
	case IrcMessage::Quit:
	case IrcMessage::Topic:
		return "special";
	case IrcMessage::Invite:
	case IrcMessage::Motd:
	case IrcMessage::Notice:
	case IrcMessage::Whois:
	case IrcMessage::Whowas:
	case IrcMessage::WhoReply:
	case IrcMessage::Monitor:
		return "notice";
	case IrcMessage::Private:
		return static_cast<IrcPrivateMessage *>(message)->isAction() ? "action" : "normal";
	default:
		return QString();
	}
}

QString IrcMessageFormatter::messageSource(IrcMessage *message)
{
	switch (message->type())
	{
	case IrcMessage::Join:
		return "-->";
	case IrcMessage::Nick:
		return "<->";
	case IrcMessage::Kick:
		return "<-*";
	case IrcMessage::Part:
	case IrcMessage::Quit:
		return "<--";
	case IrcMessage::Capability:
	case IrcMessage::Mode:
	case IrcMessage::Topic:
		return "*";
	case IrcMessage::Invite:
	case IrcMessage::Notice:
		return message->nick();
	case IrcMessage::Motd:
	case IrcMessage::Whois:
	case IrcMessage::Whowas:
	case IrcMessage::WhoReply:
	case IrcMessage::Monitor:
		return "*";
	case IrcMessage::Private:
		return static_cast<IrcPrivateMessage *>(message)->isAction() ? "-*-" : message->nick();
	default:
		return QString();
	}
}

QString IrcMessageFormatter::messageContent(IrcMessage* message)
{
	switch (message->type())
	{
	case IrcMessage::Join:
	{
		const QString channel = static_cast<IrcJoinMessage *>(message)->channel();
		if (message->isOwn())
		{
			return QObject::tr("You have joined %1 as %2").arg(channel, message->nick());
		}
		else
		{
			return QObject::tr("%1 has joined %2").arg(message->nick(), channel);
		}
	}
	case IrcMessage::Nick:
	{
		IrcNickMessage *msg = static_cast<IrcNickMessage *>(message);
		if (message->isOwn())
		{
			return QObject::tr("You are now known as %1").arg(msg->newNick());
		}
		else
		{
			return QObject::tr("%1 is now known as %2").arg(msg->oldNick(), msg->newNick());
		}
	}
	case IrcMessage::Kick:
	{
		IrcKickMessage *msg = static_cast<IrcKickMessage *>(message);
		if (message->isOwn())
		{
			return QObject::tr("You where kicked from %1 by %2: %3").arg(msg->channel(), msg->nick(), msg->reason());
		}
		else
		{
			return QObject::tr("%1 was kicked from %2 by %3: %4").arg(msg->user(), msg->channel(), msg->nick(), msg->reason());
		}
	}
	case IrcMessage::Part:
	{
		IrcPartMessage *msg = static_cast<IrcPartMessage *>(message);
		return QObject::tr("%1 (%2) has left %3 (%4)").arg(msg->nick(), msg->ident(), msg->channel(), msg->reason());
	}
	case IrcMessage::Quit:
	{
		IrcQuitMessage *msg = static_cast<IrcQuitMessage *>(message);
		return QObject::tr("%1 (%2) has quit (%3)").arg(msg->nick(), msg->ident(), msg->reason());
	}
	case IrcMessage::Capability:
	case IrcMessage::Mode:
	{
		IrcModeMessage *msg = static_cast<IrcModeMessage *>(message);
		if (msg->isReply())
		{
			return QObject::tr("%1 mode is %2 %3").arg(msg->target(), msg->mode(), msg->arguments().join(' '));
		}
		else
		{
			return QObject::tr("%1 sets mode %2 %3 %4").arg(msg->nick(), msg->target(), msg->mode(), msg->arguments().join(' '));
		}
	}
	case IrcMessage::Topic:
	{
		IrcTopicMessage *msg = static_cast<IrcTopicMessage *>(message);
		return QObject::tr("%1 has changed the topic for %2 to \"%3\"").arg(msg->nick(), msg->channel(), msg->topic());
	}
	case IrcMessage::Invite:
	{
		IrcInviteMessage *msg = static_cast<IrcInviteMessage *>(message);
		return QObject::tr("%1 was invited to %2 by %3").arg(msg->user(), msg->channel(), msg->nick());
	}
	case IrcMessage::Notice:
		return static_cast<IrcNoticeMessage *>(message)->content();
	case IrcMessage::Motd:
		return static_cast<IrcMotdMessage *>(message)->lines().join('\n');
	case IrcMessage::Whois:
	case IrcMessage::Whowas:
	case IrcMessage::WhoReply:
	case IrcMessage::Monitor:
		return "TODO: Implement";
	case IrcMessage::Private:
	{
		IrcPrivateMessage *msg = static_cast<IrcPrivateMessage *>(message);
		const QString content = IrcTextFormat().toHtml(msg->content());
		if (msg->isAction())
		{
			return QObject::tr("%1 %2").arg(msg->nick(), content);
		}
		else
		{
			return content;
		}
	}
	default:
		return QString();
	}
}
