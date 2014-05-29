#include "dialognewrecepie.h"
#include "ui_dialognewrecepie.h"

DialogNewRecepie::DialogNewRecepie(QWidget *parent) : QDialog(parent), ui(new Ui::DialogNewRecepie)
{
    ui->setupUi(this);
    //Средната колона се разтяга до запълване на таблицата
    QHeaderView *headerView = ui->tableWidget->horizontalHeader();
    headerView->setSectionResizeMode(QHeaderView::Stretch);
    headerView->setSectionResizeMode(1, QHeaderView::Interactive);
    //Запълва се колоната с имената на съставките в реда в който са в бункерите
    QSqlQuery qry;
    bool ok;
    QString sql = "SELECT mName,Tank FROM tblMeds WHERE Available=1;";
    qry.prepare(sql);
    if(qry.exec())
    {
        while(qry.next())
        {
            int tank = qry.value("Tank").toInt(&ok);
            if(ok && (tank>0) && (tank<=TANKS_NUM))
            {
                ui->tableWidget->item(tank-1,0)->setText(qry.value("mName").toString());
                ui->tableWidget->item(tank-1,1)->setText("0");
            }
        }
    }

    //Таблицата е запълнена. За тези бункери за които няма зададена съставка се забраняват
    //за редактиране полетата в таблицата.
    for(int i = 0 ; i!= TANKS_NUM ; i++)
    {
        QString med = ui->tableWidget->item(i,0)->text().trimmed();
        if(med.length()==0)
        {
            ui->tableWidget->item(i,0)->setBackgroundColor(Qt::lightGray);
            ui->tableWidget->item(i,1)->setBackgroundColor(Qt::lightGray);
            ui->tableWidget->item(i,1)->setFlags(0);
        }
    }
}

DialogNewRecepie::~DialogNewRecepie()
{
    delete ui;
}

void DialogNewRecepie::on_btnOK_clicked()
{
    bool ok;
    int pos;
    //Проверяват се стойностите за количество дали са валидни
    for(int i=0 ; i!=TANKS_NUM ; i++)
    {
        if(ui->tableWidget->item(i,1)->text().trimmed().length()==0) continue;
        ui->tableWidget->item(i,1)->text().toDouble(&ok);
        if(!ok)
        {
            QMessageBox::critical(this,"ГРЕШКА", "Невалидна стойност за количество на ред " +QString::number(i+1));
            return;
        }
    }
    //Проверка на името. Не трябва да съдържа символи недопустими в SQL заявка
    QString rName = ui->leRecepieName->text().trimmed();
    QString tag = ui->leTag->text().trimmed();
    if (rName.length()==0)
    {
        QMessageBox::critical(this,"ГРЕШКА","Не е въведено име на рецептата!");
        return;
    }

    QRegExp rxSafeSql("^[^<>'\"/;`%]*$");
    QRegExpValidator validator;
    validator.setRegExp(rxSafeSql);

    QValidator::State state = validator.validate(rName,pos);
    if(state != QValidator::Acceptable)
    {
        QMessageBox::critical(this,"ГРЕШКА","Името на рецептата съдържа недопустими символи!");
        return;
    }

    state = validator.validate(tag,pos);
    if(tag.length()!=0)
    {
        if(state != QValidator::Acceptable)
        {
            QMessageBox::critical(this,"ГРЕШКА","Името на етикета съдържа недопустими символи!");
            return;
        }
    }

    //Проверка, че няма дублирано име на рецепта
    QSqlQuery qry;
    QString sql;

    if(tag=="") sql = QString("SELECT * FROM tblRcps WHERE rName='%1';").arg(rName);
    else sql = QString("SELECT * FROM tblRcps WHERE rName='%1' OR tag='%2';").arg(rName).arg(tag);

    qry.prepare(sql);
    qry.exec();
    if(qry.next())
    {
        QMessageBox::critical(this,"ГРЕШКА","Вече има рецепта с такова име/етикет!");
        return;
    }

    //Запис в БД
    QStringList listMedId, listKg;
    QString strMeds,strKgs;
    //в списъците listMedId, listKg се вкарва id на лекарства и съответно килограми.
    for(int i=0 ; i!=TANKS_NUM ; i++)
    {
        QString kg = ui->tableWidget->item(i,1)->text().trimmed();
        if(kg.length()==0) kg="0";
        double dkg = kg.toDouble(&ok);
        if(!ok)
        {
            QMessageBox::critical(this,"ГРЕШКА", QString("Невалидна стойност на ред %1!").arg(i+1));
            return;
        }

        strKgs =  "0";
        strMeds = "1";

        if(dkg!=0.0)
        {
            QString mName = ui->tableWidget->item(i,0)->text();
            QString s = QString("SELECT mID FROM tblMeds WHERE mName='%1';").arg(mName);
            qry.prepare(s);
            if(qry.exec())
            {
                if(qry.next())
                {
                    strMeds = qry.value("mID").toString();
                    strKgs = QString::number(dkg * 100);
                }
            }
        }
        listKg.append(strKgs);
        listMedId.append(strMeds);
    }

    sql = "INSERT INTO tblRcps VALUES (NULL, ";
    sql += QString("'%1', '%2' ").arg(rName).arg(tag);
    for(int i=0 ; i!=TANKS_NUM ; i++) sql += ("," + listMedId[i]);
    sql += " ";
    for(int i=0 ; i!=TANKS_NUM ; i++)  sql += ("," + listKg[i]);
    sql += ");";

    qDebug() << sql;
    qry.prepare(sql);
    qry.exec();
    accept();
}
