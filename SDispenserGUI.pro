#-------------------------------------------------
#
# Project created by QtCreator 2014-05-08T19:49:11
#
#-------------------------------------------------

QT       += core gui  serialport sql printsupport network xml webkitwidgets

#CONFIG += console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SDispenser
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    dictionary.cpp \
    iniparser.cpp \
    dialogrecepies.cpp \
    dialogeditingradients.cpp \
    dialognewrecepie.cpp \
    dialogeditrecepie.cpp \
    serialreader.cpp \
    dialogqueryrcps.cpp \
    dialogqueryingradients.cpp \
    dialogqueryexpenses.cpp \
    dialogquerycompletercps.cpp \
    timeoutresumer.cpp \
    dialogabout.cpp \
    tcpstartfailwarning.cpp

HEADERS  += mainwindow.h \
    iniparser.h \
    dictionary.h \
    dialogrecepies.h \
    definitions.h \
    dialogeditingradients.h \
    dialognewrecepie.h \
    dialogeditrecepie.h \
    Settings.h \
    serialreader.h \
    dialogqueryrcps.h \
    dialogqueryingradients.h \
    dialogqueryexpenses.h \
    dialogquerycompletercps.h \
    timeoutresumer.h \
    dialogabout.h \
    tcpstartfailwarning.h

FORMS    += mainwindow.ui \
    dialogrecepies.ui \
    dialogeditingradients.ui \
    dialognewrecepie.ui \
    dialogeditrecepie.ui \
    dialogqueryrcps.ui \
    dialogqueryingradients.ui \
    dialogqueryexpenses.ui \
    dialogquerycompletercps.ui \
    dialogabout.ui

#Икона на приложението - този файл е текстов и съдържа един ред в който се указва път към файла с иконата
RC_FILE = appIcon.rc

OTHER_FILES += \
    sql.txt \
    TODO.txt

RESOURCES += \
    res.qrc
