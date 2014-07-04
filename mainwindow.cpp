#include "mainwindow.h"
#include "ui_mainwindow.h"

//ncat 127.0.0.1 9000

MainWindow::MainWindow(QWidget *parent, QString tag, int base) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    state=IDLE;
    ui->setupUi(this);

    //отваряне на базата данни
    QFile dbFile("sdispenser.db");
    if(!dbFile.exists()) QMessageBox::critical(this, tr("Грешка база данни"), tr("Файлът с БД не съществува"));
    else
    {   database = QSqlDatabase::addDatabase("QSQLITE"); //QSQLITE е за версия 3 и нагоре, QSQLITE2 e за версия 2
        database.setDatabaseName("sdispenser.db");
        if(!database.open()) QMessageBox::critical(this, tr("Грешка база данни"), tr("Не мога да отворя БД"));

        this->close();
    }
    //Зареждане на html фона
    QFile htmlFile("background.html");
    if(htmlFile.exists())
    {
        htmlFile.open(QIODevice::ReadOnly);
        rawHTML = QString( htmlFile.readAll() );
        htmlFile.close();
    }
    else qDebug() << "failed to load html background";
//    qDebug() << "rawHtml=" <<rawHTML;
    updateWebView();

    //зареждане на настройки от ini файл
    QFile iniFile("settings.ini");
    if(!iniFile.exists())
    {
        //Файла не съществува. Създава се нов и в него се записват настройки по подразбиране
        qDebug("ERR: failed to open settings.ini. Creating new with default values.");
        iniFile.open(QIODevice::WriteOnly);
        QTextStream s(&iniFile);
        s<< "[SETTINGS]\n";
        s<< "TwidoAddr=11\n";
        s<< "COM=\n";
        s<< "Port=9000\n";
        s<< "Fine=0\n";

        s<< "Advance1=1\n";
        s<< "Advance2=1\n";
        s<< "Advance3=1\n";
        s<< "Advance4=1\n";
        s<< "Advance5=1\n";
        s<< "Advance6=1\n";
        s<< "Advance7=1\n";
        s<< "Advance8=1\n";
        s<< "Advance9=1\n";
        s<< "Advance10=1\n";
        s<< "Advance11=1\n";

        s<< "DumpHoldTime=30\n";
        s<< "DumpTime=5\n";
        s<< "HoldTime=1500\n";
        s<< "DelayedStart=10\n";
        iniFile.close();
    }

    for(int i=0 ; i!=TANKS_NUM ; i++)
    {
        targetDoses[i]=0;
        finalDoses[i]=0;
    }

    char * fn = new char[40];
    strcpy(fn,"settings.ini");
    dictionary *dict;
    dict = iniparser_load(fn);
    if(dict == NULL)   qDebug("ERR: Failed to load dictionary from settings.ini");
    settings.advance1 = iniparser_getint(dict,QString("SETTINGS:Advance1").toLatin1().data(),ADVANCE_DEFAULT_VALUE);
    settings.advance2 = iniparser_getint(dict,QString("SETTINGS:Advance2").toLatin1().data(),ADVANCE_DEFAULT_VALUE);
    settings.advance3 = iniparser_getint(dict,QString("SETTINGS:Advance3").toLatin1().data(),ADVANCE_DEFAULT_VALUE);
    settings.advance4 = iniparser_getint(dict,QString("SETTINGS:Advance4").toLatin1().data(),ADVANCE_DEFAULT_VALUE);
    settings.advance5 = iniparser_getint(dict,QString("SETTINGS:Advance5").toLatin1().data(),ADVANCE_DEFAULT_VALUE);
    settings.advance6 = iniparser_getint(dict,QString("SETTINGS:Advance6").toLatin1().data(),ADVANCE_DEFAULT_VALUE);
    settings.advance7 = iniparser_getint(dict,QString("SETTINGS:Advance7").toLatin1().data(),ADVANCE_DEFAULT_VALUE);
    settings.advance8 = iniparser_getint(dict,QString("SETTINGS:Advance8").toLatin1().data(),ADVANCE_DEFAULT_VALUE);
    settings.advance9 = iniparser_getint(dict,QString("SETTINGS:Advance9").toLatin1().data(),ADVANCE_DEFAULT_VALUE);
    settings.advance10 = iniparser_getint(dict,QString("SETTINGS:Advance10").toLatin1().data(),ADVANCE_DEFAULT_VALUE);
    settings.advance11 = iniparser_getint(dict,QString("SETTINGS:Advance11").toLatin1().data(),ADVANCE_DEFAULT_VALUE);

    settings.fine = iniparser_getint(dict,QString("SETTINGS:Fine").toLatin1().data(),0);
    settings.dumpHoldTime = iniparser_getint(dict,QString("SETTINGS:DumpHoldTime").toLatin1().data(),30);
    settings.dumpTime = iniparser_getint(dict,QString("SETTINGS:DumpTime").toLatin1().data(),5);
    settings.twidoAddress = iniparser_getint(dict,QString("SETTINGS:TwidoAddr").toLatin1().data(),11);
    settings.holdTime = iniparser_getint(dict,QString("SETTINGS:HoldTime").toLatin1().data(),1500);
    settings.comPort = QString( iniparser_getstring(dict, QString("SETTINGS:COM").toLatin1().data(), QString("").toLatin1().data()) );
    settings.port = iniparser_getint(dict, QString("SETTINGS:Port").toLatin1().data(), 9000);
    settings.delayedStart = iniparser_getint(dict, QString("SETTINGS:DelayedStart").toLatin1().data(), 10);

    delete dict;
    delete fn;

    refreshRecepiesView();

    state = IDLE;
    qDebug() << "tag=" << tag << " base=" << base;
    currentRecepieId=-1;
    ui->listRecepies->setCurrentRow(-1);
    currentTank=0;
    currentRecepieName="НЯМА ИЗБРАНА РЕЦЕПТА!";

    if(settings.dumpTime<3) settings.dumpTime=3;
    settings.dumpTime = settings.dumpTime+1;

    //Подготвяне на нишката за връзка по сериен порт.
    commState = COMM_IDLE;
    reader.setSerialPort(settings.comPort);
    reader.setupThread(txrxThread);
    reader.moveToThread(&txrxThread);
    connect(&txrxThread, SIGNAL(finished()), this, SLOT(onReadingDone()));
    comErrCounterScale = COM_ERR_THRESHOLD;
    comErrCounterTwido = COM_ERR_THRESHOLD;

    //Отваряне на TCP порта за слушане
    sock=NULL;
    srv.listen(QHostAddress::LocalHost, settings.port);
    connect(&srv, SIGNAL(newConnection()), this, SLOT(onNewConnection()));

    //в ТWIDO се записва 0 - няма дозиране, разтоварващия клапан е затворен
    QByteArray wval;
    wval.resize(2);
    wval[0]= 0;
    wval[1]= 0;
    reader.prepareTwidoWrite(settings.twidoAddress,10,1,wval);

    toResumer = new TimeoutResumer(this);
    connect(toResumer, SIGNAL(finished()), this, SLOT(resumeFromTimeout()));

    //Старт на работния цикъл
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimerTick()));
    timer.setSingleShot(true);
    timer.start(500);

    //Проверка дали програмата е стартирана от Delphi с етикет
    _base = base;
    _tag = tag;
    noRecepieErrMsg="";

    if(tag!="")
    {
        if(selectRecepieByTag(_tag)) on_btnStart_clicked();
        else
        {
//            QString msg = "Дозирането на медикаменти\nне може да бъде извършено!\nНяма конфигурирана рецепта с етикет\n" + _tag;
//            QMessageBox::critical(this,"ГРЕШКА!", msg);
            noRecepieErrMsg = "<div style=\"background-color:#FAFAFA; color:#FF0000; padding-left:20; padding-right:20;\" > <h3>";
            noRecepieErrMsg+="Дозирането на медикаменти не може да бъде извършено!<br>Няма конфигурирана рецепта с етикет" + _tag;
            noRecepieErrMsg+= " </h3></div>";
        }
    }

    updateWebView();

}

MainWindow::~MainWindow()
{
    if(sock!= NULL)
    {
        sock->close();
        delete sock;
    }
    srv.close();

    QByteArray wval;
    //При излизане в TWIDO се записва 0 за сигурност.
    state = IDLE;
    if(txrxThread.isRunning()) txrxThread.wait(1500);
    wval.resize(2);
    wval[0]= 0;
    wval[1]= 0;
    reader.prepareTwidoWrite(settings.twidoAddress,10,1,wval);
    txrxThread.start();
    txrxThread.wait(1500);

    toResumer->wait(100);
    delete toResumer;

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug() <<"state=" << state << " IDLE=" <<IDLE;
    if((state==IDLE) || (state==FINISHED)) event->accept();
    else
    {
        QMessageBox::information(this, "ПРЕДУПРЕЖДЕНИЕ", "Дозирането не е приключило.\nПрограмата не може да бъде спряна.");
        event->ignore();
    }
}

void MainWindow::onTimerTick()
{
    /* За връзка по сериен порт се ползва обекта reader изнесен в нишката txrxThread.
     * Комуникацията може да е в 3 състояния: COMM_IDLE, COMM_WAITING и COMM_DATA_READY.
     * Нишката се стартира пр COMM_IDLE и в COMM_DATA_READY  според приетете данни се
     * управлява работния цикъл на програмата. Текущото състояние е в state.
     */
    QPalette pal;
    QByteArray rval,wval;

    switch(commState)
    {
    case COMM_IDLE:
        //четене Twido. Везната се прочита винаги.
        //Писането в Twido aко е необходимо се подготвя при COMM_DATA_READY.
        reader.prepareTwidoRead(settings.twidoAddress,0,1);
        txrxThread.start();
        commState = COMM_WAITING;
        break;
    case COMM_WAITING:
        break;
    case COMM_DATA_READY:
        //1.Проверка на данните от везната
        pal = ui->lblScaleStatus->palette();
        if(reader.scaleError)
        {
            if(comErrCounterScale) comErrCounterScale--;
            else
            {
                ui->lblScaleStatus->setText("  НЯМА ВРЪЗКА С ВЕЗНАТА!  ");
                pal.setColor(QPalette::Background, Qt::red);
            }
        }
        else
        {
            comErrCounterScale = COM_ERR_THRESHOLD;
            ui->lblScaleStatus->setText("");
            pal.setColor(QPalette::Background, Qt::green);
            QString w;
            w =w.sprintf("%3.2f",(double)reader.weight / 100.0);
            ui->lcdScale->display(w);
        }
        ui->lblScaleStatus->setPalette(pal);

        //2.Проверка на флаговете прочетени от TWIDO
        if(reader.twidoWrError)
        {
            qDebug() << "ERR: Failed writing to TWIDO! Write repeat!";
            //Повтаря се последния запис правен в TWIDO:
//            wval.resize(2);
//            wval[0] = twidoW10_H;
//            wval[1]= twidoW10_L;
//            reader.prepareTwidoWrite(settings.twidoAddress,10,1,wval);
        }
        if(reader.twidoRdError)
        {
            if(comErrCounterTwido) comErrCounterTwido--;
            else
            {
                ui->lblAir->setEnabled(false);
                ui->lblInverter->setEnabled(false);
                ui->lblValve->setEnabled(false);
                pal = ui->lblPLC->palette();
                pal.setColor(QPalette::Background, Qt::red);
                ui->lblPLC->setPalette(pal);
            }
        }
        else
        {
            comErrCounterTwido = COM_ERR_THRESHOLD;
            ui->lblAir->setEnabled(true);
            ui->lblInverter->setEnabled(true);
            ui->lblValve->setEnabled(true);
            pal = ui->lblPLC->palette();
            pal.setColor(QPalette::Background, Qt::green);
            ui->lblPLC->setPalette(pal);
            //Извличане на флаговете
            rval = reader.twidoRxData;
            //MW0.0 Флаг въздух
            flgAir = ((rval[4] & 0x01)!= 0);
            //MW0.1 Флаг грешка инвертор
            flgInverter = ((rval[4] & 0x02) != 0);
            //MW0.13 Флаг таймаут на twido (За да се изчисти тази грешка трябва да запиша 0 обратно);
            flgTimeout = ((rval[3] & 0x20) != 0);
            //MW0.2 състояние на шибъра
            flgValve = ((rval[4] & 0x04) != 0);

            if(flgTimeout && (!toResumer->isRunning()))
            {

                qDebug() << "flgTimeout=true";
                pausedState = state;
                state = PAUSE;
                toResumer->start();
            }

            //Оцветяване на етикетите според флаговете
            pal = ui->lblAir->palette();
            if(flgAir) pal.setColor(QPalette::Background, Qt::green);
            else pal.setColor(QPalette::Background, Qt::red);
            ui->lblAir->setPalette(pal);

            pal = ui->lblInverter->palette();
            if(flgInverter) pal.setColor(QPalette::Background, Qt::red);
            else pal.setColor(QPalette::Background, Qt::green);
            ui->lblInverter->setPalette(pal);

            pal = ui->lblValve->palette();
            if(flgValve) pal.setColor(QPalette::Background, Qt::yellow);
            else pal.setColor(QPalette::Background, Qt::green);
            ui->lblValve->setPalette(pal);
        }

        //3. **** Работен цикъл ****
        switch(state)
        {
        case HOLD:
            break;
        case IDLE:
        case FINISHED:
                if(noRecepieErrMsg.length()!=0)
                {
                    updateWebView();
                    noRecepieErrMsg="";
                }
            break;
        case TANK_SELECT:
            if(delayedStartTimer.elapsed() > (settings.delayedStart * 1000))
            {

                //Номерът на текущия бункер се записва в TWIDO. От това състояние започва цикъла на дозирането
                wval.resize(2);
                wval[0]= 0;
                wval[1]= currentTank;
                twidoW10_H= 0;
                twidoW10_L= currentTank;
                reader.prepareTwidoWrite(settings.twidoAddress,10,1,wval);
                state = WAIT_FINE_REACHED;
                updateWebView();
                qDebug() << "currentTank=" << currentTank;
                qDebug() << "state = WAIT_FINE_REACHED;";
            }
            break;
        case WAIT_FINE_REACHED:
#ifdef TEST_MODE
            reader.weight++;
#endif
            if(reader.weight-totalWeight >= targetDoses[currentTank-1]-settings.fine)
            {
                wval.resize(2);
                wval[0] = 1;//флаг за фино в twido
                wval[1]= currentTank;
                twidoW10_H= 1;
                twidoW10_L= currentTank;
                reader.prepareTwidoWrite(settings.twidoAddress,10,1,wval);
                state = WAIT_WEIGHT_REACHED;
                updateWebView();
                qDebug() << "state = WAIT_WEIGHT_REACHED;";

                switch(currentTank)
                {
                case 1:
                    advance = settings.advance1;
                    break;
                case 2:
                    advance = settings.advance2;
                    break;
                case 3:
                    advance = settings.advance3;
                    break;
                case 4:
                    advance = settings.advance4;
                    break;
                case 5:
                    advance = settings.advance5;
                    break;
                case 6:
                    advance = settings.advance6;
                    break;
                case 7:
                    advance = settings.advance7;
                    break;
                case 8:
                    advance = settings.advance8;
                    break;
                case 9:
                    advance = settings.advance9;
                    break;
                case 10:
                    advance = settings.advance10;
                    break;
                case 11:
                    advance = settings.advance11;
                    break;
                default:
                    advance = ADVANCE_DEFAULT_VALUE;
                    break;
                }
                qDebug() << "Advance for tank " << currentTank << " is " << advance;
            }
            break;
        case WAIT_WEIGHT_REACHED:
#ifdef TEST_MODE
            reader.weight++;
#endif

            if((reader.weight-totalWeight) >= (targetDoses[currentTank-1] - advance))
            {//Проверява дали е достигната дозата за текущата съставка.
                qDebug() << "reader.weight=" << reader.weight << "  totalWeight=" << totalWeight;
                qDebug() << "targetDoses[currentTank-1]=" << targetDoses[currentTank-1] << " advance=" << advance;
                wval.resize(2);
                wval[0]=0;
                wval[1]=0;
                twidoW10_H= 0;
                twidoW10_L= 0;
                reader.prepareTwidoWrite(settings.twidoAddress,10,1,wval);
                interDoseTimer.restart();
                state = INTERDOSE_PAUSE;
                updateWebView();
                qDebug() << "state = INTERDOSE_PAUSE;";
            }
            break;
        case INTERDOSE_PAUSE:
            if(interDoseTimer.elapsed() > settings.holdTime)
            {
#ifdef TEST_MODE
            reader.weight++;
#endif
                finalDoses[currentTank-1] = reader.weight - totalWeight;
                totalWeight = reader.weight;
                //Определя се следващия бункер
                int i = currentTank - 1;
                do
                {
                  i++;
                  if(i==TANKS_NUM) //това е бил последния бункер за дозиране
                  {
                      dumpHoldTimer.restart();
                      qDebug() << "state = DUMP_HOLD;";
                      state = DUMP_HOLD;
                      updateWebView();
                      break;
                  }
                }while(targetDoses[i]==0);
                if(i==TANKS_NUM) break; //със state=DUMP_HOLD

                currentTank = i + 1;
//                delayedStartTimer.restart();
                state = TANK_SELECT;
                updateWebView();
                qDebug() << "state = TANK_SELECT;";
            }
            break;
        case DUMP_HOLD:
            if(dumpHoldTimer.elapsed() > (settings.dumpHoldTime * 1000))
            {
                dumpTimer.restart();
                wval.resize(2);
                wval[0]=0;
                wval[1]=100;
                reader.prepareTwidoWrite(settings.twidoAddress,10,1,wval);
                qDebug() << "state = DUMP;";
                state = DUMP;
                updateWebView();
            }
            break;
        case DUMP:
            if(dumpTimer.elapsed() > (settings.dumpTime * 1000))
            {
#ifdef TEST_MODE
            reader.weight=0;
#endif
                wval.resize(2);
                wval[0]=0;
                wval[1]=0;
                reader.prepareTwidoWrite(settings.twidoAddress,10,1,wval);

                ui->listRecepies->setEnabled(true);
                ui->actionEditIngradients->setEnabled(true);
                ui->actionEditRecepies->setEnabled(true);
                ui->action_Exit->setEnabled(true);
                ui->action_QueryCompleteRcps->setEnabled(true);
                ui->action_QueryExpenses->setEnabled(true);
                ui->action_QueryIngradients->setEnabled(true);
                ui->action_QueryRcps->setEnabled(true);
                ui->btnStart->setEnabled(true);
                ui->btnAbort->setEnabled(false);
                saveLogData();//Запис в БД на количествата от finalDoses
                qDebug() << "state = IDLE;";
                state = FINISHED;
                updateWebView();
            }
            break;
        case PAUSE:
            wval.resize(2);
            wval[0]=0;
            wval[1]=0;
            reader.prepareTwidoWrite(settings.twidoAddress,10,1,wval);
            qDebug() << "state = HOLD; (on pause)";
            state = HOLD;
            updateWebView();
            break;
        case RESUME:
            wval.resize(2);
            wval[0]= twidoW10_H;
            wval[1]= twidoW10_L;
            reader.prepareTwidoWrite(settings.twidoAddress,10,1,wval);
            qDebug() << "Resuming...";
            state = pausedState;
            updateWebView();
            break;
        case ABORT:
            wval.resize(2);
            wval[0]=0;
            wval[1]=0;
            reader.prepareTwidoWrite(settings.twidoAddress,10,1,wval);

            interDoseTimer.restart();
            state = ABORTING;
            updateWebView();
            qDebug() << "state = ABORTING";
            ui->btnAbort->setEnabled(false);
            break;
        case ABORTING:
            if(interDoseTimer.elapsed() > settings.holdTime)
            {
                finalDoses[currentTank-1] = reader.weight - totalWeight;
                currentTank = 0;

                dumpTimer.restart();
                wval.resize(2);
                wval[0]=0;
                wval[1]=100;
                reader.prepareTwidoWrite(settings.twidoAddress,10,1,wval);
                qDebug() << "state = DUMP;";
                state = DUMP;
                updateWebView();
            }

            break;
        }//Край на работния цикъл

        commState = COMM_IDLE;
        break;
    }

    timer.start(TICK_PERIOD);
}

void MainWindow::saveLogData()
{
    QDateTime dt = QDateTime::currentDateTime();
    QSqlQuery qry;
    QString sql = QString("INSERT INTO tblLog VALUES (NULL, '%1 %2', ")
                            .arg(dt.date().toString("yyyy-MM-dd"))
                            .arg(dt.time().toString("hh:mm:ss"));
    sql += (QString::number(currentRecepieId) + ", ");

    for(int i=0 ; i!=TANKS_NUM ; i++) sql += (QString::number(medIds[i]) +",");
    sql += " ";
    for(int i=0 ; i!=TANKS_NUM ; i++) sql += (QString::number(finalDoses[i])+ ",");
    sql += (" " + QString::number(finishStatusCode));
    sql +=");";
    qry.prepare(sql);

    qDebug() <<sql;
    if(!qry.exec()) qDebug() << "ERR: saveLogData() failed writing to tblLog";
}

void MainWindow::onReadingDone()
{
    commState = COMM_DATA_READY;
}

void MainWindow::on_actionEditRecepies_triggered()
{
    //Редактиране на рецептите от главното меню
    DialogRecepies dlg;
    dlg.exec();
    refreshRecepiesView();
}

void MainWindow::on_actionEditIngradients_triggered()
{
    DialogEditIngradients dlg;
    dlg.exec();
}

void MainWindow::refreshRecepiesView()
{
    QSqlQuery qry;
    qry.prepare("SELECT rName FROM tblRcps;");
    ui->listRecepies->clear();
    if(qry.exec())
    {
        while(qry.next())
        {
            ui->listRecepies->addItem( qry.value("rName").toString() );
        }
    }

}

void MainWindow::on_btnStart_clicked()
{
    noRecepieErrMsg="";
    if(state==FINISHED) state = IDLE;
    qDebug() << "state=IDLE";
    if(currentRecepieId == -1)
    {
        QMessageBox::warning(this,"ПРЕДУПРЕЖДЕНИЕ", "Няма избрана рецепта");
        return;
    }

#ifndef TEST_MODE
    if(flgValve)
    {
        QMessageBox::warning(this,"ПРЕДУПРЕЖДЕНИЕ", "Разтоварващия клапан е отворен.\nДозирането не може да започне.");
        return;
    }
#endif

    //Проверка дали всички съставки са налични
    QSqlQuery qry;
    QString sql;
    for(int i=0 ; i!= TANKS_NUM ; i++)
    {
        if(medIds[i]==1) continue;
        sql = "SELECT mName,Available FROM tblMeds WHERE mID=" + QString::number(medIds[i]) +";";
        qry.prepare(sql);
        if(qry.exec())
        {
            if(qry.next())
            {
                if(qry.value("Available").toInt()!=1)
                {
                    QMessageBox::critical(this,"ГРЕШКА",QString("Съставката \"%1\" не е налична.\nРецептата не може да бъеде изпълнена.").arg(qry.value("mName").toString()));
                    return;
                }
            }qDebug() << "ERR: failed with SQL SELECT 1";
        }else qDebug() << "ERR: failed with SQL SELECT 2";
    }

    ui->listRecepies->setEnabled(false);
    ui->actionEditIngradients->setEnabled(false);
    ui->actionEditRecepies->setEnabled(false);
    ui->action_Exit->setEnabled(false);
    ui->action_QueryCompleteRcps->setEnabled(false);
    ui->action_QueryExpenses->setEnabled(false);
    ui->action_QueryIngradients->setEnabled(false);
    ui->action_QueryRcps->setEnabled(false);
    ui->btnStart->setEnabled(false);
    ui->btnAbort->setEnabled(true);

#ifdef TEST_MODE
            reader.weight=0;
#endif

    qDebug() << "Дози по рецепта:";
    currentTank = 0;
    for(int i=0 ; i!= TANKS_NUM ; i++)
    {
        if((currentTank==0) && (targetDoses[i]!=0)) currentTank=i+1; //Първия бункер за дозиране
        if(targetDoses[i]) qDebug() << "tank " << i+1 << ":" << targetDoses[i];
        finalDoses[i]=0;
    }
    qDebug() << "Първия бункер за дозиране е " << currentTank;

    finishStatusCode = 0;
    totalWeight = reader.weight;
    delayedStartTimer.restart();
    state = TANK_SELECT;
    qDebug() << "state = TANK_SELECT;";
}

void MainWindow::on_btnAbort_clicked()
{
    noRecepieErrMsg="";
    pausedState = state;
    state = PAUSE;
    qDebug() << "state = PAUSE;";

    QMessageBox msgBox;
    msgBox.setText(QString("ПРЕДУПРЕЖДЕНИЕ"));
    msgBox.setInformativeText(QString("Дозирането е временно спряно.\nЖелаете ли да го прекратите?"));
    QAbstractButton *btnAbort = msgBox.addButton(QString(" ПРЕКРАТИ "), QMessageBox::YesRole);
    msgBox.addButton(QString(" ПРОДЪЛЖИ "), QMessageBox::NoRole);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();

    if(msgBox.clickedButton() == btnAbort)
    {        
        state = ABORT;
        updateWebView();
        finishStatusCode=1;
        qDebug() << "state = ABORT;";
    }
    else
    {
        state = RESUME;
        updateWebView();
        qDebug() << "state = RESUME;";
    }
}

void MainWindow::updateWebView()
{
    QString str = qApp->applicationDirPath();
    //qDebug() <<"url=" << str;
    QUrl url = QUrl::fromLocalFile(qApp->applicationDirPath());

    //Цветове
    QString clrRowFinished = "cdcdcd";
    QString clrRowDosing= "00FF00";
    QString clrRowWaiting= "FFFFFF";

    QString tableRow = "<tr bgcolor=\"#~rowColor~\"> <td> ~tank~</td> <td> ~mName~</td> <td> ~target~</td> <td> ~final~ &nbsp;&nbsp; ~img~ </td></tr>";

    QString html = rawHTML;
    html.replace("~currentRecepie~", currentRecepieName);
    html.replace("~noRecepieErrMsg~", noRecepieErrMsg);

    //ТЕкущо състояние
    switch(state)
    {
    case IDLE:
        html.replace("~state~", "ГОТОВ ЗА ДОЗИРАНЕ");
        break;
    case DUMP:
        html.replace("~state~","РАЗТОВАРВАНЕ");
    case DUMP_HOLD:
        html.replace("~state~","ОТЛОЖЕНО РАЗТОВАРВАНЕ - ИЗЧАКВАНЕ");
    case PAUSE:
    case HOLD:
        html.replace("~state~", "НА ПАУЗА");
        break;
    case ABORT:
    case ABORTING:
        html.replace("~state~", "ПРИНУДИТЕЛНО СПИРАНЕ");
        break;
    case FINISHED:
        html.replace("~state~", "ДОЗИРАНЕТО Е ЗАВЪРШЕНО");
        break;
    default:
        html.replace("~state~", "ДОЗИРАНЕ");
    }

    //Редове в таблицата
    QString row;
    for(int i=0; i!=TANKS_NUM ; i++)
    {
        row = tableRow;
        //Ако съставката не е включена в рецептата или ако няма избрана рецепта редовете са празни.
        if((medIds[i]==1) || (currentRecepieId==-1))
        {
            row="";
        }
        else
        {
            //ЦВЯТ НА РЕДА

            switch (state)
            {
            case IDLE:
                row.replace("~rowColor~",clrRowWaiting);
                row.replace("~img~", "");
                break;
            case FINISHED:
                row.replace("~rowColor~",clrRowFinished);
                row.replace("~img~", "");
                break;
            case DUMP:
                row.replace("~rowColor~",clrRowFinished);
                row.replace("~img~", "<img height=\"24\" src=\"qrc:files/res/truck-dumps.gif\">");
                break;
            case DUMP_HOLD:
                row.replace("~rowColor~",clrRowFinished);
                row.replace("~img~", "<img height=\"24\" src=\"qrc:files/res/hourglass.gif\">");
                break;
            case ABORT:
            case ABORTING:
                row.replace("~img~", ""); //НЯМА break!!
            default:
                if(currentTank==i+1)
                {
                    //row.replace("~img~", "<img src=\"animWheel24.gif\">");
                    row.replace("~img~", "<img height=\"24\" src=\"qrc:files/files2workDir/animWheel24.gif\">");
                    row.replace("~rowColor~",clrRowDosing);
                }
                else
                {
                    row.replace("~img~", "");
                    if(currentTank<(i+1)) row.replace("~rowColor~",clrRowWaiting);
                    else row.replace("~rowColor~",clrRowFinished);
                }
            }
            //СЪДЪРЖАНИЕ НА РЕДА
            row.replace("~tank~", QString::number(i+1));
            row.replace("~mName~",medNames[i]);
            row.replace("~target~",QString::number((double)targetDoses[i]/100.0));
            row.replace("~final~",QString::number((double)finalDoses[i]/100.0));
        }
        html.replace(QString("~row%1~").arg(i+1), row);
    }
    ui->webView->setHtml(html, url);
}

void MainWindow::on_listRecepies_currentRowChanged(int currentRow)
{    
    /* Запазват се
     *  currentRecepieName - име на рецепта
     *  targetDoses[] - за съставките в рецептата теглото коригирани ако base е различно от 1000
     *  medIds[] - id на съставките в таблицата tblMeds
     *
     *  нулира се finalDoses[]
     */

    if(currentRow==-1)
    {
        currentRecepieId=-1;
        currentTank=0;
        currentRecepieName="НЯМА ИЗБРАНА РЕЦЕПТА!";
        for(int i=0; i!=TANKS_NUM ; i++)
        {
            medIds[i]=1;
            targetDoses[i]=0;
            medNames[i]="none";
        }
        updateWebView();
        return;
    }

    currentRecepieName = ui->listRecepies->item(currentRow)->text().trimmed();
    qDebug() << "currentRecepieName=" <<currentRecepieName;

    if(state==FINISHED) state=IDLE;

    // 2. Проверява се дали избраната рецепта има всички съставки налични
    //  и се запълват в doses според номера на бункера;

    QSqlQuery qry, qry2;
    bool ok;
    bool fail = false;
    for(int i=0 ; i!=TANKS_NUM ; i++)
    {
        targetDoses[i]=0;
        finalDoses[i]=0;
        medIds[i]=1;
    }
    QString sql = "SELECT * FROM tblRcps WHERE rName='" + currentRecepieName + "';";
    qry.prepare(sql);
    if(qry.exec())
    {
        if(qry.next())
        {
            currentRecepieId = qry.value("rId").toInt();
            //Зареждат се количествата на съставките
            for(int i = 0 ; i!= TANKS_NUM ; i++)
            {
                QString kg = qry.value(QString("med%1").arg(i+1)).toString();
                QString id = qry.value(QString("med%1_Id").arg(i+1)).toString();

                if(id=="1") continue;//с id=1 са позиции на съставки които не са включени в рецептата.

                sql = "SELECT Tank,mName,Available FROM tblMeds WHERE mID=" + id +";";
                qry2.prepare(sql);
                if(qry2.exec())
                {
                     if(qry2.next())
                     {
                         int tank = qry2.value("Tank").toInt(&ok);
                         if(ok && (tank>0) && (tank<=TANKS_NUM))
                         {
                             if(_base==1000)  targetDoses[tank-1] = kg.toInt();
                             else targetDoses[tank-1] = (int) (kg.toDouble() * _base/1000);
                             medNames[tank-1] = qry2.value("mName").toString();
                             medIds[tank-1] = qry.value(QString("med%1_Id").arg(i+1)).toInt();
                         }
                         else
                         {
                             QMessageBox::critical(this,"ГРЕШКА", QString("Невалиден номер на бункер %1!\nРецептата не може да бъде изпълнена.").arg(tank));
                             return;
                         }
                     }else fail=true;
                }else fail=true;
            }
        }else fail= true;
    }else fail=true;

    if(fail)
    {
        QMessageBox::critical(this,"ГРЕШКА", "Възникна грешка при извличането на данните.\nРецептата не може да бъде изпълнена.");
        return;
    }

    updateWebView();

}

bool MainWindow::selectRecepieByTag(QString tag)
{
    QSqlQuery qry, qry2;
    bool ok;
    bool fail = false;
    for(int i=0 ; i!=TANKS_NUM ; i++)
    {
        targetDoses[i]=0;
        finalDoses[i]=0;
        medIds[i]=1;
    }
    QString sql = "SELECT * FROM tblRcps WHERE tag='" + tag + "';";
    qry.prepare(sql);
    if(qry.exec())
    {
        if(qry.next())
        {
            currentRecepieId = qry.value("rId").toInt();
            currentRecepieName = qry.value("rName").toString();
            //Зареждат се количествата на съставките
            for(int i = 0 ; i!= TANKS_NUM ; i++)
            {
                QString kg = qry.value(QString("med%1").arg(i+1)).toString();
                QString id = qry.value(QString("med%1_Id").arg(i+1)).toString();

                if(id=="1") continue;//с id=1 са позиции на съставки които не са включени в рецептата.

                sql = "SELECT Tank,mName,Available FROM tblMeds WHERE mID=" + id +";";
                qry2.prepare(sql);
                if(qry2.exec())
                {
                     if(qry2.next())
                     {
                         int tank = qry2.value("Tank").toInt(&ok);
                         if(ok && (tank>0) && (tank<=TANKS_NUM))
                         {
                             if(_base==1000)  targetDoses[tank-1] = kg.toInt();
                             else targetDoses[tank-1] = (int) (kg.toDouble() * _base/1000);
                             medNames[tank-1] = qry2.value("mName").toString();
                             medIds[tank-1] = qry.value(QString("med%1_Id").arg(i+1)).toInt();
                         }
                         else  return false;
                     }else fail=true;
                }else fail=true;
            }
        }else fail= true;
    }else fail=true;

    if(fail)  return false;

    updateWebView();
    return true;
}

void MainWindow::on_action_QueryExpenses_triggered()
{
    DialogQueryExpenses dlg;
    dlg.exec();
}

void MainWindow::on_action_QueryCompleteRcps_triggered()
{
    DialogQueryCompleteRcps dlg;
    dlg.exec();
}

void MainWindow::on_action_QueryIngradients_triggered()
{
    DialogQueryIngradients dlg;
    dlg.exec();
}

void MainWindow::on_action_QueryRcps_triggered()
{
    DialogQueryRcps dlg;
    dlg.exec();
}

void MainWindow::resumeFromTimeout()
{
    state = RESUME;
    updateWebView();
    qDebug() << "state = RESUME(from twido timeout);";
}

void MainWindow::on_actionAbout_triggered()
{
    DialogAbout dlg(this);
    dlg.exec();
}

void MainWindow::onNewConnection()
{
    QTcpSocket *nc = srv.nextPendingConnection();
    if (sock!=NULL)
    {
        nc->abort();
        qDebug() << "onNewConnection(): socket is already open! abort connection.";
    }
    else
    {
        sock = nc;
        connect(sock, SIGNAL(readyRead()), this, SLOT(readTcpData()));
        connect(sock, SIGNAL(disconnected()), this, SLOT(onSocketDisconnected()));

        QString msg = QString("onNewConnection(): %1:%2").arg(sock->peerAddress().toString()).arg(sock->peerPort());
        qDebug() << msg;
    }
}

void MainWindow::onSocketDisconnected()
{
    disconnect(sock, SIGNAL(readyRead()), this, SLOT(readTcpData()));
    disconnect(sock, SIGNAL(disconnected()), this, SLOT(onSocketDisconnected()));

    sock = NULL;
    qDebug() << "onSocketDisconnected()";
}

void MainWindow::readTcpData()
{
/* Получената команда трябва да бъде в следния формат:
 *
 * <SlClient>
 *   <base>1200</base>           --База за преизчисляване на зададеното количество
 *   <tag>GETSTATE</tag>    --Етикет на рецепта
 * </SlClient>
 */

    int b=sock->bytesAvailable();
    if(b==0) return;

    QByteArray data = sock->readAll();

    qDebug() << "readTcpData():"  << QString(data);

    //Разчитане на приетите данни
    bool ok;
    QDomDocument docRx; //XML приети данни
    //parse xml
    docRx.setContent(data);
    //get root element
    QDomElement rootRx = docRx.documentElement();
    QString rootName = rootRx.tagName();
    if(rootName!="SlClient")
    {
        qDebug() << "ERR: Error parsing XML. Root element must be SlClient.";
        return;
    }

    QDomNodeList childNodesList = rootRx.childNodes();
    if(childNodesList.count()<2)
    {
        qDebug() << "ERR: Error parsing XML. Child nodes missing.";
        return;
    }

    //base
    int base;
    QDomNode baseNode = childNodesList.at(0);
    QDomElement  baseElement = baseNode.toElement();
    if(baseElement.tagName() != "base")
    {
        qDebug() << "ERR: Error parsing XML. First tag must be <base>.";
        return;
    }
    else
    {
        base = baseElement.text().toInt(&ok);
        if(!ok)
        {
            qDebug() << "ERR: Error parsing XML. Failed parsing base.";
            return;
        }
    }

    //tag
    QDomNode tagNode = childNodesList.at(1);
    QDomElement  tagElement = tagNode.toElement();
    if(tagElement.tagName() != "tag")
    {
        qDebug() << "ERR: Error parsing XML. Second tag must be <tag>.";
        return;
    }

    QString tag=tagElement.text().trimmed();

    if(base<=0)
    {
        qDebug() << "ERR: TCP data - incorrect base value " << base;
        return;
    }

    if(tag == "")
    {
        qDebug() << "ERR: TCP data - empty tag " << base;
        return;
    }
    _tag=tag;
    _base=base;

    //COMMAND
    QDomNode cmdNode = childNodesList.at(2);
    QDomElement  cmdElement = cmdNode.toElement();
    if(cmdElement.tagName() != "cmd")
    {
        qDebug() << "ERR: Error parsing XML. Third tag must be <cmd>.";
        return;
    }

    QString cmd=cmdElement.text().trimmed();

    qDebug() << "XML parsed cmd=" << cmd << " tag=" << tag << " base=" <<base;
    if(cmd=="START")
    {
        noRecepieErrMsg="";
        if((state==FINISHED) || (state==IDLE))
        {
            if(selectRecepieByTag(_tag))
            {
                on_btnStart_clicked();
            }
            else
            {
//                QString msg = "Дозирането на медикаменти\nне може да бъде извършено!\nНяма конфигурирана рецепта с етикет\n" + _tag;
//                QMessageBox::critical(this,"ГРЕШКА!", msg);
                noRecepieErrMsg = "<div style=\"background-color:#FAFAFA; color:#FF0000; padding-left:20; padding-right:20;\" > <h3>";
                noRecepieErrMsg+="Дозирането на медикаменти не може да бъде извършено!<br>Няма конфигурирана рецепта с етикет" + _tag;
                noRecepieErrMsg+= " </h3></div>";
            }
        }
        else
        {   //Има опит за стартиране на дозиране по TCP докато тече друго дозиране.
            //Извежда се предупредително съобщение
            if(!tcpFailWarning.isRunning())
            {
                tcpFailWarning.start();
            }
        }
    }

    if(cmd=="STOP")
    {
        noRecepieErrMsg="";
        if((state!=FINISHED) && (state!=IDLE))
        {
            state = ABORT;
            updateWebView();
            finishStatusCode=1;
            qDebug() << "state = ABORT;";
        }
    }


}
