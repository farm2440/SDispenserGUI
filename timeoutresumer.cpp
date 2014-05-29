#include "timeoutresumer.h"

TimeoutResumer::TimeoutResumer(QObject *parent) : QThread(parent)
{
}

void TimeoutResumer::run()
{
    QMessageBox msgBox;
    msgBox.setText(QString("ПРЕДУПРЕЖДЕНИЕ"));
    msgBox.setInformativeText(QString("  Дозирането е временно спряно поради настъпил таймаут. Моля проверете дали има материал в бункерите и натиснете бутона за продължение!"));
    msgBox.addButton(QString(" ПРОДЪЛЖИ "), QMessageBox::YesRole);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();
}
