#ifndef DIALOGRECEPIES_H
#define DIALOGRECEPIES_H

#include <QDialog>
#include <QDebug>
#include <QMessageBox>
#include<QSql>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QHash>

#include "definitions.h"
#include "dialognewrecepie.h"
#include "dialogeditrecepie.h"

namespace Ui {
class DialogRecepies;
}

class DialogRecepies : public QDialog
{
    Q_OBJECT

public:
    explicit DialogRecepies(QWidget *parent = 0);
    ~DialogRecepies();

private slots:
    void on_tvRecepies_clicked(const QModelIndex &index);

    void on_btnDelete_clicked();

    void on_btnNew_clicked();

    void on_btnEdit_clicked();

private:
    Ui::DialogRecepies *ui;

    void fillRecepiesView();
    QString currentRcpId;
};

#endif // DIALOGRECEPIES_H
