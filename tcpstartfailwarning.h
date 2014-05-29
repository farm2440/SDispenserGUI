#ifndef TCPSTARTFAILWARNING_H
#define TCPSTARTFAILWARNING_H

#include <QThread>
#include <QMessageBox>

class TcpStartFailWarning : public QThread
{
    Q_OBJECT
public:
    explicit TcpStartFailWarning(QObject *parent = 0);

signals:

public slots:
    void run();
};

#endif // TCPSTARTFAILWARNING_H
