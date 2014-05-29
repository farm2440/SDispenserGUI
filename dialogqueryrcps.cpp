#include "dialogqueryrcps.h"
#include "ui_dialogqueryrcps.h"

DialogQueryRcps::DialogQueryRcps(QWidget *parent) : QDialog(parent), ui(new Ui::DialogQueryRcps)
{
    ui->setupUi(this);

    //Списък с рецепти
    QSqlQuery qry;
    qry.prepare("SELECT rName FROM tblRcps;");
    if(qry.exec())
    {
        while(qry.next())
        {
            qDebug() << "Recepie: " << qry.value(0).toString();
            QListWidgetItem *itm = new QListWidgetItem(qry.value("rName").toString().trimmed());
            itm->setCheckState(Qt::Unchecked);
            ui->listWidget->addItem(itm);
        }
    }
    else qDebug() << "failed to exec SQL SELECT ";

    //Printing
    printer = new QPrinter(QPrinter::HighResolution);
    preview = new QPrintPreviewDialog(printer,this);
    connect(preview,SIGNAL(paintRequested(QPrinter*)),this,SLOT(print(QPrinter*)));
}

DialogQueryRcps::~DialogQueryRcps()
{
    delete printer;
    delete preview;

    delete ui;
}

void DialogQueryRcps::on_btnPrint_clicked()
{
    //При натискане на бутона за печат се отвяря QPrintPreviewDialog диалога.
    //Той ще извика функцията print в която се вика doc.print(printer);
    //Превю диалог:
    preview->setWindowState(Qt::WindowMaximized);
    preview->exec();
    this->accept();
}

void DialogQueryRcps::print(QPrinter *prn)
{
    QTextDocument doc;
    QTextCursor cursor(&doc);
    QTextTableFormat tabFormat;
    QTextCursor tabCursor;
    QTextTable *pTable;

    QTextCharFormat  chrFormat10;
    QTextBlockFormat blkFormatCenter, blkFormatLeft;
    QFont fnt;

    QString kg,sql;
    QSqlQuery qry, qry2;

    //Подготовка на формати за шрифтове
    chrFormat10 = cursor.charFormat();
    fnt = chrFormat10.font();
    fnt.setBold(false);
    fnt.setPointSize(10);
    chrFormat10.setFont(fnt);
    //Формати за блокове
    blkFormatCenter.setAlignment(Qt::AlignHCenter);
    blkFormatLeft.setAlignment(Qt::AlignLeft);

    //Вмъква се таблицата за данните
    tabFormat.setCellPadding(2); //отстояние от съдържанието до стените на клетката
    tabFormat.setCellSpacing(0); //отстояние между клетките
    tabFormat.setAlignment(Qt::AlignLeft);
    tabFormat.setHeaderRowCount(1); //Ако таблицата се пренася в нова страница то хедъра се отпечатва отново
    tabFormat.setBorderStyle(QTextTableFormat::BorderStyle_Solid);

    //Tekst
    cursor.insertBlock(blkFormatCenter,chrFormat10);
    cursor.insertText("   ");
    cursor.setPosition(QTextCursor::End);

    cursor.insertBlock(blkFormatCenter,chrFormat10);
    cursor.insertText("                  \n");
    cursor.setPosition(QTextCursor::End);

    for(int n=0 ; n!=ui->listWidget->count() ; n++)
    {
        if(!ui->listWidget->item(n)->checkState()) continue;

        cursor.insertBlock(blkFormatLeft,chrFormat10);
        cursor.insertText("\n\nРецепта: " + ui->listWidget->item(n)->text() + "\n");
        sql="SELECT * FROM tblRcps WHERE rName='" + ui->listWidget->item(n)->text() +"';";
        qry.prepare(sql);
        if(qry.exec())
        {
            if(qry.next())
            {
                cursor.insertText(QString("Етикет: %1\n").arg(qry.value("tag").toString()));

                pTable = cursor.insertTable(1,2,tabFormat);
                tabCursor = pTable->cellAt(0,0).firstCursorPosition();
                tabCursor.insertText("Съставка");
                tabCursor = pTable->cellAt(0,1).firstCursorPosition();
                tabCursor.insertText("Количество");

                int r=1;
                for(int i=0 ; i!=TANKS_NUM ; i++)
                {
                    kg.sprintf("%3.3f", qry.value(QString("med%1").arg(i+1)).toDouble() / 100.0);
                    QString id = qry.value(QString("med%1_Id").arg(i+1)).toString();
                    if(id=="1") continue;//с id=1 са позиции на съставки които не са включени в рецептата.

                    sql = "SELECT mName FROM tblMeds WHERE mID=" + id +";";
                    qry2.prepare(sql);
                    if(qry2.exec())
                    {
                         if(qry2.next())
                         {
                             pTable->appendRows(1);
                             tabCursor = pTable->cellAt(r,0).firstCursorPosition();
                             tabCursor.insertText(qry2.value("mName").toString());
                             tabCursor = pTable->cellAt(r,1).firstCursorPosition();
                             tabCursor.insertText(kg);
                         }
                    }
                    r++;
                }
            }
        }
        cursor.setPosition(QTextCursor::End);

    }

    cursor.setPosition(QTextCursor::Start);
    cursor.insertText("СПРАВКА РЕЦЕПТИ");
    doc.print(prn);
}
