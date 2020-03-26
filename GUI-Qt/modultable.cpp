#include "modultable.h"
#include "ui_modultable.h"
#include <QComboBox>
#include <QTableWidgetItem>
#include <QToolButton>


ModulTable::ModulTable(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ModulTable)
{
    ui->setupUi(this);
    ui->tableWidget->setColumnWidth(0,140);
    ui->tableWidget->setRowCount(1);
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setColumnWidth(1,20);
    comboModule << (new QComboBox());
    for (int i=0; i<3; i++)
        comboModule[0]->addItem(modullist[i]);
    comboModule[0]->setCurrentIndex(0);
    ui->tableWidget->setCellWidget(0,0,comboModule[0]);
    arrowButton << new QToolButton();
    arrow = new QIcon(":/resources/images/arrow-right.xpm");
    arrowButton[0]->setIcon(*arrow);
    ui->tableWidget->setCellWidget(0,1,arrowButton[0]);

    connect(comboModule[0],SIGNAL(currentTextChanged(QString)),this,SLOT(comboModulItemChanged(QString)));
}

ModulTable::~ModulTable()
{
    delete ui;
}

void ModulTable::comboModulItemChanged(QString text)
{
    emit changedCombo(text);
    if ((ui->tableWidget->currentRow()) != ui->tableWidget->rowCount()-1) return;
    ui->tableWidget->insertRow(ui->tableWidget->rowCount());
    comboModule << (new QComboBox());
    for (int i=0; i<3; i++)
    {
        comboModule[ui->tableWidget->rowCount()-1]->addItem(modullist[i]);
        comboModule[ui->tableWidget->rowCount()-1]->setObjectName(text);
    }
    comboModule[ui->tableWidget->rowCount()-1]->setCurrentIndex(0);
    ui->tableWidget->setCellWidget(ui->tableWidget->rowCount()-1,0,comboModule[ui->tableWidget->rowCount()-1]);
    connect(comboModule[ui->tableWidget->rowCount()-1],SIGNAL(currentTextChanged(QString)),this,SLOT(comboModulItemChanged(QString)));
    arrowButton << new QToolButton();
    arrowButton[ui->tableWidget->rowCount()-1]->setIcon(*arrow);
    ui->tableWidget->setCellWidget(ui->tableWidget->rowCount()-1,1,arrowButton[ui->tableWidget->rowCount()-1]);
}
