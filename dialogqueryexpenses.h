#ifndef DIALOGQUERYEXPENSES_H
#define DIALOGQUERYEXPENSES_H

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
class DialogQueryExpenses;
}

class DialogQueryExpenses : public QDialog
{
    Q_OBJECT

public:
    explicit DialogQueryExpenses(QWidget *parent = 0);
    ~DialogQueryExpenses();

private slots:
    void on_btnPrint_clicked();

    void print(QPrinter *prn);
private:
    Ui::DialogQueryExpenses *ui;

    QPrinter *printer;
    QPrintPreviewDialog *preview;
};

#endif // DIALOGQUERYEXPENSES_H
