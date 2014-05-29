#include "dialogqueryexpenses.h"
#include "ui_dialogqueryexpenses.h"

DialogQueryExpenses::DialogQueryExpenses(QWidget *parent) :    QDialog(parent),    ui(new Ui::DialogQueryExpenses)
{
    ui->setupUi(this);

    ui->dateEditEnd->setDate(QDate::currentDate());
    ui->dateEditStart->setDate(QDate::currentDate());
    //Запълване на списъка със съставките
    QSqlQuery qry;
    qry.prepare("SELECT mName FROM tblMeds");

    if(qry.exec())
    {
        while(qry.next())
        {
            QString mName = qry.value(0).toString();
            if(mName=="none") continue;
            QListWidgetItem *itm = new QListWidgetItem(mName);
            itm->setCheckState(Qt::Unchecked);
            ui->listWidget->addItem(itm);
        }
    }

    //Печат
    printer = new QPrinter(QPrinter::HighResolution);
    preview = new QPrintPreviewDialog(printer);
    connect(preview, SIGNAL(paintRequested(QPrinter*)), this, SLOT(print(QPrinter*)));
}

DialogQueryExpenses::~DialogQueryExpenses()
{
    delete printer;
    delete preview;

    delete ui;
}

void DialogQueryExpenses::on_btnPrint_clicked()
{
    QDate start = ui->dateEditStart->date();
    QDate end = ui->dateEditEnd->date();

    if(end<start)
    {
        QMessageBox::critical(this,"ГРЕШКА","Неправелно зададен период на справка!");
        return;
    }

    preview->setWindowState(Qt::WindowMaximized);
    preview->exec();
}

void DialogQueryExpenses::print(QPrinter *prn)
{
    QTextDocument doc;
    QTextCursor cursor(&doc);

    QTextCharFormat tcf12;
    tcf12.setFontPointSize(12);


    cursor.insertText("СПРАВКА ЗА РАЗХОД НА МАТЕРИАЛИ ЗА ",tcf12);

    QDate start = ui->dateEditStart->date();
    QDate end = ui->dateEditEnd->date();

    if(end<start)
    {
        QMessageBox::critical(this,"ГРЕШКА","Неправелно зададен период на справка!");
        return;
    }

    if(start==end) cursor.insertText(QString("\n%1\n\n").arg(start.toString("d MMMM yyyy")));
    else cursor.insertText(QString("ПЕРИОДА\nOT %1 ДО %2\n\n").arg(start.toString("d MMMM yyyy")).arg(end.toString("d MMMM yyyy")));

    QSqlQuery qry;
    QString sql;
    QString kg,id,mName,strTotal;
    QString tStart = start.toString("yyyy-MM-dd")  + " 00:00:00";
    QString tEnd = end.toString("yyyy-MM-dd") + " 24:00:00";

    for(int i=0 ; i!=ui->listWidget->count() ; i++)
    {
        if(ui->listWidget->item(i)->checkState()==Qt::Unchecked) continue;
        mName = ui->listWidget->item(i)->text();


        int total=0;
        for(int n=0; n!=TANKS_NUM ; n++)
        {
            id=QString("tblLog.med%1_Id").arg(n);
            kg=QString("tblLog.med%1").arg(n);
            sql=QString("SELECT SUM(%1) AS total FROM tblLog,tblMeds  WHERE %2=tblMeds.mId AND tblMeds.mName='%3' AND tblLog.Timestamp>'%4' AND tblLog.Timestamp<'%5';")
                    .arg(kg).arg(id).arg(mName).arg(tStart).arg(tEnd);
            qDebug() <<sql;
            qry.prepare(sql);
            if(qry.exec())
            {
                if(qry.next()) total+=qry.value("total").toInt();
            }
        }
        strTotal.sprintf("%3.3f",(double)total /100.0);
        cursor.insertText(mName + "\t" + strTotal +" кг.\n");
    }

    doc.print(prn);
    this->accept();
}

