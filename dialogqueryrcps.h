#ifndef DIALOGQUERYRCPS_H
#define DIALOGQUERYRCPS_H

#include <QDialog>
#include <QDebug>

#include <QSql>
#include <QSqlQuery>

#include <QTextCursor>
#include <QTextDocument>
#include <QTextTable>

#include <QPrinter>
#include <QPrintPreviewDialog>

#include "definitions.h"

namespace Ui {
class DialogQueryRcps;
}

class DialogQueryRcps : public QDialog
{
    Q_OBJECT

public:
    explicit DialogQueryRcps(QWidget *parent = 0);
    ~DialogQueryRcps();

private slots:
    void on_btnPrint_clicked();
    void print(QPrinter *prn);

private:
    Ui::DialogQueryRcps *ui;

    QPrinter *printer;
    QPrintPreviewDialog *preview;

};

#endif // DIALOGQUERYRCPS_H
