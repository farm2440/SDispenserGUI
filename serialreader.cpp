#include "serialreader.h"

SerialReader::SerialReader(QObject *parent) :  QObject(parent)
{
    rdTwido=false;
    wrTwido=false;
}

void SerialReader::setupThread(QThread &rxThread)
{
    //Този обект се пуска в друга нишка без да наследява QThread
    connect(&rxThread,SIGNAL(started()),this,SLOT(readSerialDevices()));
    thr= &rxThread;
}

void SerialReader::setSerialPort(QString portName)
{
#ifdef TEST_MODE
    return;
#endif

    if(sp.isOpen()) sp.close();
    sp.setPortName(portName);
    sp.open(QIODevice::ReadWrite);
    sp.setBaudRate(QSerialPort::Baud9600);
    sp.setDataBits(QSerialPort::Data8);
    sp.setParity(QSerialPort::EvenParity);
    sp.setStopBits(QSerialPort::TwoStop);
    sp.setFlowControl(QSerialPort::NoFlowControl);
}

void SerialReader::closeSerialPort()
{
    if(sp.isOpen()) sp.close();
}

void SerialReader::readSerialDevices()
{
#ifdef TEST_MODE
    twidoRxData.resize(16);
    twidoRxData[3]=0;
    twidoRxData[4]=3;
    twidoRdError=false;
    twidoWrError=false;
    scaleError=false;
    thr->msleep(TEST_REPLY_DELAY);
    thr->exit();
    return;
#endif

    QMutex mutex;
    readScale();
    if(wrTwido) twidoWrError = !writeTwido(twidoAddr, wr_start, wr_registers, wr_values);
    if(rdTwido)
    {
        twidoRdError = !readTwido(twidoAddr,rd_start, rd_registers, rd_values);
        mutex.lock();
            twidoRxData.append(rd_values);
        mutex.unlock();
    }
    thr->exit();
}

bool SerialReader::writeTwido(uchar address, ushort start, ushort registers, QByteArray &values)
{//Във values се  връща отговора от twido
    wrTwido=false;
    //Function 16 - write to registers
        /*Изпраща се съобщение:
          address 1 + func 1 + start 2 + reg 2 + count 1 + values N + crc 2
          address е адреса на устройство в modbus. func=16 - писане.
          В registers броя регистри започвайки от start се записват стойностите от values.
          Трябва да се има предвид, че modbus регистрите са 16 битови така,че
          броя байтове във values трябва да е 2 пъти по-голям от registers

          */
        if(!sp.isOpen())
        {
            thr->msleep(100);
            return false;
        }; //порта не е отворен.
        if(values.count() != (registers * 2) ) { return false; }

        unsigned short crc;
        QByteArray message,response;
        response.resize(8);
        message.resize(9+values.count());
        message[0] =address;
        message[1] = 16;
        message[2] = (uchar)(start >> 8);
        message[3] = (uchar)start;
        message[4] = (uchar)(registers >> 8);
        message[5] = (uchar)registers;
        message[6] = (uchar) values.count();
        message.replace(7,values.count(),values);
        crc=GetCRC(message);
        message[message.count()-2] = (ushort) (crc & 0xFF);
        message[message.count()-1] = (ushort) ((crc>>8)&0xFF);

        //Send modbus message to Serial Port:
        sp.clear();
        sp.write(message);
        QString str;
//        qDebug("writeTwido():");
        for(int i=0 ; i!= message.count() ; i++)  str += (QString::number(message[i] & 0xff,16) + " ");
//        qDebug("PC->Twido:" + str.toAscii());

        //отговор
        QTime time;
        int n=0;
        time.restart();
        values.clear();
        do
        {
            n=values.count();
            values.append(sp.readAll());
            if(n!=values.count()) time.restart();//има приети данни. таймера се рестартира
            else
                if(time.elapsed()>150) break; //излиз
        }while(1);
        str="";
        for(int i=0 ; i!= values.count() ; i++)  str += (QString::number(values[i]&0xff,16) + " ");

//        qDebug() << "writeTwido response: " << str;
//TODO: Освен проверка на CRC да се проверява и дали TWIDO не е върнало грешка
        bool result = checkTwidoResponseCRC(values);
//        if(result) qDebug() << "response CRC check: OK";
//        else qDebug() << "response CRC check: FAIL";
        return result;
}

bool SerialReader::readTwido(uchar address, ushort start, ushort registers, QByteArray &values)
{//Function 3 - Read Registers
    //Изпраща се запитване за стойност на регистрите в устройство  address.
    //Броят е посоченв registers а номера на началния регистър е start
    //Резултатът се връща във values
//    qDebug("readTwido()");
    rdTwido=false;
    if(!sp.isOpen())
    {
        //qDebug("ERR: Serial port not open");
        thr->msleep(100);
        return false; //порта не е отворен.
    }

    unsigned short crc;
    QByteArray message;
    //Function 3 request is always 8 bytes:
    message.resize(8);
    //Function 3 response buffer:
    values.resize(5+2*registers);
    //Build outgoing modbus message:
    message[0] =address;
    message[1] = 3;
    message[2] = (uchar)(start >> 8);
    message[3] = (uchar)start;
    message[4] = (uchar)(registers >> 8);
    message[5] = (uchar)registers;
    crc=GetCRC(message);
    message[message.count()-2] = (ushort) (crc & 0xFF);
    message[message.count()-1] = (ushort) ((crc>>8)&0xFF);
    //Send modbus message to Serial Port:
    sp.clear();
    sp.write(message);

//    qDebug("readTwido:");
    QString str;
    for(int i=0 ; i!= message.count() ; i++)  str += (QString::number((uchar)message[i]&0xff,16) + " ");
//    qDebug("PC->Twido:" + str.toAscii());

    //отговор
    QTime time;
    int n=0;
    values.clear();
    time.restart();
    do
    {
        n=values.count();
        values.append(sp.readAll());
        if(n!=values.count()) time.restart();//има приети данни. таймера се рестартира
        else
            if(time.elapsed()>150) break; //излиза
    }while(1);
    str="";
    for(int i=0 ; i!= values.count() ; i++)  str += (QString::number((uchar)values[i]&0xff,16) + " ");
//    qDebug("Twido->PC:" + str.toAscii());

    if(values.count()<7) return false;
    return checkTwidoResponseCRC(values);
}

void SerialReader::prepareTwidoWrite(uchar address, ushort start, ushort registers, QByteArray &values)
{
    twidoAddr = address;
    wr_start = start;
    wr_registers = registers;
    wr_values.resize(0);
    wr_values.append(values);
    wrTwido=true;
}

void SerialReader::prepareTwidoRead(uchar address, ushort start, ushort registers)
{
    QMutex mutex;
    mutex.lock();
        twidoRxData.resize(0);
    mutex.unlock();
    twidoAddr = address;
    rd_start = start;
    rd_registers = registers;
    rdTwido = true;
}

unsigned short SerialReader::GetCRC(QByteArray message)
{
    //Function expects a modbus message of any length
    // 2 byte CRC returned src-h crc-l
    ushort CRCFull=0xFFFF;
    uchar CRCLSB;

    for(int i = 0 ; i<message.count() - 2 ; i++)
    {
        CRCFull = (ushort)(CRCFull ^ message[i]);

        for(int j=0 ; j<8 ; j++)
        {
            CRCLSB = (uchar)(CRCFull & 0x0001);
            CRCFull = (ushort)((CRCFull>>1) & 0x7FFF);
            if(CRCLSB==1) CRCFull = (ushort)(CRCFull ^ 0xA001);
        }
    }
    return CRCFull;
}

bool SerialReader::checkTwidoResponseCRC(QByteArray response)
{//Тази функция проверява CRC на отговора на readTwido;
    ushort crc, rcrc;
    crc = GetCRC(response);

    int h=response[response.count()-1] & 0xff;
    int l=response[response.count()-2] & 0xff;

    rcrc = (ushort) ((h*256)+l);
//    qDebug("CRC in responce is " + QString::number(rcrc, 16).toAscii() +
//           " should be " + QString::number(crc,16).toAscii());

    return (crc==rcrc);
}

void SerialReader::readScale()
{
//    qDebug("readScale()");
    char reqData[] = { 2 , '0', '3', '0', '0', '0', '0', '1',
                      '0', '1', 'C', '0', '0', '0', '0', '2',
                      '0', '0', '0', '0', '0', '1',  3 , '@'};
    QTime time;
    QByteArray respData;
    bool ok;
    int retry=4; //брой опити за четене от везната
    bool err=false;
    QMutex mutex;

    if(!sp.isOpen())
    {
        mutex.lock();
            weight=0;
            scaleError=true;
        mutex.unlock();
        thr->msleep(100);
        return;
    }
    do
    {
        //запитване
        respData.clear();
        sp.clear();
        sp.write(reqData,24);

        time.restart();
        //отговор
        do
        {
            if(sp.bytesAvailable())
            {
                respData.append(sp.readAll());
                time.restart();//има приети данни. таймера се рестартира
            }

            if(time.elapsed()>150) break; //излиз
            thr->msleep(5);
        }while(1);

        //Отговора трябва да е 25 байта. запо4ва с 0х02, предпоследния е 0х03 и последния е контролна сума
        if(respData.count()!=25) ok=false;
        else
        {
            if(((int)respData[0]==2) && ((int)respData[23]==3)) ok=true;
            else ok=false;
        }

        //проверка контролна сума
        int cs = respData[1];
        if(ok)
        {
            for(int i=2 ; i!=24 ; i++) cs^=respData[i];
//            qDebug("check sum=0x" + QString::number(cs,16).toAscii());
            if(cs!=(int)respData[24]) ok=false;
        }

        //проверка краен брой опити за четене
        retry--;
        if(retry==0)
        {
            err=true;
            break;
        }

    }while(!ok);

//    qDebug("scale:" + QString(respData).toAscii());
    if(err)
    {
        //qDebug("ERR: Scale communication fail.");
        mutex.lock();
            weight=0;
            scaleError=true;
        mutex.unlock();
        return;
    }
    else
    {
        QString strw(respData.mid(17,6));
//        qDebug("strw=" +strw.toAscii());
        int w = strw.toInt(&ok,16);
        if(respData.mid(15,2) == "FF") //числото е отрицателно
        {
            w = (0xFFFFFF-w+1) * -1;
        }
//        qDebug("weight=" + QString::number(weight).toAscii());
        mutex.lock();
            weight=w;
            scaleError=false;
        mutex.unlock();
        return;
    }
}
