#ifndef DIALOGEDITRECEPIE_H
#define DIALOGEDITRECEPIE_H

#include <QDialog>
#include <QDebug>
#include <QMessageBox>
#include <QSql>
#include <QSqlQuery>
#include <QRegExp>
#include <QRegExpValidator>

#include "definitions.h"

namespace Ui {
class DialogEditRecepie;
}

class DialogEditRecepie : public QDialog
{
    Q_OBJECT

public:
    explicit DialogEditRecepie(QWidget *parent = 0, QString editId = "");
    ~DialogEditRecepie();

private slots:
    void on_btnOK_clicked();

private:
    Ui::DialogEditRecepie *ui;
    QString _editId;
};

#endif // DIALOGEDITRECEPIE_H
