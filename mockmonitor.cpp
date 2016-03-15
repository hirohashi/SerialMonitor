#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

    MainWindow *main_window = new MainWindow();
	main_window->show();
    int rc = app.exec();

    delete main_window;
    return rc;
}
