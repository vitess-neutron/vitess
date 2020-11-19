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
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

ModulTable::ModulTable(QStringList s1,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ModulTable)
{
    modNames = s1;
    arrow = new QIcon(":/resources/images/arrow-right.xpm");
    ui->setupUi(this);
    ui->tableWidget->setColumnWidth(0,170);
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setColumnWidth(1,20);

    //Menu in module table
    header = ui->tableWidget->verticalHeader();
    header->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, SIGNAL(customContextMenuRequested(const QPoint&)),this, SLOT(showContextMenu(const QPoint&)));

    //Get maximum width of list entries
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
    QMenu moduleMenu;
    moduleMenu.addAction("Move Down",this,SLOT(insertModule()));
    moduleMenu.addAction("Remove Module",this,SLOT(removeModule()));
    moduleMenu.addSeparator();
    moduleMenu.addAction("Edit here",this,SLOT(infoModule()));
    moduleMenu.addAction("Separat Window",this,SLOT(infoModule()));
    moduleMenu.addSeparator();
    moduleMenu.addAction("Disable Module",this,SLOT(infoModule()));
    moduleMenu.addAction("Enable",this,SLOT(infoModule()));
    moduleMenu.addAction("Enable all",this,SLOT(infoModule()));
    moduleMenu.addSeparator();
    moduleMenu.addAction("Info",this,SLOT(infoModule()));
    moduleMenu.exec(globalPos);
}

void ModulTable::removeRow(int index)
{
    ui->tableWidget->removeRow(index);
}

void ModulTable::insertModule()
{
    int index=ui->tableWidget->currentRow();
    ui->tableWidget->insertRow(index);
    QComboBox *cb = new QComboBox();
    cb->addItem("--inactive--");
    cb->addItems(modNames);
    cb->setCurrentIndex(0);
    cb->view()->setMinimumWidth(minWidth);    // width of modullist
    connect(cb,SIGNAL(currentTextChanged(QString)),this,SLOT(comboModulItemChanged(QString)));
    comboModule.insert(index,cb);
    QToolButton *tb = new QToolButton();
    tb->setIcon(*arrow);
    tb->setEnabled(false);
    connect(tb,SIGNAL(clicked(bool)),this,SLOT(arrowButtonPressed(bool)));
    arrowButton.insert(index,tb);
    ui->tableWidget->setCellWidget(index,0,cb);
    ui->tableWidget->setCellWidget(index,1,tb);
    emit insertCombo(index);
}

//called from mainwindow when reading yaml file
void ModulTable::loadModule(QString text)
{
    foreach (QString s, modNames)
    {
        if (!s.endsWith(":"))
            switch ( s.indexOf(text) )
            {
                case 0: // main entry
                case 2: // subentry
                    comboModule.last()->view()->setMinimumWidth(minWidth);    // width of modullist
                    comboModule.last()->setCurrentText(s);
                    arrowButton.last()->setEnabled(true);
                    return;
            }
    }
}

void ModulTable::infoModule()
{
    int index=ui->tableWidget->currentRow();
    QString wwwFile = comboModule[index]->currentText().toLower();
    if (comboModule[index]->currentText().contains(QChar(0x2514)))
       wwwFile = wwwFile.mid(2);
    QDesktopServices::openUrl(QUrl("/home/jcns/source/qt/test/WWW/" + wwwFile + ".html"));
}


void ModulTable::removeModule()
{
    int index=ui->tableWidget->currentRow();
    ui->tableWidget->removeRow(index);
    comboModule.remove(index);
    arrowButton.remove(index);
    emit removeCombo(index);
}


void ModulTable::cleanModules()
{
    comboModule.clear();
    arrowButton.clear();

    ui->tableWidget->setRowCount(0);
    addNewRow();
}
void ModulTable::addNewRow()
{
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

    int curRow = 0;
    for ( ; curRow<comboModule.size(); curRow++ )
    {
        if ( comboModule.at(curRow) == cb )
           {
            break;
           }
    }
    if ( cb->currentIndex() == 0 )
    {   // --inactive-- choosen toolbutton disabled
        arrowButton.at(curRow)->setEnabled(false);
        return;
    }
    arrowButton.at(curRow)->setEnabled(true);
    if ( curRow == ui->tableWidget->rowCount()-1 )
        addNewRow();
    emit changedComboVal(text,curRow);
}


void ModulTable::arrowButtonPressed(bool)
{

    QToolButton *tb = static_cast<QToolButton*>(sender());

    int curRow = 0;
    for ( ; curRow<arrowButton.size(); curRow++ )

        if ( ui->tableWidget->cellWidget(curRow,1) == tb )
        {   // give mainwindow name of selected module class
//            emit arrowPressed(static_cast<QComboBox*>(ui->tableWidget->cellWidget(curRow,0))->currentText(),curRow);
            emit arrowPressed(curRow);
            break;
        }
}
