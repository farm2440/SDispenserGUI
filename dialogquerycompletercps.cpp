#include "dialogquerycompletercps.h"
#include "ui_dialogquerycompletercps.h"

DialogQueryCompleteRcps::DialogQueryCompleteRcps(QWidget *parent) :  QDialog(parent), ui(new Ui::DialogQueryCompleteRcps)
{
    ui->setupUi(this);

    ui->dateEditEnd->setDate(QDate::currentDate());
    ui->dateEditStart->setDate(QDate::currentDate());
    //Запълване на списъка със съставките
    QSqlQuery qry;
    qry.prepare("SELECT rName FROM tblRcps");

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

DialogQueryCompleteRcps::~DialogQueryCompleteRcps()
{
    delete printer;
    delete preview;

    delete ui;
}

void DialogQueryCompleteRcps::on_btnPrint_clicked()
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

void DialogQueryCompleteRcps::print(QPrinter *prn)
{
    QTextDocument doc;
    QTextCursor cursor(&doc);

    QTextCharFormat tcf12;
    tcf12.setFontPointSize(12);

    QDate start = ui->dateEditStart->date();
    QDate end = ui->dateEditEnd->date();

    if(end<start)
    {
        QMessageBox::critical(this,"ГРЕШКА","Неправелно зададен период на справка!");
        return;
    }

    cursor.insertText("СПРАВКА ЗА ИЗПЪЛНЕНИ РЕЦЕПТИ ЗА ",tcf12);
    if(start==end) cursor.insertText(QString("\n%1\n\n").arg(start.toString("d MMMM yyyy")));
    else cursor.insertText(QString("ПЕРИОДА\nOT %1 ДО %2\n\n").arg(start.toString("d MMMM yyyy")).arg(end.toString("d MMMM yyyy")));

    QSqlQuery qry,qry2;
    QString sql;
    QString id,sKg,rName,mName;
    int kg;
    QString tStart = start.toString("yyyy-MM-dd")  + " 00:00:00";
    QString tEnd = end.toString("yyyy-MM-dd") + " 24:00:00";

    sql = QString("SELECT * from namedLog WHERE Timestamp>'%1' AND Timestamp<'%2' ORDER BY Timestamp ASC;")
            .arg(tStart).arg(tEnd);
    qry.prepare(sql);
    if(qry.exec())
    {
        while(qry.next())
        {
            if(qry.value("rName").toString()==NULL) continue; //"ИЗТРИТА РЕЦЕПТА";
            else rName= qry.value("rName").toString();

            bool reportRcp = false;
            for(int i=0 ; i!=ui->listWidget->count() ; i++)
            {
                if(ui->listWidget->item(i)->text()==rName)
                {
                    if(ui->listWidget->item(i)->checkState()==Qt::Checked)
                    {
                        reportRcp=true;
                        break;
                    }
                }
            }
            if(!reportRcp) continue;

            cursor.insertText("\n" + qry.value("Timestamp").toString() + "  ");
            cursor.insertText(rName + "\n");

            for(int n=0; n!=TANKS_NUM ; n++)
            {
                id=QString("med%1_Id").arg(n+1);
                if(qry.value(id).toString()=="1") continue;
                kg= qry.value(QString("med%1").arg(n+1)).toInt();
                sql=QString("SELECT mName FROM tblMeds  WHERE mId=%1;").arg(qry.value(id).toString());
                qDebug() <<sql;
                qry2.prepare(sql);
                if(qry2.exec())
                {
                    if(qry2.next())
                    {
                        mName = qry2.value("mName").toString();
                        sKg.sprintf("%3.3f",(double)kg /100.0);
                        cursor.insertText(mName + "\t" + sKg +" кг.\n");
                    }
                }
            }
        }
    }

    doc.print(prn);
    this->accept();
}

