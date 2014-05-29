#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QSerialPort>

struct Settings
{
    //Везна и Twido са на един порт по RS485
    QString comPort;    //Порт за Twido/везна
    int port;// ТСР порт
    int advance1;        //Предварение за спиране на пълненето
    int advance2;
    int advance3;
    int advance4;
    int advance5;
    int advance6;
    int advance7;
    int advance8;
    int advance9;
    int advance10;
    int advance11;

    int fine;           //Предварение в единици от везната (5 = 50 грама)
    int dumpHoldTime;   //Време за отлагане на дозирането в секунди
    int dumpTime;       //времe за разтоварване в секунди
    int twidoAddress;   //MODBUS адрес
    int holdTime;       //Време за успокояване на везната мс.
                        //Това е времето за пауза между дозирането на 2 съставки
};

#endif // SETTINGS_H
