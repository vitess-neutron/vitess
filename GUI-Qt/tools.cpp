//=============================================================================
// File:    tools.cpp
// Author:  Lydia Fleischhauer-Fuß <l.fleischhauer-fuss@fz-juelich.de>
// Date:    2021
// Purpose:
//=============================================================================

#include "tools.h"


void getWidgetDesign(QString parName,QMap<QString,QString> mapParameter,
                                 QGridLayout *gridLayout,int &row,int &index)
{
    QLabel *label;
    QLineEdit *lEdit;
    QComboBox *cBox;
    QCheckBox *checkBox;
    QPushButton *browseBut, *editBut;
    QValidator *validator;
    QFormLayout *formLayout;
    bool ok;
    //add parameter to grid
    if (mapParameter["type"] == "title")                          //type
    {
       label = new QLabel("<b>" + mapParameter["default"] + "</b>\n");
       gridLayout->addWidget(label,row+1,0,1,3,Qt::AlignHCenter);
       row+=2;
    }else
    {
       if ( mapParameter["column"] == "" ||                       //column
            mapParameter["column"].toInt() == 0 ||
            mapParameter["column"].toInt() >2 )
       {
            row++;
            index = 0;
       }
       else index = mapParameter["column"].toInt();
       label = new QLabel(mapParameter["descr"]);                 //label desription
       label->setMinimumWidth(120);
       label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
       label->setToolTip(mapParameter["tooltip"]);                //toolTip
       formLayout = new QFormLayout ;
       bool flag= false;
       //        file, string, float, int, combo, switch


       switch (typeList.indexOf(mapParameter["type"]))            //check type
       {
       case 0:                                        //file
           lEdit = new QLineEdit();
           lEdit->setObjectName(parName);
           lEdit->setText(mapParameter["default"]);    //default
           formLayout->addRow(label,lEdit);
           formLayout->setAlignment(lEdit,Qt::AlignVCenter);
           gridLayout->addLayout(formLayout,row,0,1,2,Qt::AlignRight);    //span over 2 columns
           formLayout = new QFormLayout;
           browseBut = new QPushButton();
           browseBut->setObjectName("browse_" + parName);
           browseBut->setMinimumWidth(80);
           browseBut->setText("Browse");
           editBut = new QPushButton;
           editBut->setObjectName("edit_" + parName);
           editBut->setMinimumWidth(80);
           editBut->setFixedWidth(80);
           editBut->setText("Edit");
           formLayout->addRow(browseBut,editBut);
           gridLayout->addLayout(formLayout,row,2,1,1,Qt::AlignRight);    //span over 1 column
           row++;
           break;
       case 1:                                       //string
           validator = nullptr;
           flag = true;
       case 2:                                       //float
           if (flag == false)
           {
               validator = new QDoubleValidator;
               //wenn nur float Darstellung (nicht exponential)
               //static_cast<QDoubleValidator*>(validator)->setNotation(QDoubleValidator::StandardNotation);
               double val = mapParameter["min"].toDouble(&ok);          //min    minimum
               if (ok)
               {
                   static_cast<QDoubleValidator*>(validator)->setBottom(val);
                   label->setToolTip(label->toolTip() + "\nMinimum: " +mapParameter["min"]);
               }
               val = mapParameter["max"].toDouble(&ok);                 //max    maximum
               if (ok)
               {
                   static_cast<QDoubleValidator*>(validator)->setTop(val);
                   label->setToolTip(label->toolTip() + "\nMaximum: " +mapParameter["max"]);
               }
               validator->setLocale(QLocale::C);
               flag = true;
           }
       case 3:                                                               //int
           if (flag == false)
           {

               QIntValidator *intValidator = new QIntValidator;
               if (mapParameter["min"].toInt())
               {
                  intValidator->setBottom( mapParameter["min"].toInt());
                  label->setToolTip(label->toolTip() + "\nMinimum: " +mapParameter["min"]);
               }
               if (mapParameter["max"].toInt())
               {
                  intValidator->setTop( mapParameter["max"].toInt());
                  label->setToolTip(label->toolTip() + "\nMaximum: " +mapParameter["max"]);
               }
               validator = intValidator;
           }
           lEdit = new QLineEdit();
           lEdit->setObjectName(parName);
           lEdit->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
           lEdit->setValidator(validator);
           lEdit->setText(mapParameter["default"]);                    //default
           formLayout->addRow(label,lEdit);
           formLayout->setAlignment(lEdit,Qt::AlignVCenter);
           gridLayout->addLayout(formLayout,row,index,1,1,Qt::AlignRight);
           break;
       case 4:                                                               //combo     comboBox
           cBox = new QComboBox();
           cBox->setFocusPolicy(Qt::StrongFocus);
           //cBox->installEventFilter(this);
           cBox->setObjectName(parName);
           //combo items in default
           foreach (QString str, mapParameter["default"].split(",")) cBox->addItem(str);
           formLayout->addRow(label,cBox);
           gridLayout->addLayout(formLayout,row,index,1,1,Qt::AlignRight);
           break;
       case 5:                                                              //switch     checkBox
           checkBox = new QCheckBox(" ");
//           checkBox->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
           checkBox->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
           checkBox->setStyle(QStyleFactory::create("fusion"));
           checkBox->setObjectName(parName);
           if (mapParameter["default"].toUpper() == "YES")
               checkBox->setChecked(true);
           formLayout->addRow(label,checkBox);
           formLayout->setSpacing(10);
           gridLayout->addLayout(formLayout,row,index,1,1,Qt::AlignRight);
           break;
       case 6:
           label->setText(parName);
           lEdit = new QLineEdit();
           lEdit->setObjectName(parName.toLower()+"_file");
           formLayout->addRow(label,lEdit);
           gridLayout->addLayout(formLayout,row,0,1,2,Qt::AlignRight);    //span over 2 columns
           formLayout = new QFormLayout;
           browseBut = new QPushButton();
           browseBut->setObjectName("browse_" + parName);
           browseBut->setMinimumWidth(80);
           browseBut->setText("Browse");

           QPushButton *paramBut = new QPushButton;
           paramBut->setObjectName(parName);
           paramBut->setMinimumWidth(120);
           paramBut->setText(parName);
           formLayout->addRow(browseBut,paramBut);
           gridLayout->addLayout(formLayout,row,2,1,1,Qt::AlignRight);    //span over 2 columns
           break;
       }
    }
}
void pythonScript(QString instrumentDir, QStringList cmdList, QString logfile)
{
    QString fileName = QFileDialog::getSaveFileName(nullptr,"Save  python script as",instrumentDir);
    if (fileName == "") return;
    if (!fileName.endsWith(".py")) fileName += ".py";
    QFile file(fileName);
    file.open(QFile::WriteOnly | QFile::Text);
    //stream to write to file
    std::ofstream fout(fileName.toStdString());
    fout <<
         "import os\n"
         "def pwrite(fn,pattern):\n"
         " f=open(fn, 'w')\n"
         " for i in range(1," << cmdList.size()+1 <<"):\n"
         "  name = pattern+str(i)\n"
         "  for line in open(name):\n"
         "   f.write(line)\n"
         " f.close\n";
    fout << "cmd = \"" << cmdList.join(" | ").toStdString() << " --Fno_file\"\n" ;
    fout << "os.system( \"export GSL_RNG_SEED='1' GSL_RNG_TYPE='ran3' ;\" + cmd )\n";
    fout << "pwrite('" << instrumentDir.toStdString() << "/result.txt', '" << logfile.toStdString() << "')";
}

void shellScript(QString instrumentDir, QStringList cmdList, QString logfile)
{
    QString fileName = QFileDialog::getSaveFileName(nullptr,"Save  shell script as",instrumentDir);
    if (fileName == "") return;
    if (!fileName.endsWith(".sh")) fileName += ".sh";
    QFile file(fileName);
    file.open(QFile::WriteOnly | QFile::Text);
    //stream to write to file
    std::ofstream fout(fileName.toStdString());
    fout << "#!/bin/sh\n";
    fout <<  cmdList.join(" | ").toStdString() << " --Fno_file\n" ;
    fout << "cat " << logfile.toStdString() << "? > " << instrumentDir.toStdString() << "/result.txt\n";
    fout << "cat " << logfile.toStdString() << "?? >> " << instrumentDir.toStdString() << "/result.txt\n";
}
