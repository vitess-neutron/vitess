#include "help.h"
#include "ui_help.h"
#include <iostream>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

Help::Help(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Help)
{
    ui->setupUi(this);
    helpFiles << "/home/jcns/source/qt/test/help/vitess-general.txt"
              << "/home/jcns/source/qt/test/help/getting-started.txt";
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
    std::cout << "help:" << arg1.toStdString() << std::endl;
    writeHelp(ui->comboBox->currentIndex());
}

void Help::writeHelp(int i)
{
//    QFile file(helpFiles[ui->comboBox->currentIndex()]);
    QFile file(helpFiles[i]);
    QTextStream text(&file);
    if (!file.open(QFile::ReadOnly))
        QMessageBox::information(this, "info", file.errorString());
    ui->textBrowser->setText(text.readAll());
    file.close();
}

