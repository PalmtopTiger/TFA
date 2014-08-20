#include "mainwindow.h"
#include <QApplication>

#if QT_VERSION < 0x050000
#include <QTextCodec>
#endif

int main(int argc, char *argv[])
{
#if QT_VERSION < 0x050000
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
#endif

    QApplication a(argc, argv);
    a.setApplicationName("TFA");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("Unlimited Web Works");
	a.setWindowIcon(QIcon(":/main.ico"));

    MainWindow w;
    w.show();
    
    return a.exec();
}
