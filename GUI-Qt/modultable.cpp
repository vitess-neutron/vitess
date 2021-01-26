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
}

void ModulTable::enableModule()
{
    int index=ui->tableWidget->currentRow();
    disableFlag[index] = false;
    ui->tableWidget->verticalHeaderItem(index)->setTextColor(Qt::black);
}

void ModulTable::enableAllModules()
{
    for (int index=0; index < ui->tableWidget->rowCount()-2; index++)
    {
        disableFlag[index] = true;
        ui->tableWidget->verticalHeaderItem(index)->setTextColor(Qt::black);
    }
    ui->tableWidget->verticalHeaderItem(oldRow)->setTextColor(Qt::red);
}

void ModulTable::insertModule()
{
    int index=ui->tableWidget->currentRow();
    ui->tableWidget->insertRow(index);
    QComboBox *cb = new QComboBox();
    cb->setFocusPolicy(Qt::StrongFocus);  //no wheel change
    cb->installEventFilter(this);
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
    for (int ind=index;ind < ui->tableWidget->rowCount();ind++)
    {
        QTableWidgetItem *vertItem = new QTableWidgetItem(QString::number(ind+1));
        ui->tableWidget->setVerticalHeaderItem(ind,vertItem);
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
    QString VitessDir = QApplication::applicationDirPath().
                    left(QApplication::applicationDirPath().lastIndexOf("/"));
    QDesktopServices::openUrl(QUrl(VitessDir + "/WWW/" + wwwFile + ".html"));
}


void ModulTable::removeModule()
{
    int index=ui->tableWidget->currentRow();
    ui->tableWidget->removeRow(index);
    for (int ind=index; ind < ui->tableWidget->rowCount();ind++)
    {
        QTableWidgetItem *vertItem = new QTableWidgetItem(QString::number(ind+1));
        ui->tableWidget->setVerticalHeaderItem(ind,vertItem);
    }
    ui->tableWidget->verticalHeaderItem(oldRow)->setTextColor(Qt::black);
    ui->tableWidget->verticalHeaderItem(index)->setTextColor(Qt::red);
    ui->tableWidget->setCurrentCell(index,0);
    oldRow = index;
    comboModule.remove(index);
    arrowButton.remove(index);
    emit removeCombo(index);
}


void ModulTable::cleanModules()
{
    comboModule.clear();
    arrowButton.clear();
    oldRow = 0;
    ui->tableWidget->setRowCount(0);
    addNewRow();
}
void ModulTable::addNewRow()
{
    // new Combobox
    QComboBox *cb = new QComboBox();
    cb->setFocusPolicy(Qt::StrongFocus);
    cb->installEventFilter(this);
    //cb->setFixedWidth(minWidth);
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
    QTableWidgetItem *vertItem = new QTableWidgetItem(QString::number(row+1));
    ui->tableWidget->setVerticalHeaderItem(row,vertItem);
    // oldRow = row;

    disableFlag << false;
}

void ModulTable::comboModulItemChanged(QString text)
{
    QComboBox *cb = static_cast<QComboBox*>(sender());
    cb->setFocusPolicy(Qt::StrongFocus);  //no wheel change
    cb->installEventFilter(this);
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
            ui->tableWidget->verticalHeaderItem(oldRow)->setTextColor(Qt::black);
            ui->tableWidget->verticalHeaderItem(curRow)->setTextColor(Qt::red);
            oldRow = curRow;
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
        {
            ui->tableWidget->verticalHeaderItem(oldRow)->setTextColor(Qt::black);
            ui->tableWidget->verticalHeaderItem(curRow)->setTextColor(Qt::red);
            ui->tableWidget->setCurrentCell(curRow,0);
            oldRow = curRow;
            emit arrowPressed(curRow);
            break;
        }
}
bool ModulTable::eventFilter(QObject *obj, QEvent *ev)
{
    if(ev->type()== QEvent::Wheel)
    {
        QComboBox* combo = qobject_cast<QComboBox*>(obj);
        if (combo && !combo->hasFocus())
        return true;
    }
    return false;
}
