#include "modultable.h"
#include "ui_modultable.h"
#include <QTreeView>
#include <QStandardItemModel>
#include <QComboBox>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QFile>
#include <QTextStream>
#include <iostream>

ModulTable::ModulTable(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ModulTable)
{
    ui->setupUi(this);
    ui->tableWidget->setColumnWidth(0,140);
    ui->tableWidget->setRowCount(1);
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setColumnWidth(1,20);

    linenum = 0;
    QFile file("/home/jcns/source/qt/test/parmodule.txt");
    if (file.open(QIODevice::ReadOnly))
    {
       QTextStream in(&file);
       comboModule << (new QComboBox());
       comboModule[0]->setView(new QTreeView(comboModule[0]));
       comboModule[0]->view()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
       comboModule[0]->setModel(new QStandardItemModel(comboModule[0]));
       while(!in.atEnd())
       {
           QString line=in.readLine();
           module[linenum++]=line.split(' ');
           if (module[linenum-1].length() > 1)
           {
               QStandardItem *item = new QStandardItem(module[linenum-1][0]);
               ((QStandardItemModel*) comboModule[0]->model())->appendRow(item);
               for (int i=1; i<module[linenum-1].length(); i++)
                  item->appendRow(new QStandardItem(module[linenum-1][i]));
           }else comboModule[0]->addItem(module[linenum-1][0]);
       }
    }else {

        std::cout <<"cannot open file" <<std::endl;
    }
    comboModule[0]->setCurrentIndex(0);
    ui->tableWidget->setCellWidget(0,0,comboModule[0]);
    arrowButton << new QToolButton();
    arrow = new QIcon(":/resources/images/arrow-right.xpm");
    arrowButton[0]->setIcon(*arrow);
    ui->tableWidget->setCellWidget(0,1,arrowButton[0]);

    connect(comboModule[0],SIGNAL(currentTextChanged(QString)),this,SLOT(comboModulItemChanged(QString)));
    connect(arrowButton[0],SIGNAL(clicked()),&mapper,SLOT(map()));
    mapper.setMapping(arrowButton[0],0);
    connect(&mapper,SIGNAL(mapped(int)),this,SLOT(arrowButtonClicked(int)));
}

ModulTable::~ModulTable()
{
    delete ui;
}

void ModulTable::arrowButtonClicked(int i)
{
    QString text=comboModule[i]->currentText();
    emit changedCombo(text);
}

void ModulTable::comboModulItemChanged(QString text)
{
    emit changedCombo(text);
    if ((ui->tableWidget->currentRow()) != ui->tableWidget->rowCount()-1) return;
    ui->tableWidget->insertRow(ui->tableWidget->rowCount());
    comboModule << (new QComboBox());
    comboModule[ui->tableWidget->rowCount()-1]->setView(new QTreeView(comboModule[0]));
    comboModule[ui->tableWidget->rowCount()-1]->view()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    comboModule[ui->tableWidget->rowCount()-1]->setModel(new QStandardItemModel(comboModule[0]));
    for (int ii=0;ii<linenum;ii++)
    {
        if (module[ii].length() > 1)
        {
            QStandardItem *item = new QStandardItem(module[ii][0]);
            ((QStandardItemModel*) comboModule[ui->tableWidget->rowCount()-1]->model())->appendRow(item);
            for (int i=1; i<module[ii].length(); i++)
                item->appendRow(new QStandardItem(module[ii][i]));
        }else {
            comboModule[ui->tableWidget->rowCount()-1]->addItem(module[ii][0]);
        }
     }
    comboModule[ui->tableWidget->rowCount()-1]->setCurrentIndex(0);
    ui->tableWidget->setCellWidget(ui->tableWidget->rowCount()-1,0,comboModule[ui->tableWidget->rowCount()-1]);
    connect(comboModule[ui->tableWidget->rowCount()-1],SIGNAL(currentTextChanged(QString)),this,SLOT(comboModulItemChanged(QString)));
    arrowButton << new QToolButton();
    arrowButton[ui->tableWidget->rowCount()-1]->setIcon(*arrow);
    ui->tableWidget->setCellWidget(ui->tableWidget->rowCount()-1,1,arrowButton[ui->tableWidget->rowCount()-1]);
    connect(arrowButton[ui->tableWidget->rowCount()-1],SIGNAL(clicked()),&mapper,SLOT(map()));
    mapper.setMapping(arrowButton[ui->tableWidget->rowCount()-1],ui->tableWidget->rowCount()-1);
    connect(&mapper,SIGNAL(mapped(int)),this,SLOT(arrowButtonClicked(int)));
}
