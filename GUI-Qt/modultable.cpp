#include "modultable.h"
#include "ui_modultable.h"
#include <QStandardItemModel>
#include <QComboBox>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QMenu>
#include <QTextStream>
#include <iostream>
#include <QDebug>

ModulTable::ModulTable(QStringList s1,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ModulTable)
{

    modNames = s1;
    arrow = new QIcon(":/resources/images/arrow-right.xpm");
    ui->setupUi(this);
    ui->tableWidget->setColumnWidth(0,165);
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setColumnWidth(1,20);

    header = ui->tableWidget->verticalHeader();
    header->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, SIGNAL(customContextMenuRequested(const QPoint&)),this, SLOT(showContextMenu(const QPoint&)));

    // get maximum width of list entries
    minWidth = 0;
    foreach (QString s, modNames)
    {
        // length of string plus 1 pixel
        int w = fontMetrics().width(s) + s.length();
        if ( minWidth < w )
            minWidth = w;
    }
    minWidth += 10;  // some more space

    addNewRow();
}


ModulTable::~ModulTable()
{
    delete ui;
}

void ModulTable::showContextMenu(const QPoint& pos)
{
    QPoint globalPos = header->mapToGlobal(pos);
    QMenu myMenu;
    myMenu.addAction("Move Down",this,SLOT(insertModule()));
    myMenu.addAction("Remove Module",this,SLOT(removeModule()));
    myMenu.addSeparator();
    myMenu.addAction("Edit here",this,SLOT(infoModule()));
    myMenu.addAction("Separat Window",this,SLOT(infoModule()));
    myMenu.addSeparator();
    myMenu.addAction("Disable Module",this,SLOT(infoModule()));
    myMenu.addAction("Enable",this,SLOT(infoModule()));
    myMenu.addAction("Enable all",this,SLOT(infoModule()));
    myMenu.addSeparator();
    myMenu.addAction("Info",this,SLOT(infoModule()));
    myMenu.exec(globalPos);
}

void ModulTable::removeRow(int index)
{
    std::cout <<"removeRow" << std::endl;
    ui->tableWidget->removeRow(index);
}

void ModulTable::insertModule()
{
    std::cout <<"insertRow" << std::endl;
}

void ModulTable::infoModule()
{
    std::cout <<"not implemented" << std::endl;
}

void ModulTable::removeModule()
{
    std::cout <<"deleteRow" << std::endl;
    int index=ui->tableWidget->currentRow();
    ui->tableWidget->removeRow(index);
    comboModule.remove(index);
    arrowButton.remove(index);
}

void ModulTable::addNewRow()
{
    std::cout <<"addNewRow" << std::endl;

    // new Combobox
    QComboBox *cb = new QComboBox();
    cb->addItem("--inactive--");
    cb->addItems(modNames);
    cb->setCurrentIndex(0);
    cb->view()->setMinimumWidth(minWidth);    // width of modullist
    connect(cb,SIGNAL(currentTextChanged(QString)),this,SLOT(comboModulItemChanged(QString)));
    comboModule << cb;

    // new disabled ToolButton
    QToolButton *tb = new QToolButton();
    tb->setIcon(*arrow);
    tb->setEnabled(false);
    connect(tb,SIGNAL(clicked(bool)),this,SLOT(arrowButtonPressed(bool)));
    arrowButton << tb;

    // new row in table with elements
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount( row+1 );
    ui->tableWidget->setCellWidget(row,0,cb);
    ui->tableWidget->setCellWidget(row,1,tb);

}

void ModulTable::comboModulItemChanged(QString text)
{
    QComboBox *cb = static_cast<QComboBox*>(sender());
    if ( text.endsWith(":") )
    {   // if top of menu selected get first sunmenu entry
        cb->setCurrentIndex( cb->currentIndex()+1 );
        return;
    }

    emit changedCombo(text);
    int curRow = 0;
    for ( ; curRow<comboModule.size(); curRow++ )
        if ( comboModule.at(curRow) == cb )
            break;
    if ( cb->currentIndex() == 0 )
    {   // --inactive-- choosen toolbutton disabled
        arrowButton.at(curRow)->setEnabled(false);
        return;
    }
    arrowButton.at(curRow)->setEnabled(true);
    // in last row then add new row
    if ( curRow == ui->tableWidget->rowCount()-1 )
        addNewRow();
}


void ModulTable::arrowButtonPressed(bool)
{

    QToolButton *tb = static_cast<QToolButton*>(sender());

    int curRow = 0;
    for ( ; curRow<arrowButton.size(); curRow++ )

        if ( ui->tableWidget->cellWidget(curRow,1) == tb )

        {   // give mainwindow name of selected module class
            emit changedCombo(static_cast<QComboBox*>(ui->tableWidget->cellWidget(curRow,0))->currentText());
            break;
        }
}

