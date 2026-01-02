#include <QApplication>

#include "GUI/main_window.hpp"

constexpr char NAME[] = "Sampler file manager";

int main(int argc, char **argv)
{
	QCoreApplication::setOrganizationName(QString(NAME));
	QCoreApplication::setApplicationName(QString(NAME));

	QApplication app(argc, argv);

	main_window_t main_window;

	app.exec();

	return 0;
}
