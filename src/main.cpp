#include <QApplication>

#if _WIN32
	#include <QStyleFactory>
#endif

#include "GUI/main_window.hpp"

constexpr char NAME[] = "Sampler file manager";

int main(int argc, char **argv)
{
	#if _WIN32
		/*On Windows, the default "windowsvista" style doesn't support dark mode
		(even though the old "windows" style does), so default to Fusion.*/
		QApplication::setStyle(QStyleFactory::create("Fusion"));
	#endif

	QCoreApplication::setOrganizationName(QString(NAME));
	QCoreApplication::setApplicationName(QString(NAME));

	QApplication app(argc, argv);

	main_window_t main_window;

	app.exec();

	return 0;
}
