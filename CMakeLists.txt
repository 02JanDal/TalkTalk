cmake_minimum_required(VERSION 2.8.9)

project(TalkTalk)

include(FeatureSummary)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(Qt5 REQUIRED QUIET COMPONENTS Core Network)
find_package(Qt5 COMPONENTS Core Gui Widgets WebSockets Sql)
find_package(LibCommuni)
find_package(KF5GuiAddons)
find_package(KF5WidgetsAddons)
set_package_properties(LibCommuni PROPERTIES
	URL http://communi.github.io/
	DESCRIPTION "Communi provides a set of tools for enabling IRC connectivity in Qt-based C++ and QML applications."
	TYPE OPTIONAL
	PURPOSE "Enables support for connecting to IRC with the core"
)
set_package_properties(Qt5Core PROPERTIES
	URL http://qt.io/
	DESCRIPTION "Provides core non-GUI functionality"
	TYPE REQUIRED
	PURPOSE "Required for all parts of TalkTalk"
)
set_package_properties(Qt5Gui PROPERTIES
	URL http://qt.io/
	DESCRIPTION "Qt GUI module provides the basic enablers for graphical applications written with Qt"
	TYPE OPTIONAL
	PURPOSE "Required for building any of the clients"
)
set_package_properties(Qt5Widgets PROPERTIES
	URL http://qt.io/
	DESCRIPTION "Qt Widgets module extends Qt GUI with C++ widget functionality"
	TYPE OPTIONAL
	PURPOSE "Required for building the widgets client"
)
set_package_properties(Qt5Network PROPERTIES
	URL http://qt.io/
	DESCRIPTION "Provides classes to make network programming easier and portable"
	TYPE REQUIRED
	PURPOSE "Since most parts of TalkTalk need some sort of network access QtNetwork is required"
)
set_package_properties(Qt5WebSockets PROPERTIES
	URL http://qt.io/
	DESCRIPTION "List of C++ classes that provide WebSockets communication"
	TYPE OPTIONAL
	PURPOSE "Enables support for websocket connections to the core"
)
set_package_properties(Qt5Sql PROPERTIES
	URL http://qt.io/
	DESCRIPTION "Provides a driver layer, SQL API layer, and a user interface layer for SQL databases"
	TYPE OPTIONAL
	PURPOSE "Required for building persistant backlog support"
)
set_package_properties(KF5WidgetsAddons PROPERTIES
	URL https://www.kde.org/
	DESCRIPTION "Large set of desktop widgets"
	TYPE OPTIONAL
	PURPOSE "Required for building the widgets client"
)
set_package_properties(KF5GuiAddons PROPERTIES
	URL https://www.kde.org/
	DESCRIPTION "The KDE GUI addons provide utilities for graphical user interfaces in the areas of colors, fonts, text, images, keyboard input."
	TYPE OPTIONAL
	PURPOSE "Required for building the widgets client"
)

option(BUILD_CORE "Build the TalkTalk Core" ON)
option(BUILD_WIDGETS_CLIENT "Build the TalkTalk Widgets Client" ON)
option(BUILD_CORE_BACKLOG "Enable persisting backlog to a database" ON)
option(BUILD_CORE_WEBSOCKETS "Make the core accept WebSocket connections" ON)
option(BUILD_CORE_TCP "Make the core accept TCP connections" ON)
add_feature_info(Core BUILD_CORE "Build the TalkTalk Core")
add_feature_info(WidgetsClient BUILD_WIDGETS_CLIENT "Build the TalkTalk Widgets Client")
add_feature_info(Backlog BUILD_CORE_BACKLOG "Build the core with support for persisting the backlog to a database")
add_feature_info(WebSockets BUILD_CORE_WEBSOCKETS "Build the core with support for accepting WebSocket connections")
add_feature_info(Tcp BUILD_CORE_TCP "Build the core with support for accepting TCP connections")

set(CORE_SRC
	common/Json.h
	common/Json.cpp
	common/FileSystem.h
	common/FileSystem.cpp
	common/Exception.h
	common/BaseConfigObject.h
	common/BaseConfigObject.cpp

	core/ConnectionManager.h
	core/ConnectionManager.cpp
	core/AbstractClientConnection.h
	core/AbstractClientConnection.cpp
	core/SyncableList.h
	core/SyncableList.cpp
	core/ObjectWithId.h
	core/Plugin.h
)
set(CLIENT_LIB_SRC
	common/Json.h
	common/Json.cpp
	common/FileSystem.h
	common/FileSystem.cpp
	common/Exception.h
	common/TcpUtils.h
	common/TcpUtils.cpp

	client/core/ServerConnection.h
	client/core/ServerConnection.cpp
	client/core/ChannelModel.h
	client/core/ChannelModel.cpp
	client/core/ChannelsModel.h
	client/core/ChannelsModel.cpp
	client/core/UsersModel.h
	client/core/UsersModel.cpp
	client/core/IrcServersModel.h
	client/core/IrcServersModel.cpp
	client/core/MonitorModel.h
	client/core/MonitorModel.cpp
	client/core/AbstractConsumer.h
	client/core/AbstractConsumer.cpp
	client/core/SyncedList.h
	client/core/SyncedList.cpp
)
set(WIDGETS_CLIENT_SRC
	client/main_widgetsgui.cpp
	client/widgetsgui/MainWindow.h
	client/widgetsgui/MainWindow.cpp
	client/widgetsgui/pages/IRCServersPage.h
	client/widgetsgui/pages/IRCServersPage.cpp
	client/widgetsgui/ChatView.h
	client/widgetsgui/ChatView.cpp
)
set(WIDGETS_CLIENT_UI
	client/widgetsgui/MainWindow.ui
	client/widgetsgui/pages/IRCServersPage.ui
)

set(CORE_EXTRA_QT )
set(CORE_EXTRA_LIBS )
if(${Qt5WebSockets_FOUND})
	list(APPEND CORE_SRC
		core/websockets/WebSocketClientConnection.h
		core/websockets/WebSocketClientConnection.cpp
		core/websockets/WebSocketServer.h
		core/websockets/WebSocketServer.cpp
		core/websockets/WebSocketsPlugin.h
		core/websockets/WebSocketsPlugin.cpp
	)
	list(APPEND CORE_EXTRA_QT WebSockets)
	add_definitions(-DTALKTALK_CORE_WEBSOCKETS)
endif()
if(${LIBCOMMUNI_FOUND})
	list(APPEND CORE_SRC
		core/irc/IrcClientConnection.h
		core/irc/IrcClientConnection.cpp
		core/irc/IrcServer.h
		core/irc/IrcServer.cpp
		core/irc/IrcMessageFormatter.h
		core/irc/IrcMessageFormatter.cpp
		core/irc/IrcPlugin.h
		core/irc/IrcPlugin.cpp
	)
	list(APPEND CORE_EXTRA_LIBS IrcCore IrcModel IrcUtil)
	add_definitions(-DTALKTALK_CORE_IRC)
	set_package_properties(Qt5Network PROPERTIES TYPE REQUIRED)
endif()
if(BUILD_CORE_BACKLOG)
	list(APPEND CORE_SRC
		core/backlog/BacklogClientConnection.h
		core/backlog/BacklogClientConnection.cpp
		core/backlog/BacklogPlugin.h
		core/backlog/BacklogPlugin.cpp
		core/backlog/SqlHelpers.h
		core/backlog/SqlHelpers.cpp
	)
	list(APPEND CORE_EXTRA_QT Sql)
	add_definitions(-DTALKTALK_CORE_BACKLOG)
	set_package_properties(Qt5Sql PROPERTIES TYPE REQUIRED)
endif()
if(BUILD_CORE_TCP)
	list(APPEND CORE_SRC
		common/TcpUtils.h
		common/TcpUtils.cpp
		core/tcp/TcpServer.h
		core/tcp/TcpServer.cpp
		core/tcp/TcpClientConnection.h
		core/tcp/TcpClientConnection.cpp
		core/tcp/TcpPlugin.h
		core/tcp/TcpPlugin.cpp
	)
	list(APPEND CORE_EXTRA_QT Network)
	add_definitions(-DTALKTALK_CORE_TCP)
endif()

if(BUILD_CORE)
	add_library(TalkTalkCoreLib ${CORE_SRC})
	qt5_use_modules(TalkTalkCoreLib Core Network ${CORE_EXTRA_QT})
	target_link_libraries(TalkTalkCoreLib ${CORE_EXTRA_LIBS})

	add_executable(TalkTalkCore core/main.cpp)
	qt5_use_modules(TalkTalkCore Core Network ${CORE_EXTRA_QT})
	target_link_libraries(TalkTalkCore TalkTalkCoreLib ${CORE_EXTRA_LIBS})
endif()

if(BUILD_WIDGETS_CLIENT OR BUILD_QML_CLIENT)
	add_library(TalkTalkClientLib ${CLIENT_LIB_SRC})
	qt5_use_modules(TalkTalkClientLib Core Gui Network)
	set_package_properties(Qt5Gui PROPERTIES TYPE REQUIRED)
endif()

if(BUILD_WIDGETS_CLIENT)
	qt5_wrap_ui(WIDGETS_CLIENT_UI_SRC ${WIDGETS_CLIENT_UI})
	add_executable(TalkTalkClient ${WIDGETS_CLIENT_SRC} ${WIDGETS_CLIENT_UI_SRC})
	qt5_use_modules(TalkTalkClient Core Network Widgets)
	target_link_libraries(TalkTalkClient TalkTalkClientLib KF5::GuiAddons KF5::WidgetsAddons)
	set_package_properties(Qt5Widgets PROPERTIES TYPE REQUIRED)
	set_package_properties(KF5WidgetsAddons PROPERTIES TYPE REQUIRED)
	set_package_properties(KF5GuiAddons PROPERTIES TYPE REQUIRED)
endif()

feature_summary(FATAL_ON_MISSING_REQUIRED_PACKAGES WHAT ALL)
