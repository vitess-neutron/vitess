#include "modultable.h"
#include "ui_modultable.h"
#include <QStandardItemModel>
#include <QTableWidgetItem>
#include <QMenu>
#include <QTextStream>
#include <iostream>

ModulTable::ModulTable(QStringList s1,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ModulTable)
{
    modNames = s1;
    arrow = new QIcon(":/resources/images/arrow-right.xpm");
    ui->setupUi(this);

   //for setting color of tableHeaderItems
    ui->tableWidget->verticalHeader()->setStyle(QStyleFactory::create("fusion"));

    //Get maximum width of list entries
    minWidth = 0;
    foreach (QString s, modNames)
    {
        // length of string plus 1 pixel
        int w = fontMetrics().width(s) + s.length();
        if ( minWidth < w )
            minWidth = w;
    }
    //Menu for table toolbuttons
    menu= new QMenu();
    for (int index=0; index < menuTitle.size(); index++)
    {
        subMenu << new QMenu();
        subMenu[index]->setTitle(menuTitle[index]);
    }
    bool sMenu;
    foreach (QString s, modNames)
    {
        sMenu = false;
        QAction *newAction = new QAction(s,this);
        //connect(newAction,SIGNAL(triggered()),this,SLOT(toolAction()));
        for (int index=0; index < menuSearch.size(); index++)
            if (s.startsWith(menuSearch[index]))
            {
                subMenu[index]->addAction(newAction);
                menu->addMenu(subMenu[index]);
                sMenu = true;
                break;
            }
        if (!sMenu) menu->addAction(newAction);
    }

    minWidth+=15;
    ui->tableWidget->verticalHeader()->setMinimumWidth(25);
    ui->tableWidget->setColumnWidth(0,minWidth);
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setColumnWidth(1,20);
    ui->tableWidget->adjustSize();
    oldRow = 0;

    //Menu in module table
    header = ui->tableWidget->verticalHeader();
    header->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, SIGNAL(customContextMenuRequested(const QPoint&)),this, SLOT(showContextMenu(const QPoint&)));

    addNewRow();
}


ModulTable::~ModulTable()
{
    delete ui;
}

//right mouse pressed on vertical table header
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
    moduleMenu.addAction("Disable Module",this,SLOT(disableModule()));
    moduleMenu.addAction("Enable",this,SLOT(enableModule()));
    moduleMenu.addAction("Enable all",this,SLOT(enableAllModules()));
    moduleMenu.addSeparator();
    moduleMenu.addAction("Info",this,SLOT(infoModule()));
    moduleMenu.exec(globalPos);
}

void ModulTable::disableModule()
{
    int index=ui->tableWidget->currentRow();
    disableFlag[index] = true;
    ui->tableWidget->verticalHeaderItem(index)->setTextColor(Qt::lightGray);
    ui->tableWidget->verticalHeader()->update();
}

void ModulTable::setDisabled()
{
    for (int index=0; index < disableFlag.size(); index++)
        if (disableFlag[index])
           ui->tableWidget->verticalHeaderItem(index)->setTextColor(Qt::lightGray);
}

void ModulTable::enableModule()
{
    int index=ui->tableWidget->currentRow();
    disableFlag[index] = false;
    ui->tableWidget->verticalHeaderItem(index)->setTextColor(Qt::black);
}

void ModulTable::enableAllModules()
{
    for (int index=0; index < ui->tableWidget->rowCount()-1; index++)
    {
        disableFlag[index] = false;
        ui->tableWidget->verticalHeaderItem(index)->setTextColor(Qt::black);
    }
    ui->tableWidget->verticalHeaderItem(oldRow)->setTextColor(Qt::red);
}

void ModulTable::insertModule()
{
    int index=ui->tableWidget->currentRow();
    ui->tableWidget->insertRow(index);
    QToolButton *tBut = new QToolButton();
    tBut->setText("--inactive--");
    tBut->setMenu(menu);
    tBut->setPopupMode(QToolButton::InstantPopup);
    connect(tBut,SIGNAL(triggered(QAction*)),this,SLOT(butModulItemChanged(QAction*)));
    tBut->setMinimumWidth(minWidth);    // width of modullist
    butModule.insert(index,tBut);
    QToolButton *tb = new QToolButton();
    tb->setIcon(*arrow);
    tb->setEnabled(false);
    connect(tb,SIGNAL(clicked(bool)),this,SLOT(arrowButtonPressed(bool)));
    arrowButton.insert(index,tb);
    ui->tableWidget->setCellWidget(index,0,tBut);
    ui->tableWidget->setCellWidget(index,1,tb);
    disableFlag.insert(index,false);
    for (int ind=index;ind < ui->tableWidget->rowCount();ind++)
    {
        QTableWidgetItem *vertItem = new QTableWidgetItem(QString::number(ind+1));
        ui->tableWidget->setVerticalHeaderItem(ind,vertItem);
        if (disableFlag[ind])
           ui->tableWidget->verticalHeaderItem(ind)->setTextColor(Qt::lightGray);
    }
    emit insertCombo(index);
}

//called from mainwindow when reading yaml file
void ModulTable::loadModule(QString text)
{
    foreach (QString s, modNames)
    {
        if ( s.indexOf(text) == 0)
        {
           butModule.last()->setMinimumWidth(minWidth);    // width of modullist
           butModule.last()->setText(s);
           arrowButton.last()->setEnabled(true);
           emit changedComboVal(text,butModule.size()-1);
           addNewRow();
           return;
        }
    }
}

void ModulTable::infoModule()
{
    int index=ui->tableWidget->currentRow();
    QString wwwFile = butModule[index]->text().toLower();
    QString VitessDir = QApplication::applicationDirPath().
                    left(QApplication::applicationDirPath().lastIndexOf("/"));
    QDesktopServices::openUrl(QUrl(VitessDir + "/WWW/" + wwwFile + ".html"));
}


void ModulTable::removeModule()
{
    int index=ui->tableWidget->currentRow();
    ui->tableWidget->removeRow(index);
    disableFlag.remove(index);
    for (int ind=index; ind < ui->tableWidget->rowCount();ind++)
    {
        QTableWidgetItem *vertItem = new QTableWidgetItem(QString::number(ind+1));
        ui->tableWidget->setVerticalHeaderItem(ind,vertItem);
        if (disableFlag[ind])
           ui->tableWidget->verticalHeaderItem(ind)->setTextColor(Qt::lightGray);
    }
    ui->tableWidget->verticalHeaderItem(oldRow)->setTextColor(Qt::black);
    ui->tableWidget->verticalHeaderItem(index)->setTextColor(Qt::red);
    ui->tableWidget->setCurrentCell(index,0);
    oldRow = index;
    butModule.remove(index);
    arrowButton.remove(index);
    emit removeCombo(index);
}


void ModulTable::cleanModules()
{
    disableFlag.clear();
    butModule.clear();
    arrowButton.clear();
    oldRow = 0;
    ui->tableWidget->setRowCount(0);
    addNewRow();
}
void ModulTable::addNewRow()
{
    // new ToolButton
    QToolButton *tBut = new QToolButton();
    tBut->setText("--inactive--");
    tBut->setMenu(menu);
    tBut->setPopupMode(QToolButton::InstantPopup);
    connect(tBut,SIGNAL(triggered(QAction*)),this,SLOT(butModulItemChanged(QAction*)));
    butModule << tBut;

    // new disabled arrow ToolButton
    QToolButton *tb = new QToolButton();
    tb->setIcon(*arrow);
    tb->setEnabled(false);
    connect(tb,SIGNAL(clicked(bool)),this,SLOT(arrowButtonPressed(bool)));
    arrowButton << tb;

    // new row in table with elements
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount( row+1 );
    ui->tableWidget->setCellWidget(row,0,tBut);
    ui->tableWidget->setCellWidget(row,1,tb);
    QTableWidgetItem *vertItem = new QTableWidgetItem(QString::number(row+1));
    ui->tableWidget->setVerticalHeaderItem(row,vertItem);
    // oldRow = row;

    disableFlag << false;
}

void ModulTable::butModulItemChanged(QAction* action )
{
    QToolButton *toolBut = static_cast<QToolButton*>(sender());
    toolBut->setText(action->text());
    int curRow = 0;
    for ( ; curRow<butModule.size(); curRow++ )
    {
        if ( butModule.at(curRow) == toolBut )
           {
            ui->tableWidget->verticalHeaderItem(oldRow)->setTextColor(Qt::black);
            ui->tableWidget->verticalHeaderItem(curRow)->setTextColor(Qt::red);
            oldRow = curRow;
            break;
           }
    }
    if ( toolBut->text() == "--inactive--" )
    {   // --inactive-- choosen toolbutton disabled
        arrowButton.at(curRow)->setEnabled(false);
        return;
    }
    arrowButton.at(curRow)->setEnabled(true);

    if ( curRow == ui->tableWidget->rowCount()-1 )
        addNewRow();
    emit changedComboVal(action->text(),curRow);
}


void ModulTable::arrowButtonPressed(bool)
{

    QToolButton *tb = static_cast<QToolButton*>(sender());

    int curRow = 0;
    for ( ; curRow<arrowButton.size(); curRow++ )

        if ( ui->tableWidget->cellWidget(curRow,1) == tb )
        {
            ui->tableWidget->verticalHeaderItem(oldRow)->setTextColor(Qt::black);
            ui->tableWidget->verticalHeaderItem(curRow)->setTextColor(Qt::red);
            ui->tableWidget->setCurrentCell(curRow,0);
            oldRow = curRow;
            emit arrowPressed(curRow);
            break;
        }
}
