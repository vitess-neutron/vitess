#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include  <QPushButton>
#include <QTextStream>
#include <QMessageBox>
#include <QDesktopServices>
#include <iostream>
#include <unistd.h>
#include <fstream>

#include "string.h"

using namespace YAML;
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    modultab(nullptr)
{
    ui->setupUi(this);
    #ifdef __unix__
       syspar = "_Linux_x86_64";
    #else    //_WIN32 _WIN64
       syspar =".exe";
    #endif
    ui->RndSeed->setValidator(new QDoubleValidator);
    while ( ui->stackedWidget->count() > 0 )
        ui->stackedWidget->removeWidget( ui->stackedWidget->widget(0) );
    ui->stackedWidget->hide();

    QDir directory ("/home/jcns/source/qt/yaml/");
    QStringList modulList;
    // List of all configuration yaml files
    fList = directory.entryList({"*.yaml"});
    for(int i=0; i<fList.count();i++ )
    {
       QString modulFile = directory.path()+"/"+fList[i];
       QFile file(modulFile);
       if (!file.open(QFile::ReadOnly | QFile::Text))
       {
        QMessageBox::information(this,"Warning cannot open: ",modulFile);
        return;
       }
       cout << "file:" << modulFile.toStdString() << endl;
       YAML::Node config = YAML::LoadFile(modulFile.toStdString());
       file.close();
        //list of modulNames for comboBox in tableWidget
       modulList << QString::fromStdString(config.begin()->first.as<string>());
       //map modulname and filename
       Module[modulList[i]] = modulFile;
       mapModul.clear();

       //scrollArea for modul
       QScrollArea *scrollArea = new QScrollArea;
       scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
       QWidget *modulWindow = new QWidget;
       gridLayout = new QGridLayout;
       modulWindow->setLayout(gridLayout);
       scrollArea->setWidget(modulWindow);
       for(int i=0; i<3; i++)  gridLayout->setColumnMinimumWidth(i,230);
       scrollArea->setWidgetResizable(true);

       //set parameters and their value definition to mapModul
       YAML::Node configParam = config.begin()->second;
       cout << "size: " << configParam.size() << endl;
       getModulParam(configParam, modulList[i]);

       // map of modulname and moduldesign
       modulGui[modulList[i]] = scrollArea;

       //map of modulname and map of modulparameter and their definitions
       mapVitess[modulList[i]] = mapModul;
    }

    modultab = new ModulTable(modulList, ui->modWidget);

    //Connect signals to slots
    //An arrow was pressed
    connect(modultab,SIGNAL(arrowPressed(int)),this,SLOT(showSelectedModul(int)));

    //Modul comboBox Value changed
    connect(modultab,SIGNAL(changedComboVal(QString,int)),this,SLOT(changeModulWidget(QString,int)));

    //Remove module from modultable
    connect(modultab,SIGNAL(removeCombo(int)),this,SLOT(removeModule(int)));

    //Insert module in modultable
    //connect(modultab,SIGNAL(insertCombo(QString,int)),this,SLOT(insertModule(QString,int)));
    connect(modultab,SIGNAL(insertCombo(int)),this,SLOT(insertModule(int)));

    //connect signals for menu action buffersize
    foreach(QAction * act, ui->menunBuffer->actions())
        connect(act,SIGNAL(triggered()),this,SLOT(BufferSize_triggered()));

    nBuffer = "50000";          //default setting

    //connect signals for min. neutron weight
    foreach(QAction * act, ui->menuMinWght->actions())
        connect(act,SIGNAL(triggered()),this,SLOT(minNeutWeight_triggered()));
    MinWght = "0.0";
//    connect(browseBut,SIGNAL(clicked()),this,SLOT(browseBut_clicked()));
//    connect(editBut,SIGNAL(clicked()),this,SLOT(editBut_clicked()));
}

MainWindow::~MainWindow()
{
    delete ui;
}


//Arrow button pressed
void MainWindow::showSelectedModul(int row)
{
    ui->stackedWidget->setCurrentIndex(row);
    ui->stackedWidget->show();
}


//Module table value changed or module added
void MainWindow::changeModulWidget(QString modul,int row)
{
    allLineEdits.clear();
    allComboBoxes.clear();
    cout << "row:  " << row << endl;
    cout << "modul:  " << modul.toStdString() << "  File:  " << Module[modul].toStdString() << endl;

    if (row != ui->stackedWidget->count())
    {
        ui->stackedWidget->removeWidget(ui->stackedWidget->widget(row));
        ui->stackedWidget->insertWidget(row,modulGui[modul]);
        ui->stackedWidget->setCurrentIndex(row);
    }
    else {
        ui->stackedWidget->addWidget(modulGui[modul]);
        ui->stackedWidget->setCurrentIndex(ui->stackedWidget->count()-1);
    }
    ui->stackedWidget->show();
}


//Remove module from module table
void MainWindow::removeModule(int row)
{
    ui->stackedWidget->hide();
    ui->stackedWidget->removeWidget(ui->stackedWidget->widget(row));
    ui->stackedWidget->setCurrentIndex(row);
    ui->stackedWidget->show();
}

//Insert module in module table
void MainWindow::insertModule(int row)
{
    ui->stackedWidget->hide();
    ui->stackedWidget->insertWidget(row,new QWidget);   //place holder until new module is selected
    ui->stackedWidget->setCurrentIndex(row);
    ui->stackedWidget->show();
}

//Menue load instrument
void MainWindow::on_actionLoad_triggered()
{

   instrumentName = QFileDialog::getOpenFileName(this,"Open Instrument","/home/jcns/Downloads/vitess3.4",
                                                 tr("YAML (*.yaml *.yml)"));
   QFileInfo fileinfo(instrumentName);
   ui->InstName->setText(fileinfo.baseName());
   QFile file(instrumentName);
   if (!file.open(QFile::ReadOnly | QFile::Text))
   {
       QMessageBox::information(this,"Warning cannot open: ",instrumentName);
       return;
   }

   modultab->cleanModules();
   while ( ui->stackedWidget->count() > 0 )
        ui->stackedWidget->removeWidget( ui->stackedWidget->widget(0) );
   config = YAML::LoadFile(instrumentName.toStdString());
   for(YAML::const_iterator it=config.begin(); it!=config.end(); ++it)
   {
       configChildren.reset();
       configChildren = it->second;
       //get module name
       QString module = QString::fromStdString(it->first.as<string>());
       if (module == "GlobalParameters")
          loadHeader(configChildren);
       else
       {
           //put module in tabelle, this sends signal changedComboVal
           modultab->loadModule(module);
           for(YAML::const_iterator it=configChildren.begin(); it!=configChildren.end(); ++it)
           {
               QString childName = QString::fromStdString(it->first.as<string>());      //key
//               cout << "childName: " << childName.toStdString() << endl;
               QWidget *modulWidget = ui->stackedWidget->widget(ui->stackedWidget->count()-1);
               if (modulWidget->findChild<QLineEdit *>(childName))
                   modulWidget->findChild<QLineEdit *>(childName)
                              ->setText(QString::fromStdString(it->second.as<string>()));
               else if (modulWidget->findChild<QComboBox *>(childName))
                   modulWidget->findChild<QComboBox *>(childName)
                              ->setCurrentText(QString::fromStdString(it->second.as<string>()));
           }
        }
   }
   file.close();
   //to do: error case
   ui->textBrowser->setText("Successfully loaded intrument:  "+fileinfo.baseName());
}

void MainWindow::on_actionSave_triggered()
{
    if (instrumentName == "")                          //no filename set
    {
        instrumentName = QFileDialog::getSaveFileName(this,"Save Instrument","/home/jcns/Downloads/vitess3.4",
                                                     "Files (*.yaml *.yml)");
        //filename without extension
        if (!instrumentName.endsWith(".yaml") && !instrumentName.endsWith(".yml"))
           instrumentName += ".yml";
    }
    saveFile(instrumentName);
}

void MainWindow::on_actionSave_as_triggered()
{

    instrumentName = QFileDialog::getSaveFileName(this,"Save Instrument as","/home/jcns/Downloads/vitess3.4",
                                                  tr("Files (*.yaml *.yml)"));
    if (!instrumentName.endsWith(".yaml") && !instrumentName.endsWith(".yml"))
        instrumentName += ".yml";
    saveFile(instrumentName);
}

void MainWindow::on_actionNewInst_triggered()
{

    while ( ui->stackedWidget->count() > 0 )
        ui->stackedWidget->removeWidget( ui->stackedWidget->widget(0) );
    ui->stackedWidget->hide();
    modultab->cleanModules();

}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionGeneral_Information_triggered()
{

    //open seperat help dialog
    Help *help_general = new Help(this);
    help_general->defaultHelp();
    help_general->show();

}

void MainWindow::on_actionTutorial_triggered()
{
    //open pdf in webbrowser
    QDesktopServices::openUrl(QUrl("/home/jcns/source/qt/test/help/tutorial.pdf"));     // tutorial.pdf

}

void MainWindow::on_pushFresh_clicked()
{

    modultab->cleanModules();
    while ( ui->stackedWidget->count() > 0 )
         ui->stackedWidget->removeWidget( ui->stackedWidget->widget(0) );
}

void MainWindow::on_pushClear_clicked()
{
    ui->textBrowser->clear();
}

void MainWindow::on_pushSave_clicked()
{

    QString logFile = QFileDialog::getSaveFileName(this,"Save logfile as","/home/jcns/Downloads/vitess3.4",
                                                   tr("Files (*.*)"));
    QFile file(logFile);
    if (!file.open(QFile::WriteOnly | QFile::Text))        //open file
    {
        QMessageBox::warning(this,"cannot open logFile: ",logFile);
        return;
    }
    ofstream fout(logFile.toStdString());          // std::ofstream
    fout << ui->textBrowser->toPlainText().toStdString();
    file.close();

}

void MainWindow::on_pushDryrun_clicked()
{

// Start a dry run.
// A dry run is a pipe execution with few neutron trajectories.
    ui->pushCheck->clicked();

    if (pipeActive == true)
    {
        ui->textBrowser->setTextColor(Qt::red);
        ui->textBrowser->append("Pipe is active");
        ui->textBrowser->setTextColor(Qt::black);
        return;
    }
    // first module should be a source module
    if (cmdList[0].indexOf("source_") < 0)
    {
        ui->textBrowser->setTextColor(Qt::red);
        ui->textBrowser->append("First module should be a source module");
        ui->textBrowser->setTextColor(Qt::black);
        return;
    }
    // generate 100 trajectories only, change parameter -n to 100000
    cmdList[0].replace(QRegExp("-n[0-9]+"), "-n100000") ;

    procList.clear();
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
        QProcess *p = new QProcess();
        procList.append(p);
    }
    connect(procList.last(),SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(finishedLast()));
    pipeActive = true;
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
        if (i < ui->stackedWidget->count()-1)
            //Output of process is input of next process
            procList[i]->setStandardOutputProcess(procList[i+1]);     //pipe commands
        //toDo error handling
        procList[i]->start(cmdList[i]);
        if (!procList[i]->waitForStarted())
        {
            cout << "Error with start proc" << i << endl;
            pipeActive = false;
            return;
        }
    }

}

void MainWindow::finishedLast()
{

    pipeActive = false;
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
       QFile file("/home/jcns/source/testlog" + QString::number(i+1));
       if (!file.open(QFile::ReadOnly | QFile::Text))
       {
           QMessageBox::information(this,"Warning cannot open: ","/home/jcns/source/testlog" + QString::number(i+1));
           return;
       }
       ui->textBrowser->append(file.readAll());
       file.close();
       procList[i]->close();
    }

}

void MainWindow::BufferSize_triggered()
{
      foreach (QAction* act,ui->menunBuffer->actions())
        act->setChecked(false);
      this->findChild<QAction *>(sender()->objectName())->setChecked(true);
      nBuffer = this->findChild<QAction *>(sender()->objectName())->text();
}

void MainWindow::minNeutWeight_triggered()
{
    foreach (QAction* act,ui->menuMinWght->actions())
        act->setChecked(false);
    this->findChild<QAction *>(sender()->objectName())->setChecked(true);
    MinWght = this->findChild<QAction *>(sender()->objectName())->text();
}


void MainWindow::writeHeader(YAML::Node& config)
{
    string gPara = "GlobalParameters";
    //get global entries from map
    foreach(QString entry, mapHeader.keys())
    {
        if (this->findChild<QLineEdit *>(entry))
            config[gPara][entry.toStdString()] = this->findChild<QLineEdit *>(entry)->text().toStdString();
        else if (this->findChild<QComboBox *>(entry))
            config[gPara][entry.toStdString()] = this->findChild<QComboBox *>(entry)->currentText().toStdString();
    }
    config[gPara]["nBuffer"] = nBuffer.toStdString();
    config[gPara]["MinWght"] = MinWght.toStdString();
    config[gPara]["Modnum"] = ui->stackedWidget->count();
}


void MainWindow::loadHeader(YAML::Node& nodeGlobal)
{

    QString childName;
    //get global values and the matching entries from map
    for(YAML::const_iterator iter=nodeGlobal.begin(); iter!=nodeGlobal.end(); ++iter)
    {
        childName = QString::fromStdString(iter->first.as<string>());      //key
        if (this->findChild<QLineEdit *>(childName))
            this->findChild<QLineEdit *>(childName)
                ->setText( QString::fromStdString(iter->second.as<string>()));
        else if(this->findChild<QComboBox *>(childName))
            this->findChild<QComboBox *>(childName)
                ->setCurrentText(QString::fromStdString(iter->second.as<string>()));

        else if(childName == "MinWght" | childName == "nBuffer")
        {
            if (childName == "MinWght")
            {
                MinWght = QString::fromStdString(iter->second.as<string>());
                str = MinWght.replace(QRegularExpression("[.|-]+"),"_");
            }else
            {
                nBuffer = QString::fromStdString(iter->second.as<string>());
                str = nBuffer;
            }
            foreach(QAction *action, this->findChild<QMenu *>("menu"+childName)->actions())
            {
                action->setChecked(false);
                if ( action->objectName().endsWith(str))
                    action->setChecked(true);
            }
        }
    }

}

void MainWindow::getHeader(QTextStream& out)
{

    //get global values and the matching entries from map
    foreach(QString entry, mapHeader.keys())
    {
        out << " " << mapHeader[entry][0];
        if (this->findChild<QLineEdit *>(entry))
            out << this->findChild<QLineEdit *>(entry)->text();
        else if (this->findChild<QComboBox *>(entry))
            out << this->findChild<QComboBox *>(entry)->currentIndex();
        else if(entry == "MinWght") out << MinWght;
        else if(entry == "nBuffer") out << nBuffer;
    }
    out << " ";
}



void MainWindow::on_pushCheck_clicked()
{

    QString headerStr, cmd;
    QTextStream header(&headerStr);
    cmdList.clear();
    getHeader(header);
    ui->textBrowser->append("Pipe would be:");
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
        allLineEdits.clear();
        allComboBoxes.clear();
        //cmd = QDir::currentPath() + "/MODULES/";
        cmd = "/home/jcns/Downloads/vitess3.4/Modules/";
        QString modulName = modulGui.key(qobject_cast<QScrollArea *>(ui->stackedWidget->widget(i)));
        cmd += modulName.toLower() + syspar;
        cmd += " --N" + QString::number(i+1);    //Modnum
        cmd += headerStr;
        cmd += " --L/home/jcns/source/testlog" + QString::number(i+1);
        foreach(QString param, mapVitess[modulName].keys())
        {
           if (mapVitess[modulName][param][6] != "")       //prefix
           {
             cmd += " " + mapVitess[modulName][param][6];
             if (ui->stackedWidget->widget(i)->findChild< QLineEdit *>(param))
                 cmd += ui->stackedWidget->widget(i)->findChild< QLineEdit *>(param)->text();
             else if (ui->stackedWidget->widget(i)->findChild< QComboBox *>(param))
                 cmd += QString::number(ui->stackedWidget->widget(i)->findChild< QComboBox *>(param)->currentIndex());
           }
        }
        cmdList.append(cmd);
        if (i < ui->stackedWidget->count()-1) cmd += " | ";
        ui->textBrowser->append(cmd);
    }

}


void MainWindow::on_pushIndir_clicked()
{
    QString userName = getenv("USER");
    QString InDir = QFileDialog::getExistingDirectory(this,"Set input directory",
                                 "/home/"+ userName,QFileDialog::ShowDirsOnly);
    ui->InDir->setText(InDir);

}

void MainWindow::on_pushOutdir_clicked()
{
    QString userName = getenv("USER");
    QString OutDir = QFileDialog::getExistingDirectory(this,"Set output directory",
                                  "/home/"+ userName,QFileDialog::ShowDirsOnly);
    ui->OutDir->setText(OutDir);

}

void MainWindow::on_pushStart_clicked()
{

    // Start execute pipe
    ui->pushCheck->clicked();

    if (pipeActive == true)
    {
        ui->textBrowser->setTextColor(Qt::red);
        ui->textBrowser->append("Pipe is active");
        ui->textBrowser->setTextColor(Qt::black);
        return;
    }
    // first module should be a source module
    if (cmdList[0].indexOf("source_") < 0)
    {
        ui->textBrowser->setTextColor(Qt::red);
        ui->textBrowser->append("First module should be a source module");
        ui->textBrowser->setTextColor(Qt::black);
        return;
    }
    procList.clear();
    for (int i=0; i<ui->stackedWidget->count(); i++)
       procList.append(new QProcess());
    connect(procList.last(),SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(finishedLast()));
    pipeActive = true;
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
        if (i < ui->stackedWidget->count()-1)
            procList[i]->setStandardOutputProcess(procList[i+1]);     //pipe commands
        procList[i]->start(cmdList[i]);
        if (!procList[i]->waitForStarted())
        {
            ui->textBrowser->setTextColor(Qt::red);
            ui->textBrowser->append( "Error with start module: " + QString::number(i));
            ui->textBrowser->setTextColor(Qt::black);
            pipeActive = false;
            return;
        }
    }

}

void MainWindow::on_pushKill_clicked()
{

    ui->textBrowser->setTextColor(Qt::red);
   for (int i=0; i<ui->stackedWidget->count(); i++)
       if (procList[i]->state() > 0)
       {
           procList[i]->kill();
           ui->textBrowser->append( "Module: " + QString::number(i) + " killed;");
       }
   ui->textBrowser->setTextColor(Qt::black);

}

void MainWindow::on_pushStop_clicked()
{

    ui->textBrowser->setTextColor(Qt::red);
    for (int i=0; i<ui->stackedWidget->count(); i++)
        if (procList[i]->state() > 0)
        {
            procList[i]->terminate();
            ui->textBrowser->append( "Module: " + QString::number(i) + " stopped;");
        }
    ui->textBrowser->setTextColor(Qt::black);

}
void MainWindow::saveFile(QString instrumentName)
{
    QFileInfo fileinfo(instrumentName);
    ui->InstName->setText(fileinfo.baseName());
    QFile file(instrumentName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox::warning(this,"Cannot open file: ",instrumentName);
    }
    //stream to write to file
    ofstream fout(instrumentName.toStdString());       // using namespace std
    //write yaml file
    config = YAML::LoadFile(instrumentName.toStdString());
    writeHeader(config);
    fout << config;
    fout << "\n";
    config.reset();
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
      allLineEdits.clear();
      allComboBoxes.clear();
      std::string key = modulGui.key(qobject_cast<QScrollArea *>(ui->stackedWidget->widget(i))).toStdString();
      cout << "key:  " << key << endl;
      allLineEdits <<  ui->stackedWidget->widget(i)->findChildren< QLineEdit *>();
      allComboBoxes <<  ui->stackedWidget->widget(i)->findChildren< QComboBox *>();

      for(int ii=0 ; ii < allLineEdits.size(); ii++)
        config[key][allLineEdits[ii]->objectName().toStdString()] = allLineEdits[ii]->text().toStdString();
      for(int ii=0 ; ii<allComboBoxes.size(); ii++)
        config[key][allComboBoxes[ii]->objectName().toStdString()] = allComboBoxes[ii]->currentText().toStdString();

      fout << config;
      fout << "\n";
      config.reset();
    }
    file.close();
}



void MainWindow::getModulParam(YAML::Node& configParam,QString modulName)
{
    int iGritRow = 0;
    int index = 0;
    label = new QLabel("<b>" +  modulName + "</b>\n");
    gridLayout->addWidget(label,iGritRow+1,0,1,3,Qt::AlignHCenter);
    iGritRow+=2;
    //loop all modul parameters
    for(YAML::const_iterator it=configParam.begin(); it!=configParam.end(); ++it)
    {

        QString parName = QString::fromStdString(it->first.as<string>());
        //list of the single parameter definitions: type,descr,default,min,max,column,prefix
        QStringList parDef = {};

        //definitions of one parameter
        YAML::Node configParamDef = it->second;
        if (configParamDef["type"]) parDef << QString::fromStdString(configParamDef["type"].as<string> ());
        else parDef << "";
        if (configParamDef["descr"])
        {
            //cout << "description:  " << configParamDef["descr"].as<string> () << endl;
            parDef << QString::fromStdString(configParamDef["descr"].as<string> ());
        }
        else parDef << "";
        if (configParamDef["default"])
        {
          if ( configParamDef["default"].size() > 1)
          {
            strList.clear();
            for(int i=0; i<static_cast<int>(configParamDef["default"].size()); i++)
               strList << QString::fromStdString(configParamDef["default"][i].as<string>());
            parDef << strList.join(",");
          }else parDef << QString::fromStdString(configParamDef["default"].as<string>());
        }else parDef << "";
        if (configParamDef["min"]) parDef << QString::fromStdString(configParamDef["min"].as<string> ());
        else parDef << "";
        if (configParamDef["max"]) parDef << QString::fromStdString(configParamDef["max"].as<string> ());
        else parDef << "";
        if (configParamDef["column"]) parDef << QString::fromStdString(configParamDef["column"].as<string> ());
        else parDef << "";
        if (configParamDef["prefix"]) parDef << QString::fromStdString(configParamDef["prefix"].as<string> ());
        else parDef << "";
        //cout << "pardef[1]: " << parDef[1].toStdString() << endl;
        mapModul[parName] = parDef;

        //add parameter to grid
        if (mapModul[parName][0] == "title")
        {
          // cout <<  "default: " <<  mapModul[parName][2].toStdString() << endl;
           label = new QLabel("<b>" + mapModul[parName][2] + "</b>\n");       //default
           gridLayout->addWidget(label,iGritRow+1,0,1,3,Qt::AlignHCenter);
           iGritRow+=2;
        }else
        {
           if ( mapModul[parName][5] == "" ||                         //column
                mapModul[parName][5].toInt() == 0 ||
                mapModul[parName][5].toInt() >2 )
           {
               iGritRow++;
               index = 0;
           }
           else index = mapModul[parName][5].toInt();
           //cout << " iGritRow:" << iGritRow << endl;

           //label = new QLabel(parName);
           cout << "label description:  " <<  mapModul[parName][1].toStdString() << endl;
           label = new QLabel(mapModul[parName][1]);                 //label desription
           label->setMinimumWidth(120);
           label->setAlignment(Qt::AlignRight);
           formLayout = new QFormLayout ;
//           validator = nullptr;
           flag= false;
           //        file,string, float, int, combo
           switch (typeList.indexOf(mapModul[parName][0]))            //check type
           {
           case 0:                                        //file
               lEdit = new QLineEdit();
               lEdit->setObjectName(parName);
               formLayout->addRow(label,lEdit);
               gridLayout->addLayout(formLayout,iGritRow,0,1,2,Qt::AlignRight);    //span over 2 columns
               formLayout = new QFormLayout;
               browseBut = new QPushButton();
               connect(browseBut,SIGNAL(clicked()),this,SLOT(browseBut_clicked()));
               browseBut->setObjectName("browse_" + parName);
               browseBut->setMinimumWidth(80);
               browseBut->setText("Browse");
               editBut = new QPushButton;
               editBut->setObjectName("edit_" + parName);
               editBut->setMinimumWidth(80);
               editBut->setText("Edit");
               formLayout->addRow(browseBut,editBut);
               gridLayout->addLayout(formLayout,iGritRow,2,1,1,Qt::AlignRight);    //span over 1 column
               iGritRow++;
               break;
           case 1:                                       //string
               validator = nullptr;
               flag = true;
           case 2:                                       //float
               if (flag == false)
               {
                   validator = new QDoubleValidator(this);
                   static_cast<QDoubleValidator*>(validator)->setNotation(QDoubleValidator::StandardNotation);
                   double val = mapModul[parName][3].toDouble(&ok);   //min    minimum
                   if (ok) static_cast<QDoubleValidator*>(validator)->setBottom(val);
                   val = mapModul[parName][4].toDouble(&ok);          //max    maximum
                   if (ok) static_cast<QDoubleValidator*>(validator)->setTop(val);
                   validator->setLocale(QLocale::C);
                   flag = true;
                   cout << parName.toStdString() << "   validator float" << endl;
               }
           case 3:                                       //int
               if (flag == false)
               {
                   QIntValidator *intValidator = new QIntValidator(this);
                   if (mapModul[parName][3].toInt())   //min    minimum
                        intValidator->setBottom( mapModul[parName][3].toInt());
                   if (mapModul[parName][4].toInt())   //max    maximum
                        intValidator->setTop( mapModul[parName][4].toInt());

                  //      validator = new QIntValidator(this);
                   validator = intValidator;
                   //validator->setLocale(QLocale::C);
                   cout << parName.toStdString() << "   validator int" << endl;
               }
               lEdit = new QLineEdit();
               lEdit->setObjectName(parName);
               lEdit->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
               lEdit->setValidator(validator);
               connect(lEdit, SIGNAL(textChanged(const QString &)),this,SLOT(checkIsValide()));
               //lEdit->setMaximumWidth(130);
               formLayout->addRow(label,lEdit);
               gridLayout->addLayout(formLayout,iGritRow,index,1,1,Qt::AlignRight);
               break;
           case 4:                                       //comboBox
               cBox = new QComboBox();
               cBox->setObjectName(parName);
               QStringList itemList = mapModul[parName][2].split(",");                  //combo items in default
               foreach (QString str, itemList)  cBox->addItem(str);
               formLayout->addRow(label,cBox);
               gridLayout->addLayout(formLayout,iGritRow,index,1,1,Qt::AlignRight);
               break;
           }
        }
    }
}


void MainWindow::browseBut_clicked()
{
    cout << "browseBut" << endl;
    QString fileName = openFileName();
    // cut browse_ from sender
    this->findChild<QLineEdit *>(
                qobject_cast<QPushButton *>(sender())->objectName().mid(7))
                ->setText(fileName);
}

void MainWindow::editBut_clicked()
{
}

QString MainWindow::openFileName()
{
    QString fileName = QFileDialog::getOpenFileName(this,"Open Instrument","/home/jcns/Downloads/vitess3.4",
                                                  tr("YAML (*.yaml *.yml)"));
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
//    if (!file.open(QFile::ReadWrite | QFile::Text))
    {
        QMessageBox::information(this,"Warning cannot open: ",fileName);
        return fileName="";
    }
    return fileName;
}
void MainWindow::checkIsValide()
{
    QLineEdit *testEdit = qobject_cast<QLineEdit *>(sender());
    palette.setColor(QPalette::Base,Qt::white);
    if (!testEdit->hasAcceptableInput() && testEdit->text() != "" )
        palette.setColor(QPalette::Base,Qt::red);
    testEdit->setPalette(palette);
}
