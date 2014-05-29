#ifndef DIALOGEDITINGRADIENTS_H
#define DIALOGEDITINGRADIENTS_H

#include <QDialog>
#include <QDebug>
#include <QSqlQuery>
#include <QMessageBox>
#include <QStringList>
#include <QRegExp>
#include <QRegExpValidator>

#include "definitions.h"

namespace Ui {
class DialogEditIngradients;
}

class DialogEditIngradients : public QDialog
{
    Q_OBJECT

public:
    explicit DialogEditIngradients(QWidget *parent = 0);
    ~DialogEditIngradients();

private slots:
    void on_btnOK_clicked();

private:
    Ui::DialogEditIngradients *ui;
};

#endif // DIALOGEDITINGRADIENTS_H
