#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QString>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //Първия параметър е името на exe файла. Следва база и етикет
    QString tag="";
    QString sBase;
    bool ok;
    int base=1000;

    qDebug() << "argc=" <<argc;
    if(argc)
    {
        for(int i=0 ; i!=argc ; i++) qDebug() << "argv[" << i << "]=" << argv[i];
    }

    if(argc==3)
    {
        sBase = QString(argv[1]);
        int b = sBase.toInt(&ok);
        QString t = QString(argv[2]);

        if(ok && (t.trimmed()!="") && (b>0))
        {
            tag=t;
            base=b;
        }
    }

    MainWindow w(NULL,tag,base);
    w.show();
    //w.showFullScreen();
    //w.showMaximized();

    return a.exec();
}
