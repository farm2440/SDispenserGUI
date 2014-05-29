#ifndef DIALOGNEWRECEPIE_H
#define DIALOGNEWRECEPIE_H

#include <QDialog>
#include <QDebug>
#include <QMessageBox>
#include <QSql>
#include <QSqlQuery>
#include <QRegExp>
#include <QRegExpValidator>

#include "definitions.h"

namespace Ui {
class DialogNewRecepie;
}

class DialogNewRecepie : public QDialog
{
    Q_OBJECT

public:
    explicit DialogNewRecepie(QWidget *parent = 0);
    //Ако edit не е празно се отваря за редактиране рецептата с такова име
    ~DialogNewRecepie();

private slots:
    void on_btnOK_clicked();

private:
    Ui::DialogNewRecepie *ui;
};

#endif // DIALOGNEWRECEPIE_H
