#ifndef DIALOGQUERYCOMPLETERCPS_H
#define DIALOGQUERYCOMPLETERCPS_H

#include <QDialog>
#include <QDebug>
#include <QSql>
#include <QSqlQuery>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QDateTime>
#include <QDate>
#include <QMessageBox>
#include <QTextDocument>
#include <QTextTable>
#include <QTextCursor>

#include <definitions.h>


namespace Ui {
class DialogQueryCompleteRcps;
}

class DialogQueryCompleteRcps : public QDialog
{
    Q_OBJECT

public:
    explicit DialogQueryCompleteRcps(QWidget *parent = 0);
    ~DialogQueryCompleteRcps();


private slots:
    void on_btnPrint_clicked();
    void print(QPrinter *prn);

private:
    Ui::DialogQueryCompleteRcps *ui;

    QPrinter *printer;
    QPrintPreviewDialog *preview;
};

#endif // DIALOGQUERYCOMPLETERCPS_H
