#include "mainwindow.h"
#include <QApplication>

#include "config.h"

unsigned int Roi::nbRois = 0; // initialization
ViewMode MainWindow::viewmode = ViewMode::SELECT; // SELECT mode by default
bool MainWindow::anonymized = false;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(APPLICATION_NAME);
    a.setApplicationDisplayName(a.applicationName());
    a.setApplicationVersion(APPLICATION_VERSION);
    MainWindow w;
    w.setWindowTitle("Main window");
    w.show();

    return a.exec();
}
