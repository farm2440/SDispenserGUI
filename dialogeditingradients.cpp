#include "dialogeditingradients.h"
#include "ui_dialogeditingradients.h"

DialogEditIngradients::DialogEditIngradients(QWidget *parent) : QDialog(parent),ui(new Ui::DialogEditIngradients)
{
    ui->setupUi(this);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

    //От БД се извличат зададените и активни съставки и според номера на бункера се записват в таблицата.
    QSqlQuery qry;
    QString sql = "SELECT mName,Tank FROM tblMeds WHERE Available=1";
    qry.prepare(sql);

    bool ok;
    int i;
    if(qry.exec())
    {
        while(qry.next())
        {
            i = qry.value("Tank").toInt(&ok);
            if(ok && (i>0) && (i<=TANKS_NUM))
            {
                ui->tableWidget->item(i-1,0)->setText(qry.value("mName").toString());
            }
        }
    }else qDebug() << "ERR: In DialogEditIngradients() failed with SQL SELECT! ";
}

DialogEditIngradients::~DialogEditIngradients()
{
    delete ui;
}

void DialogEditIngradients::on_btnOK_clicked()
{
    /* Излизане от диалога - проверяват се въведените данни и се записват промените.
     *  1. Проверява се дали в таблицата няма дублирано име на съставка
     *      и се проверява за забранени за SQL символи (срещу SQL Injection)
     *  2. За всички записи в табблицата Available става 0. Съставки които повеече не се ползват не
     *      се изтриват за да могат да бъдат включвани в справки.
     *  3. Обхожда се таблицата и се проверява дали в БД вече има съставка с такова име.
     *      Ако има, то за нея Available става 1.
     *      Ако няма, то се прави нов запис в БД.
     */
    //1
    QRegExp rxSafeSql("^[^<>'\"/;`%]*$");
    QRegExpValidator validator;
    validator.setRegExp(rxSafeSql);
    int pos;

    QStringList list;
    for(int i=0 ; i!=TANKS_NUM ; i++)
    {
        list.append(ui->tableWidget->item(i,0)->text().trimmed());
    }

    foreach(QString str , list)
    {
        QString tstr = str.trimmed();
        if(str.trimmed().length()==0) continue;
        if(list.count(tstr)>1)
        {
            QMessageBox::critical(this, "ГРЕШКА", QString("Дублирано наименование на съставка!\n%1").arg(tstr));
            return;
        }

        QValidator::State state = validator.validate(tstr,pos);
        if(state != QValidator::Acceptable)
        {
            QMessageBox::critical(this,"ГРЕШКА",QString("Името на съставката\n%1\nсъдържа недопустими символи!").arg(tstr));
            return;
        }

        if(tstr=="none")
        {
            QMessageBox::critical(this,"ГРЕШКА","\"none\" e недопустимо име на съставка!");
            return;
        }
    }
    //2
    QSqlQuery qry;
    QString sql = "UPDATE tblMeds SET Available=0;";
    qry.prepare(sql);
    if(!qry.exec()) qDebug() << "ERR:on_btnOK_clicked() failed with SQL UPDATE Available=0!";
    //3
    for(int i=0 ; i!=TANKS_NUM ; i++)
    {
        QString name = ui->tableWidget->item(i,0)->text().trimmed();
        if(name=="") continue;
        sql = "SELECT COUNT(*) FROM tblMeds WHERE mName='" + name + "';";
        qry.prepare(sql);
        if(!qry.exec()) qDebug() << "ERR:on_btnOK_clicked() failed with SQL COUNT()!";
        if(qry.next())
        {
            QString s = qry.value(0).toString();
            if(s=="0") sql = QString("INSERT INTO tblMeds VALUES (NULL,'%1',%2,1);").arg(name).arg(i+1);
            else sql = QString("UPDATE tblMeds SET Tank=%1, Available=1 WHERE mName='%2';").arg(i+1).arg(name);
            qry.prepare(sql);
            if(!qry.exec()) qDebug() << "ERR:on_btnOK_clicked() failed with SQL INSERT/UPDATE!";
        }
    }

    this->accept();
}
