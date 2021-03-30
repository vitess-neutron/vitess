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
    QString helpDir = QApplication::applicationDirPath();
    //If Application is under debug or release
    if (!QDir(  helpDir+"/help/").exists())
        helpDir = helpDir.left(helpDir.lastIndexOf("/"));
    helpFiles << helpDir + "/help/vitess-general.txt"
              << helpDir + "/help/getting-started.txt";
}

Help::~Help()
{
    delete ui;
}
void Help::defaultHelp()
{
   writeHelp (0);
}

void Help::on_comboBox_activated(const QString &arg1)
{
    writeHelp(ui->comboBox->currentIndex());
}

void Help::writeHelp(int i)
{
    QFile file(helpFiles[i]);
    QTextStream text(&file);
    if (!file.open(QFile::ReadOnly))
        QMessageBox::information(this, "info", file.errorString());
    ui->textBrowser->setText(text.readAll());
    file.close();
}

