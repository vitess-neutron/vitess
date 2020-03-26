#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QTableWidgetItem"
//#include <QDebug>
//#include <iostream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    modultab(nullptr)

{
    ui->setupUi(this);
    QString modulWidgets[2] ={"beamstop","detector"};

    beamstop = new Beamstop(ui->stackedWidget);
    detector = new Detector(ui->stackedWidget);
    ui->stackedWidget->hide();
    ui->stackedWidget->addWidget(beamstop);
    ui->stackedWidget->addWidget(detector);
    modultab = new ModulTable(ui->widget_modul);

    connect(modultab,&ModulTable::changedCombo,this,&MainWindow::comboModulItemChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::comboModulItemChanged(QString text)
{
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
