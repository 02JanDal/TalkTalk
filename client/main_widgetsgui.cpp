#include <QApplication>

#include "widgetsgui/MainWindow.h"

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	app.setOrganizationName("Jan Dalheimer");
	app.setApplicationName("TalkTalk Client");

	MainWindow window;
	window.show();

	return app.exec();
}
