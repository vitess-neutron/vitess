#include "chopperphases.h"
#include "ui_chopperphases.h"
#include <QProcess>
#include <iostream>

ChopperPhases::ChopperPhases(QString modHeader,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChopperPhases)
{
    ui->setupUi(this);
    cmd = modHeader;
}

ChopperPhases::~ChopperPhases()
{
    delete ui;
}

void ChopperPhases::on_Calculate_clicked()
{
    foreach(QString key,chopPhas.keys())
    {
       if (key == "waveRange")
          if (this->findChild<QComboBox *>(key)->currentIndex() ==0)
              cmd +=" " + chopPhas[key] + "a";
          else cmd += " " + chopPhas[key] + "m";
       else
       {
        cmd += " " + chopPhas[key] + this->findChild<QLineEdit *>(key)->text();
       }
    }
    ui->textBrowser->clear();
    QProcess *toolProcess = new QProcess();
    toolProcess->start(cmd);
    if (!toolProcess->waitForStarted())
    {
       std::cout << "Error with start chop_phases" << std::endl;
       return;
    }
    if (toolProcess->waitForFinished())
    {

        QFile file("/tmp/chop_phases");
        if (!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox::information(this,"Warning cannot open: ",
                                     "/tmp/chop_phases");
            return;
        }
        QTextStream in(&file);
        QString val = in.readLine();
        ui->textBrowser->append("wavelength range for center of pulse: " + val +
                                " Ang   to   " + in.readLine()+" Ang");
        val = in.readLine();
        ui->textBrowser->append("wavelength range for whole pulse     : " + val +
                                " Ang   to   " + in.readLine()+" Ang");
        val = in.readLine();
        ui->textBrowser->append("initial chopper phase                          : " + val +
                                " deg           " + in.readLine()+" rotations");
        val = in.readLine();
        ui->textBrowser->append("chopper completely open                  : " + val +
                                " ms    to  " + in.readLine()+" ms");
    }
    else
    {
       std::cout << "Timeout in chop_phases" << std::endl;
       return;
    }

}

void ChopperPhases::on_Quit_clicked()
{
    close();
}
