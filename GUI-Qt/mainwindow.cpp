#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "QTableWidgetItem"
#include <iostream>
#include <QStringList>
#include "yaml-cpp/yaml.h"
//#include "string.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    modultab(nullptr)

{
    ui->setupUi(this);
    ui->stackedWidget->hide();
    while ( ui->stackedWidget->count() > 0 )
        ui->stackedWidget->removeWidget( ui->stackedWidget->widget(0) );

    ui->stackedWidget->hide();
    ui->stackedWidget->addWidget(new Beamstop());
    ui->stackedWidget->addWidget(new Detector());
    ui->stackedWidget->addWidget(new Filter());
    ui->stackedWidget->addWidget(new Flipper_coil());
    ui->stackedWidget->addWidget(new Chopper_fermi_str());
    ui->stackedWidget->addWidget(new Chopper_disc());
    ui->stackedWidget->addWidget(new Capture_flux());
    ui->stackedWidget->addWidget(new Collimator());

    // List of modules
    QStringList modulNames;
    modulNames << "Beamstop"      // Module names must be the objectnames of the corresponding widgets
                << "Detector"
                << "Chopper:"
                << QString("%1 Chopper_disc").arg(QChar(0x2514))
                << QString("%1 Chopper_fermi_str").arg(QChar(0x2514))
                << "Collimators:"                                           // entry has subentries
                << QString("%1 Collimator").arg(QChar(0x2514))              // subentry
                << QString("%1 Collimator_radial").arg(QChar(0x2514))
                << "Filter"
                << "Flipper:"
                << QString("%1 Flipper_coil").arg(QChar(0x2514))
                << "Capture_flux";

    modultab = new ModulTable(modulNames, ui->widget_modul);

    connect(modultab,SIGNAL(changedCombo(QString)),this,SLOT(comboModulItemChanged(QString)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::comboModulItemChanged(QString text)
{
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
        switch ( text.indexOf(ui->stackedWidget->widget(i)->objectName()) )
        {
        case 0: // main entry
        case 2: // subentry
            ui->stackedWidget->setCurrentIndex(i);
            ui->stackedWidget->show();
            return;
        }
    }
    ui->stackedWidget->hide();   //no corresponding widget
}
