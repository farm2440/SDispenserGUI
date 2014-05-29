#include "dialogqueryingradients.h"
#include "ui_dialogqueryingradients.h"

DialogQueryIngradients::DialogQueryIngradients(QWidget *parent) :    QDialog(parent),    ui(new Ui::DialogQueryIngradients)
{
    ui->setupUi(this);

    QHeaderView *header = ui->tableWidget->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Interactive);

    refreshList(false);

    //Печат
    printer = new QPrinter(QPrinter::HighResolution);
    preview = new QPrintPreviewDialog(printer);
    connect(preview, SIGNAL(paintRequested(QPrinter*)), this, SLOT(print(QPrinter*)));
}

DialogQueryIngradients::~DialogQueryIngradients()
{
    delete printer;
    delete preview;
    delete ui;
}

void DialogQueryIngradients::on_btnPrint_clicked()
{
    preview->setWindowState(Qt::WindowMaximized);
    preview->exec();
}

void DialogQueryIngradients::refreshList(bool onlyAvailable)
{
    ui->tableWidget->setRowCount(0);

    QString sql;
    if(!onlyAvailable) sql = "SELECT * FROM tblMeds ORDER BY Available,Tank ASC;";
    else sql = "SELECT * FROM tblMeds WHERE Available=1 ORDER BY Tank ASC;";

    QSqlQuery qry;
    qry.prepare(sql);

    if(qry.exec())
    {
        while(qry.next())
        {
            QString mName = qry.value("mName").toString();
            if(mName=="none") continue;
            QString tank = qry.value("tank").toString();
            QString avl = qry.value("Available").toString();
            int row = ui->tableWidget->rowCount();
            ui->tableWidget->insertRow(row);
            QTableWidgetItem *itm = new QTableWidgetItem(mName);
            ui->tableWidget->setItem(row,0,itm);

            if(avl=="1") itm= new QTableWidgetItem(tank);
            else itm= new QTableWidgetItem("-");
            ui->tableWidget->setItem(row,1,itm);
        }
    }
}

void DialogQueryIngradients::on_cbAvailableOnly_toggled(bool checked)
{
    refreshList(checked);
}

void DialogQueryIngradients::print(QPrinter *prn)
{
    QTextDocument doc;
    QTextCursor cursor(&doc);

    QTextCharFormat tcf12;
    tcf12.setFontPointSize(12);

    cursor.insertText("СПРАВКА ЗА СЪСТАВКИ\nДата:",tcf12);
    QDate today = QDate::currentDate();
    cursor.insertText(QString("\n%1\n\n").arg(today.toString("d MMMM yyyy")));

    for(int i=0 ; i!=ui->tableWidget->rowCount() ; i++)
    {
        cursor.insertText( ui->tableWidget->item(i,1)->text() + "\t" );
        cursor.insertText( ui->tableWidget->item(i,0)->text() + "\n");
    }

    doc.print(prn);
    this->accept();
}
