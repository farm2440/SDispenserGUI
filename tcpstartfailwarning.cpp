#include "tcpstartfailwarning.h"

TcpStartFailWarning::TcpStartFailWarning(QObject *parent) : QThread(parent)
{
}

void TcpStartFailWarning::run()
{
    QMessageBox msgBox;
    msgBox.setText(QString("ГЕШКА"));
    msgBox.setInformativeText(QString("Не може да започне ново дозиране на медикаменти преди да е завършено текущото напълно!"));
    msgBox.addButton(QString(" ПРОДЪЛЖИ "), QMessageBox::YesRole);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();
}
