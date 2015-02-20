cmake_minimum_required(VERSION 2.8.9)

project(TalkTalk)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network)
find_package(Qt5 COMPONENTS WebSockets)
find_package(LibCommuni)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(${CMAKE_SOURCE_DIR})

set(CORE_SRC
	common/Json.h
	common/Json.cpp
	common/FileSystem.h
	common/FileSystem.cpp
	common/Exception.h
	common/TcpUtils.h
	common/TcpUtils.cpp

	core/main.cpp
	core/ConnectionManager.h
	core/ConnectionManager.cpp
	core/AbstractClientConnection.h
	core/AbstractClientConnection.cpp
	core/TcpServer.h
	core/TcpServer.cpp
	core/TcpClientConnection.h
	core/TcpClientConnection.cpp
	core/WebSocketServer.h
	core/WebSocketServer.cpp
	core/WebSocketClientConnection.h
	core/WebSocketClientConnection.cpp
)
set(CLIENT_SRC
	common/Json.h
	common/Json.cpp
	common/FileSystem.h
	common/FileSystem.cpp
	common/Exception.h
	common/TcpUtils.h
	common/TcpUtils.cpp

	client/main.cpp
	client/ServerConnection.h
	client/ServerConnection.cpp
	client/ChannelModel.h
	client/ChannelModel.cpp
	client/IrcServersModel.h
	client/IrcServersModel.cpp
	client/AbstractConsumer.h
	client/AbstractConsumer.cpp

	client/MainWindow.h
	client/MainWindow.cpp
)
set(CLIENT_UI
	client/MainWindow.ui
)

if(${Qt5WebSockets_FOUND})
	set(CORE_SRC ${CORE_SRC}
		core/WebSocketClientConnection.h
		core/WebSocketClientConnection.cpp
		core/WebSocketServer.h
		core/WebSocketServer.cpp
	)
	set(CORE_EXTRA_MODULES WebSockets)
	add_definitions(-DTALKTALK_CORE_WEBSOCKETS)
else()
	set(CORE_EXTRA_MODULES )
endif()
if(${LIBCOMMUNI_FOUND})
	set(CORE_SRC ${CORE_SRC}
		core/IrcClientConnection.h
		core/IrcClientConnection.cpp
		core/IrcMessageFormatter.h
		core/IrcMessageFormatter.cpp
	)
	set(CORE_EXTRA_LIBS IrcCore IrcModel IrcUtil)
	add_definitions(-DTALKTALK_CORE_IRC)
else()
	set(CORE_EXTRA_LIBS )
endif()

add_executable(TalkTalkCore ${CORE_SRC})
qt5_use_modules(TalkTalkCore Core Network ${CORE_EXTRA_MODULES})
target_link_libraries(TalkTalkCore ${CORE_EXTRA_LIBS})
include_directories(/opt/qt54/include/QtWebSockets)

qt5_wrap_ui(CLIENT_UI_SRC ${CLIENT_UI})
add_executable(TalkTalkClient ${CLIENT_SRC} ${CLIENT_UI_SRC})
qt5_use_modules(TalkTalkClient Core Network Widgets)
include_directories(${CMAKE_BINARY_DIR})
