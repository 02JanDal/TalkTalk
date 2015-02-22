#pragma once

#include "SyncedList.h"

class IrcServersModel : public SyncedListModel
{
	Q_OBJECT
public:
	explicit IrcServersModel(ServerConnection *server, QObject *parent = nullptr);

	enum Sections
	{
		IdSection = 1,
		NameSection,
		StatusSection,

		SecureSection,
		HostSection,
		PortSection,
		ServersSection,

		UsernameSection,
		PasswordSection,

		NickSection,
		NicksSection,
		RealNameSection,
		ConnectedSection
	};

	QVariant data(const QModelIndex &index, int role) const override;
	QHash<int, QByteArray> roleNames() const override;

	bool isConnected(const QModelIndex &index) const;

public slots:
	void connectServer(const QModelIndex &index);
	void disconnectServer(const QModelIndex &index);

	void add(const QString &displayName, const QString &host);
	void remove(const QModelIndex &index);

private slots:
	void propertyChanged(const QVariant &id, const QString &property);

signals:
	void connectedChanged(const QModelIndex &index);

private:
	void consume(const QString &channel, const QString &cmd, const QJsonObject &data) override {}
};
