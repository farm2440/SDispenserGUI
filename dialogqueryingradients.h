#ifndef DIALOGQUERYINGRADIENTS_H
#define DIALOGQUERYINGRADIENTS_H

#include <QDialog>
#include <QHeaderView>
#include <QDebug>
#include <QSqlQuery>
#include <QDate>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextCharFormat>

namespace Ui {
class DialogQueryIngradients;
}

class DialogQueryIngradients : public QDialog
{
    Q_OBJECT

public:
    explicit DialogQueryIngradients(QWidget *parent = 0);
    ~DialogQueryIngradients();
private:
    QPrinter *printer;
    QPrintPreviewDialog *preview;
private slots:
    void on_btnPrint_clicked();
    void refreshList(bool onlyAvailable);

    void on_cbAvailableOnly_toggled(bool checked);

    void print(QPrinter *prn);
private:
    Ui::DialogQueryIngradients *ui;


};

#endif // DIALOGQUERYINGRADIENTS_H
