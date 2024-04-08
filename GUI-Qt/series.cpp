//=============================================================================
// File:    series.cpp
// Author:  Lydia Fleischhauer-Fuß <l.fleischhauer-fuss@fz-juelich.de>
// Date:    2021
// Purpose: Generate parameters for series simulations
//=============================================================================

#include "series.h"
#include "ui_series.h"
#include <QFileDialog>
#include <iostream>
#include <QMessageBox>

//widget to fill in series parameters. There might be some limitations missing for
//max. number of simulation,files to be copied, modules and variable parameters
Series::Series(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Series)
{
    ui->setupUi(this);
    horiHeader = QStringList();
    seriesVar = QStringList();
    ui->seriesVariablen->setReadOnly(true);
    setVertHeader();
    //delete parameter from table by clicking horizontal header
    connect(ui->seriesTable->horizontalHeader(),SIGNAL(sectionClicked(int)),this,
                                                SLOT(on_sectionClicked(int)));

}

Series::~Series()
{
    delete ui;
}

void Series::setSeriesVariable(QString text, QString val)
{
    //fill series variable lineedit and table with values from mainwindow
    this->raise();
    //get current modul number from text
    int modNrCur = text.left(text.indexOf(":")).toInt();

    //sorted parameter table by modulnr
    for(int i=0;i<=ui->seriesTable->columnCount(); i++)
    {
        //get modul numnbers
        if (ui->seriesTable->columnCount() != i)
            modNr =ui->seriesTable->item(0,i)->text().left(
                   ui->seriesTable->item(0,i)->text().indexOf(":")).toInt();
        else
            //last column,set modulnr high for sorting
            modNr = 100;

        //fill table
        if(modNrCur <= modNr)
        {
           seriesVar.insert(i,text);
           ui->seriesTable->insertColumn(i);
           //set  <modulnr:-prefix>  disabled
           ui->seriesTable->setItem(0,i,new QTableWidgetItem(text.mid(1,text.lastIndexOf(":")-1)));
           ui->seriesTable->item(0,i)->
                   setFlags(ui->seriesTable->item(0,i)->flags() ^ Qt::ItemIsEnabled);
           //set value
           ui->seriesTable->setItem(2,i,new QTableWidgetItem(val));
           //default value for delta
           ui->seriesTable->setItem(1,i,new QTableWidgetItem("0"));
           //set parameter name in horizontal header
           horiHeader.insert(i, text.mid(text.lastIndexOf(":")+1));
           ui->seriesTable->setHorizontalHeaderLabels(horiHeader);
           break;
        }
    }

    //text of lineEdit
    ui->seriesVariablen->setText(seriesVar.join(""));

}

//get items from ui to save in yaml node
void Series::getSerieVariablen(YAML::Node& config)
{
    QStringList rowValues;
    QStringList delta;

    config["iteration"] = ui->iterations->value();
    config["variablen"] = seriesVar.join("").toStdString();
    config["stepSelect"] = ui->stepSelect->text().toStdString();
    config["copyFiles"] = ui->copyFiles->text().toStdString();
    config["copyDir"]   = ui->copyDir->text().toStdString();
    for(int c=0; c<ui->seriesTable->columnCount(); c++)
    {
        delta     << ui->seriesTable->item(1,c)->text();
        rowValues << ui->seriesTable->item(2,c)->text();
    }
    config["delta"] =   delta.join(" ").toStdString();
    config["values"] =  rowValues.join(" ").toStdString();

}


//load items from yaml node and fill in ui, used when loading instrument
void Series::loadSerieVariablen(YAML::Node& config)
{
    seriesVar.clear();
    ui->seriesTable->setColumnCount(0);
    ui->seriesTable->setRowCount(0);
    setVertHeader();
    QStringList rowValues;
    QStringList variable;
    QStringList delta;
    ui->iterations->setValue(config["iteration"].as<int>());
    QString str = QString::fromStdString(config["variablen"].as<std::string>());
    ui->seriesVariablen->setText(str);
    variable = str.mid(1).split(" ");
    ui->copyFiles->setText(QString::fromStdString(config["copyFiles"].as<std::string>()));
    ui->copyDir->setText(QString::fromStdString(config["copyDir"] .as<std::string>()));
    delta = QString::fromStdString(config["delta"] .as<std::string>()).split(" ");
    rowValues = QString::fromStdString(config["values"] .as<std::string>()).split(" ");
    ui->seriesTable->setRowCount(ui->iterations->value()+2);
    ui->seriesTable->setColumnCount(delta.size());
    //ui->seriesTable->setColumnCount(rowValues.size());
    //fill table
    for (int col=0; col < ui->seriesTable->columnCount(); col++)
    {
        float deltaVal = delta[col].toFloat();
        for (int row=0;row < ui->seriesTable->rowCount()-1;row++)
        {
           float val = rowValues[col].toFloat(&ok);
           if (ok)
               ui->seriesTable->setItem(row+2,col,
                   new QTableWidgetItem(QString::number(val+row*deltaVal)));
           else
           {
               ui->seriesTable->setItem(row+2,col,
                   new QTableWidgetItem(rowValues[col]));
           }
        }
        ui->seriesTable->setItem(0,col,
                         new QTableWidgetItem(variable[col].mid(0,variable[col].lastIndexOf(":"))));
        ui->seriesTable->item(0,col)->
                setFlags(ui->seriesTable->item(0,col)->flags() ^ Qt::ItemIsEnabled);
        ui->seriesTable->setItem(1,col,
               new QTableWidgetItem(delta[col]));
        //adapt horizontal header stringlist
        horiHeader.insert(col, variable[col].mid(variable[col].lastIndexOf(":")+1));

    }
    ui->seriesTable->setHorizontalHeaderLabels(horiHeader);

}


//iteration value changed
void Series::on_iterations_valueChanged(int arg1)
{
    //change number of rows in table
    ui->seriesTable->setRowCount(arg1+2);
    for (int indRow=3; indRow < arg1+2;indRow++)
    {
       //change vertical header
       QTableWidgetItem *vertItem = new QTableWidgetItem(QString::number(indRow-1));
       ui->seriesTable->setVerticalHeaderItem(indRow,vertItem);

       //increase table values by delta
       for (int indCol=0;indCol < ui->seriesTable->columnCount();indCol++)
       {
              float valDelta = ui->seriesTable->item(1,indCol)->text().toFloat();
              float val = ui->seriesTable->item(2,indCol)->text().toFloat(&ok);
              if (ok)
                  ui->seriesTable->setItem(indRow,indCol,
                            new QTableWidgetItem(QString::number(val+(indRow-2)*valDelta)));
              else
                  ui->seriesTable->setItem(indRow,indCol,
                            new QTableWidgetItem(ui->seriesTable->item(2,indCol)->text()));
       }
    }
}

//delta value in row one changed
void Series::on_seriesTable_cellChanged(int row, int column)
{
    if (row == 1)
    {
       //get delta
       float deltaVal = ui->seriesTable->item(row,column)->text().toFloat();
       //get changed value
       float val = ui->seriesTable->item(2,column)->text().toFloat(&ok);
       //adapt items in table
       for (int ind=0;ind < ui->seriesTable->rowCount()-1;ind++)
          if (ok)
              ui->seriesTable->setItem(ind+2,column,
                            new QTableWidgetItem(QString::number(val+ind*deltaVal)));
          else
              ui->seriesTable->setItem(ind+2,column,
                            new QTableWidgetItem(ui->seriesTable->item(2,column)->text()));
    }

}

//delete column by clicking horizontal header
void Series::on_sectionClicked(int index)
{
    ui->seriesTable->removeColumn(index);
    seriesVar.removeAt(index);
    ui->seriesVariablen->setText(seriesVar.join(""));
    horiHeader.removeAt(index);
}

//push start and emit signal for mainwindow
void Series::on_pushStart_clicked()
{
    QVector <QVector <QString>> table;
    QVector <QString> rowVec;
    for(int r=2; r<ui->seriesTable->rowCount(); r++)
    {
        rowVec.clear();
        for(int c=0; c<ui->seriesTable->columnCount(); c++)
           rowVec.append( ui->seriesTable->item(r,c)->text());
        table.append(rowVec);
    }
    QStringList copyFiles = ui->copyFiles->text().split(" ");
    QStringList stepSelect = ui->stepSelect->text().split(" ");

    emit startSeries(seriesVar, table, stepSelect, copyFiles, ui->copyDir->text());
}

//cancel clears entries
void Series::on_pushCancel_clicked()
{
    ui->seriesTable->setColumnCount(0);
    ui->seriesVariablen->setText("");
    seriesVar.clear();
    ui->iterations->setValue(1);
}

//create or select target directory
void Series::on_pushBrowse_clicked()
{
    QString copyDir = QFileDialog::getExistingDirectory(this,"Set copy directory",
                                               QApplication::applicationDirPath(),
                                               QFileDialog::ShowDirsOnly);
    if (copyDir != "") ui->copyDir->setText(copyDir);

}

//set vertical header of table
void Series::setVertHeader()
{
     QStringList vertHeader = {"Selection","Delta" , "1"};
     ui->seriesTable->setRowCount(3);
     for (int ind=0;ind < ui->seriesTable->rowCount();ind++)
     {
         QTableWidgetItem *vertItem = new QTableWidgetItem(vertHeader[ind]);
         ui->seriesTable->setVerticalHeaderItem(ind,vertItem);
     }

}

//toDo
void Series::on_pushScript_clicked()
{
    QMessageBox::information(this,"Export serie script", "Function not yet implemented");
}
