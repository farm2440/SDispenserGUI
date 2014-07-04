#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QTime>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSql>
#include <QThread>
#include <QPalette>
#include <QDir>
#include <QUrl>
#include <QListWidgetItem>
#include <QCloseEvent>
#include <QAction>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QDomNodeList>

#include "iniparser.h"
#include "dictionary.h"

#include "definitions.h"
#include "dialogrecepies.h"
#include "dialogeditingradients.h"
#include "dialognewrecepie.h"
#include "Settings.h"
#include "serialreader.h"
#include "timeoutresumer.h"
#include "tcpstartfailwarning.h"
#include "dialogabout.h"

#include "dialogqueryrcps.h"
#include "dialogqueryingradients.h"
#include "dialogqueryexpenses.h"
#include "dialogquerycompletercps.h"

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0, QString tag="", int base=1000);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

private:
    enum State { IDLE,
                 TANK_SELECT,
                 WAIT_FINE_REACHED,
                 WAIT_WEIGHT_REACHED,
                 INTERDOSE_PAUSE,
                 DUMP_HOLD,
                 DUMP,
                 PAUSE,
                 HOLD,
                 RESUME,
                 FINISHED,
                 ABORT,
                 ABORTING
               } state, pausedState;

    enum CommState { COMM_IDLE, COMM_WAITING, COMM_DATA_READY } commState;

    QSqlDatabase database;
    QTimer timer;
    QThread txrxThread;
    SerialReader reader;
    Settings settings;

    void refreshRecepiesView();
    void saveLogData(); //При завършване на дозирането с тази функция става записа в БД

    int currentRecepieId;
    QString currentRecepieName;
    int medIds[TANKS_NUM]; //Тук при старт на дозиране се записват Id на съставките за да се ползват
                           //за запис в БД при завършване на цикъла
    QString medNames[TANKS_NUM];
    int targetDoses[TANKS_NUM]; // тук се записва зададените количества по рецепта
    int finalDoses[TANKS_NUM]; //Тук влизат реално разходените при дозирането количества
    bool flgTimeout, flgAir, flgInverter, flgValve; //Флаговете се четат от TWIDO
    int currentTank; //Номер на текущо дозирания бункер
    int totalWeight; //Сума на теглото на дозираните до момента съставки.
                    //От текущото показание на везната се вади това число за да се получи
                    //текущото тегло на дозираната в момента съставка.
    int finishStatusCode; //0 - нормално завършено дозиране
                          //1 - предварително прекратена от оператор
                          //2 - предварително прекратена поради тех. проблем
    double _base;    //Теглата на съставките в рецептите са на база 1000кг фураж.
                    //Aко base е 1000 дозирането е с теглата както са в рецептата. Ако обаче
                    // base е различно от 1000 теглата по рецептата се коригират при запис в targetDoses.
    QString _tag;   //Етикет на рецепта
    int advance;
    int twidoW10_H, twidoW10_L; //Съхранение на последни записани в twido стойности.
                                //Ползват се за възстановяване след пауза.
    QTime interDoseTimer; //Таймер за определяне времето на пауза между дозирането на 2 съставки
    QTime dumpTimer,dumpHoldTimer;
    QTime delayedStartTimer;
    int comErrCounterScale , comErrCounterTwido;

    QString rawHTML;
    void updateWebView();
    void changeCurrentRecepie(QString rcp);

    QString noRecepieErrMsg; //Съобщение за грешка, което да се вкара в html-а когато по TCP
                             //дойде заявка за неконфигурирана рецепта.

    TimeoutResumer *toResumer;
    QAction *aboutDialogAction;

    //Програмата може да започне да изпълнява рецепта зададена по етикет при стартирането:
    // >sdispenser 1200 tag1
    //или по команда получена по TCP връзка
    bool selectRecepieByTag(QString tag);
    QTcpServer srv;
    QTcpSocket *sock;
    TcpStartFailWarning tcpFailWarning;

private slots:
    void onTimerTick();
    void onReadingDone();
    void on_actionEditRecepies_triggered();
    void on_actionEditIngradients_triggered();
    void on_btnStart_clicked();
    void on_btnAbort_clicked();
    void resumeFromTimeout();
    void on_listRecepies_currentRowChanged(int currentRow);

    void on_action_QueryExpenses_triggered();
    void on_action_QueryCompleteRcps_triggered();
    void on_action_QueryIngradients_triggered();
    void on_action_QueryRcps_triggered();

    void on_actionAbout_triggered();

    void onNewConnection();
    void readTcpData();
    void onSocketDisconnected();

public slots:
    void closeEvent(QCloseEvent *event);
};

#endif // MAINWINDOW_H
