#ifndef SERIALREADER_H
#define SERIALREADER_H

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QSerialPort>
#include <QTime>

/*
 * В тестов режим нишката не се опитва да работи със серииния порт. weight не се променя,
 * в twido не може да се пише,  a при четене връща фиксиран резултат
 */
#include "definitions.h"  //TEST_MODE

#define TEST_REPLY_DELAY 50 //Време на забавяне на нишката преди да върне резултат

class SerialReader : public QObject
{
    Q_OBJECT
public:
    explicit SerialReader(QObject *parent = 0);

    void setupThread(QThread &rxThread);
    void setSerialPort(QString portName);
    void closeSerialPort();

    void prepareTwidoRead(uchar address, ushort start, ushort registers);
    void prepareTwidoWrite(uchar address, ushort start, ushort registers, QByteArray &values);

    bool scaleError;//Става true при неуспешно четене от везната
    bool twidoRdError, twidoWrError;
    int  weight;
    QByteArray twidoRxData;
private:
    QSerialPort sp;
    QThread *thr;

    uchar twidoAddr;
    ushort rd_start , wr_start;
    ushort rd_registers, wr_registers;
    QByteArray rd_values, wr_values;

    bool rdTwido, wrTwido;

    unsigned short GetCRC(QByteArray message);
    bool writeTwido(uchar address, ushort start, ushort registers, QByteArray &values);
    bool readTwido(uchar address, ushort start, ushort registers, QByteArray &values);
    bool checkTwidoResponseCRC(QByteArray response);
    void readScale();

#ifdef TEST_MODE
    QByteArray testTwidoResult;
#endif
signals:
    void ReadingDone();

public slots:
    void readSerialDevices();
};

#endif // SERIALREADER_H
