#include "chrystanalyzer.h"
#include "ui_chrystanalyzer.h"
#include <QProcess>
#include <iostream>

Chrystanalyzer::Chrystanalyzer(QStringList modulSpec,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Chrystanalyzer)
{
    ui->setupUi(this);
    ui->cmdEdit->setText("cas_v40"+ modulSpec[1] + " --Z2 --L" + modulSpec[2] +
                         "/CAS_log60arm10.dat -PCAS_par60arm10.dat "
                         "-SCAS_S60arm10.dat -TCAS_D60arm10.dat -l6.174745 -w0.03 -k0");
    vdir = modulSpec[0];
}

Chrystanalyzer::~Chrystanalyzer()
{
    delete ui;
}

void Chrystanalyzer::on_Cancel_clicked()
{
    close();
}

void Chrystanalyzer::on_Execute_clicked()
{
    QString cmd = vdir +"/MODULES/"+ ui->cmdEdit->text();
    std::cout << cmd.toStdString() << std::endl;
    QProcess *toolProcess = new QProcess();
    toolProcess->start(cmd);
    if (!toolProcess->waitForStarted())
    {
       std::cout << "Error with start cas_v40" << std::endl;
       return;
    }

}
