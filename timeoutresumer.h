#ifndef TIMEOUTRESUMER_H
#define TIMEOUTRESUMER_H

#include <QThread>
#include <QMessageBox>
#include <QDebug>

class TimeoutResumer : public QThread
{
    Q_OBJECT
public:
    explicit TimeoutResumer(QObject *parent = 0);

signals:

public slots:

    void run();

};

#endif // TIMEOUTRESUMER_H
