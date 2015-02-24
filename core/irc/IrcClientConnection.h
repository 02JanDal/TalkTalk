#pragma once

#include "core/AbstractClientConnection.h"
#include "common/BaseConfigObject.h"

class SyncableQObjectList;

class IrcClientConnection : public AbstractClientConnection, public BaseConfigObject
{
	Q_OBJECT
public:
	explicit IrcClientConnection(QObject *parent = nullptr);

	void ready() override;

protected:
	void toClient(const QJsonObject &obj) override;

	QByteArray doSave() const override;
	void doLoad(const QByteArray &data) override;

private:
	SyncableQObjectList *m_servers;
};
