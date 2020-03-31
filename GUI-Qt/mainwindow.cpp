#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QTableWidgetItem"
#include <iostream>
#include <QMetaType>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    modultab(nullptr)

{
    ui->setupUi(this);

    beamstop = new Beamstop(ui->stackedWidget);
    detector = new Detector(ui->stackedWidget);
    filter = new Filter(ui->stackedWidget);
    chopper_fermi_str = new Chopper_fermi_str(ui->stackedWidget);
    collimator = new Collimator(ui->stackedWidget);

    ui->stackedWidget->hide();
    ui->stackedWidget->addWidget(beamstop);
    ui->stackedWidget->addWidget(detector);
    ui->stackedWidget->addWidget(filter);
    ui->stackedWidget->addWidget(chopper_fermi_str);
    ui->stackedWidget->addWidget(collimator);

    modultab = new ModulTable(ui->widget_modul);

    connect(modultab,&ModulTable::changedCombo,this,&MainWindow::comboModulItemChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::comboModulItemChanged(QString text)
{
    ui->stackedWidget->hide();
    for (int i=0; i<  ui->stackedWidget->count(); i++)
    {
        ui->stackedWidget->setCurrentIndex(i);
        QString objectName = ui->stackedWidget->currentWidget()->objectName();
        if ( text == ui->stackedWidget->currentWidget()->objectName() )
        {
            ui->stackedWidget->show();
            break;
        }
    }

}
