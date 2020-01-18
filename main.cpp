#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
    QCoreApplication::setOrganizationName("Wojciech Cybowski");
    QCoreApplication::setOrganizationDomain("https://github.com/wcyb");
    QCoreApplication::setApplicationName("DuGu");
    QCoreApplication::setApplicationVersion("v1.1.0.0");
    MainWindow w;
    w.show();

    return a.exec();
}
