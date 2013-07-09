#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

    QApplication a(argc, argv);
    a.setApplicationName("TFA");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("Unlimited Web Works");
	a.setWindowIcon(QIcon(":/main.ico"));

    MainWindow w;
    w.show();
    
    return a.exec();
}
