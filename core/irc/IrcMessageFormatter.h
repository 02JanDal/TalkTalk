#pragma once

class IrcMessage;
class QString;

namespace IrcMessageFormatter
{
QString messageType(IrcMessage *message);
QString messageSource(IrcMessage *message);
QString messageContent(IrcMessage* message);
}
