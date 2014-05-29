#include "dialogrecepies.h"
#include "ui_dialogrecepies.h"

DialogRecepies::DialogRecepies(QWidget *parent) :    QDialog(parent),    ui(new Ui::DialogRecepies)
{
    ui->setupUi(this);

    fillRecepiesView();
    currentRcpId="";
}

DialogRecepies::~DialogRecepies()
{
    delete ui;
}

void DialogRecepies::fillRecepiesView()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    QSqlQuery qry;

    QString qryStr = "SELECT rName FROM tblRcps;";
    qry.prepare(qryStr);
    qry.exec();

    model->setQuery(qry);
    ui->tvRecepies->setModel(model);

    model->setHeaderData(0,Qt::Horizontal,"Име на рецепта");
    ui->tvRecepies->horizontalHeader()->setStretchLastSection(true);
}

void DialogRecepies::on_tvRecepies_clicked(const QModelIndex &index)
{
    qDebug() << "on_tvRecepies_clicked : row=" << index.row() << " text=" << index.data().toString();

    QSqlQuery qryR, qryM;
    QString rName;
    QString tag;
    QHash<QString,QString> ingradients; //<ИмеНаСъставка, килограми>

    QString str = "SELECT * FROM tblRcps WHERE rName ='" + index.data().toString() + "';";
    qryR.prepare(str);
    if(qryR.exec())
    {
        if (qryR.next())
        {
            rName = qryR.value("rName").toString();
            tag = qryR.value("tag").toString();
            currentRcpId = qryR.value("rID").toString();
            QString valId, valKg;
            for(int i = 1 ; i!=TANKS_NUM+1 ; i++)
            {
                QString medId = QString("med%1_Id").arg(i); //Име на колона от tblRcps за Id на лекарството
                QString medKg = QString("med%1").arg(i); //Име на колона от tblRcps за количество на лекарството

                valId = qryR.value(medId).toString();
                valKg = qryR.value(medKg).toString();
                if((valId==QString::number(NONE_ID)) || (valKg=="0")) continue; //Ако medX_Id е 0 заначи тази съставка я няма в рецептата

                //Името на лекарството се взема от tblMeds като се ползва medId
                str = "SELECT mName from tblMeds WHERE mID=" + valId +";";
                qryM.prepare(str);
                if(qryM.exec())
                {
                    if(qryM.next())
                    {
                        ingradients.insert(qryM.value("mName").toString(),valKg);
                    }else qDebug() <<"ERR: Database 4";
                }else qDebug() <<"ERR: Database 3";
            }
        }else qDebug() <<"ERR: Database 1";
    }else qDebug() <<"ERR: Database 2";

    //Извеждане на данните
    str="<html> <body>";
    str+= "<style type=\"text/css\">";
    str+="	.tftable {font-size:14px;color:#333333;width:100%;border-width: 1px;border-color: #a9a9a9;border-collapse: collapse;} ";
    str+="	.tftable th {font-size:14px;background-color:#b8b8b8;border-width: 1px;padding: 8px;border-style: solid;border-color: #a9a9a9;text-align:left;}";
    str+="	.tftable tr {background-color:#cdcdcd;}";
    str+="	.tftable td {font-size:14px;border-width: 1px;padding: 8px;border-style: solid;border-color: #a9a9a9;}";
    str+="</style>";

    str += QString("<b>Име:" + rName + "</b><br>");
    str += QString("Етикет:" + tag + "<br><br>");
    //таблица
    str +=   QString("<table class=\"tftable\" border=\"1\">");
    str += QString("<tr  style=\"padding:4px\" > <th>Съставка</th>  <th  style=\"padding:4px\" >Количество кг.</th>  </tr>");
    QHashIterator<QString, QString> itr(ingradients);
    while (itr.hasNext())
    {
        itr.next();
        str += QString("<tr> <td  style=\"padding:4px\" > "
                       + itr.key()
                       + " </td><td  style=\"padding:4px\" > "
                       + QString::number(itr.value().toDouble()/100) +"</td></tr>");
    }
    str += QString("</table></body><html>");
    ui->webView->setHtml(str);

}

void DialogRecepies::on_btnDelete_clicked()
{
    QString name =ui->tvRecepies->currentIndex().data().toString();

    QMessageBox msgBox;
    msgBox.setText(QString("ПРЕДУПРЕЖДЕНИЕ"));
    msgBox.setInformativeText(QString("Рецептата \"" + name + "\" ще бъде изтрита!\n.Сигурни ли сте?"));
    QAbstractButton *deleteBtn = msgBox.addButton(QString("ИЗТРИИ"), QMessageBox::YesRole);
    msgBox.addButton(QString("ОТКАЖИ"), QMessageBox::NoRole);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();

    if(msgBox.clickedButton() == deleteBtn)
    {
        QSqlQuery qry;
        QString str = QString("DELETE FROM tblRcps WHERE rName='" + name +"';");
        qry.prepare(str);
        if(!qry.exec()) qDebug() <<"ERR: Database 5";
        fillRecepiesView();
        ui->webView->setHtml(QString(""));
    }
}

void DialogRecepies::on_btnNew_clicked()
{
    DialogNewRecepie dlg;
    dlg.exec();
    fillRecepiesView();
    ui->webView->setHtml(QString(""));
}

void DialogRecepies::on_btnEdit_clicked()
{
    qDebug() << "Edid recepie with id=" << currentRcpId;
    DialogEditRecepie dlg(this,currentRcpId);
    dlg.exec();
    fillRecepiesView();
    ui->webView->setHtml(QString(""));
}
