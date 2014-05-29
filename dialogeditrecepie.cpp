#include "dialogeditrecepie.h"
#include "ui_dialogeditrecepie.h"

DialogEditRecepie::DialogEditRecepie(QWidget *parent , QString editId) : QDialog(parent),ui(new Ui::DialogEditRecepie)
{
    ui->setupUi(this);

    //Средната колона се разтяга до запълване на таблицата
    QHeaderView *headerView = ui->tableWidget->horizontalHeader();
    headerView->setSectionResizeMode(QHeaderView::Stretch);
    headerView->setSectionResizeMode(1, QHeaderView::Interactive);

    //Запълва се колоната с имената на съставките в реда в който са в бункерите
    QSqlQuery qry, qry2;
    QString sql;
    bool ok;

    //Първо таблицата се запълва със съставките както са разпределени по бункери.
    //За бункерите за които няма съставки се правят "disabled". След това
    //Върху тези данни се записват данните от рецептата. Така  на позиция (бункер)
    //в който в момента няма съставка или е зареден с друга, ако има съставка
    // по рецепта там ще се появи нейния запис и ще може да се редактира.
    sql = "SELECT mName,Tank FROM tblMeds WHERE Available=1;";
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

    qDebug() << "flags=" << ui->tableWidget->item(1,1)->flags();


    //Таблицата е запълнена. За тези бункери за които няма зададена съставка се забраняват
    //за редактиране полетата в таблицата.
    for(int i = 0 ; i!= TANKS_NUM ; i++)
    {
        if(ui->tableWidget->item(i,0)->text().trimmed().length()==0)
        {
            ui->tableWidget->item(i,0)->setBackgroundColor(Qt::lightGray);
            ui->tableWidget->item(i,1)->setBackgroundColor(Qt::lightGray);
            Qt::ItemFlags flags = ui->tableWidget->item(i,1)->flags();
            ui->tableWidget->item(i,1)->setFlags(flags & (~Qt::ItemIsEnabled));
        }
    }

    //За избраната рецепта за редактиране данните се нанасят по местата им
    _editId=editId;
    sql = "SELECT * FROM tblRcps WHERE rID=" + editId + ";";
    qry.prepare(sql);
    if(qry.exec())
    {
        if(qry.next())
        {
            ui->leRecepieName->setText( qry.value("rName").toString() );
            ui->leTag->setText( qry.value("tag").toString() );
            //Зареждат се количествата на съставките
            for(int i = 0 ; i!= TANKS_NUM ; i++)
            {
                QString kg = qry.value(QString("med%1").arg(i+1)).toString();
                kg=QString::number(kg.toDouble()/100);
                QString id = qry.value(QString("med%1_Id").arg(i+1)).toString();
                if(id=="1") continue;//с id=1 са позиции на съставки които не са включени в рецептата.
                sql = "SELECT Tank,mName FROM tblMeds WHERE mID=" + id +";";
                qry2.prepare(sql);
                if(qry2.exec())
                {
                     if(qry2.next())
                     {
                         int row = qry2.value("Tank").toInt(&ok) - 1;
                         if(ok && (row>=0) && (row<TANKS_NUM))
                         {
                             ui->tableWidget->item(row,1)->setText(kg);
                             ui->tableWidget->item(row,0)->setText(qry2.value("mName").toString());
                             ui->tableWidget->item(row,0)->setBackgroundColor(Qt::white);
                             ui->tableWidget->item(row,1)->setBackgroundColor(Qt::white);
                             Qt::ItemFlags flags = ui->tableWidget->item(row,1)->flags();
                             ui->tableWidget->item(row,1)->setFlags(Qt::ItemIsEnabled | flags);
                         }
                     }
                }
            }
        }
    }
}


DialogEditRecepie::~DialogEditRecepie()
{
    delete ui;
}

void DialogEditRecepie::on_btnOK_clicked()
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
    if(tag=="") sql = QString("SELECT * FROM tblRcps WHERE rName='%1' AND rId<>%2;").arg(rName).arg(_editId);
    else sql = QString("SELECT * FROM tblRcps WHERE (rName='%1' OR tag='%2') AND rID<>%3;").arg(rName).arg(tag).arg(_editId);

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

    sql = "UPDATE tblRcps SET ";
    sql += QString("rName='%1', tag='%2' ").arg(rName).arg(tag);
    for(int i=0 ; i!=TANKS_NUM ; i++)
    {
        sql += ("," + QString("med%1_Id=").arg(i+1) + listMedId[i]);
    }
    sql += " ";
    for(int i=0 ; i!=TANKS_NUM ; i++)
    {
        sql += ("," + QString("med%1=").arg(i+1) + listKg[i]);
    }
    sql += (" WHERE rId=" + _editId + ";");


    qDebug() << sql;
    qry.prepare(sql);
    qry.exec();
    accept();
}
