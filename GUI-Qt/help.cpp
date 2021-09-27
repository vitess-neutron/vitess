//=============================================================================
// File:    help.cpp
// Author:  Lydia Fleischhauer-Fuß <l.fleischhauer-fuss@fz-juelich.de>
// Date:    2021
// Purpose: Dialog to show help entries
//=============================================================================

#include "help.h"
#include "ui_help.h"
#include <iostream>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QMessageBox>

Help::Help(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Help)
{
    ui->setupUi(this);
    //If Application is under debug or release
    if (!QDir(  QApplication::applicationDirPath()+"/help/").exists())
        helpDir = QApplication::applicationDirPath().
                  left(QApplication::applicationDirPath().lastIndexOf("/")) + "/help/";
    else
        helpDir = QApplication::applicationDirPath() + "/help/";
    helpFiles << helpDir + "vitess-general.txt"
              << helpDir + "getting-started.txt";
}

Help::~Help()
{
    delete ui;
}

void Help::showHelp(int i)
{
    QFile file(helpFiles[i]);
    QTextStream text(&file);
    if (!file.open(QFile::ReadOnly))
        QMessageBox::information(this, "info", file.errorString());
    ui->textBrowser->setText(text.readAll());
    file.close();
}

void Help::guiHelp()
{
    //show help for user interface without selection button
    ui->comboBox->hide();
    QFile file(helpDir+"vitess-gui.txt");
    QTextStream text(&file);
    if (!file.open(QFile::ReadOnly))
        QMessageBox::information(this, "info", file.errorString());
    ui->textBrowser->setText(text.readAll());
    file.close();
}

void Help::on_comboBox_activated(int index)
{
    showHelp(index);
}
