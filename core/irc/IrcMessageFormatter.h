#pragma once

class IrcMessage;
class QString;
class QStringList;

namespace IrcMessageFormatter
{
QString messageType(IrcMessage *message);
QString messageSource(IrcMessage *message);
QStringList messageContent(IrcMessage* message);
}
