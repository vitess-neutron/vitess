#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTextStream>
#include <QScrollBar>
#include <QDesktopServices>
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
    VitessDir = QApplication::applicationDirPath().
               left(QApplication::applicationDirPath().lastIndexOf("/"));
    instrumentDir = VitessDir;
    QDir directory (VitessDir+"/yaml/");
    QStringList modulList;
    // List of all configuration yaml files
    fList = directory.entryList({"*.yaml"});
    for(int i=0; i<fList.count();i++ )
    {
       //module name   filename without extension
       modulList << fList[i].left(fList[i].lastIndexOf(".yaml"));
       QString modulFile = directory.path()+"/"+fList[i];
       QFile file(modulFile);
       if (!file.open(QFile::ReadOnly | QFile::Text))
       {
        QMessageBox::information(this,"Warning cannot open: ",modulFile);
        return;
       }
       //load yaml file
       YAML::Node config = YAML::LoadFile(modulFile.toStdString());
       file.close();

       //list of modulNames for comboBox in tableWidget
       //map modulname with filename and name of used c-module
       Module[modulList[i]] <<  modulFile << QString::fromStdString(config.begin()->first.as<string>());
       //count same modules in table
       modindex[modulList[i]] = 0;
       mapModule.clear();

       designModul(modulList[i]);
       // map of modulname and moduldesign
       modulGui[modulList[i]] = scrollArea;

       //map of modulname and map of modulparameter and their definitions
       mapVitess[modulList[i]] = mapModule;
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
    if (row != ui->stackedWidget->count())
    {
        ui->stackedWidget->removeWidget(ui->stackedWidget->widget(row));
        if (modindex[modul] == 0)
            ui->stackedWidget->insertWidget(row,modulGui[modul]);
        else
        {
            designModul(modul);
            ui->stackedWidget->insertWidget(row,scrollArea);
        }
        ui->stackedWidget->setCurrentIndex(row);
    }
    else {
        if (modindex[modul] == 0)
            ui->stackedWidget->addWidget(modulGui[modul]);
        else
        {
            designModul(modul);
            ui->stackedWidget->addWidget(scrollArea);
        }
        modindex[modul]++;
        ui->stackedWidget->setCurrentIndex(ui->stackedWidget->count()-1);
    }
    int curInd = ui->stackedWidget->currentIndex();

    QLabel *headerLab = ui->stackedWidget->widget(curInd)->findChild<QLabel*>("headerLabel");
    headerLab->setText("<b>Modul " + QString::number(row+1) +"  " +
                                   ui->stackedWidget->widget(curInd)->objectName() +"</b>");
    ui->stackedWidget->show();
}


//Remove module from module table
void MainWindow::removeModule(int row)
{
    ui->stackedWidget->hide();
    ui->stackedWidget->removeWidget(ui->stackedWidget->widget(row));
    ui->stackedWidget->setCurrentIndex(row);
    for (int ind=row; ind < ui->stackedWidget->count(); ind++)
    {
        QLabel *headerLab = ui->stackedWidget->widget(ind)->findChild<QLabel*>("headerLabel");
        headerLab->setText("<b>Modul " + QString::number(ind+1) +"  " +
                                   ui->stackedWidget->widget(ind)->objectName() +"</b>");
    }
    ui->stackedWidget->show();
}

//Insert module in module table
void MainWindow::insertModule(int row)
{
    ui->stackedWidget->hide();
    ui->stackedWidget->insertWidget(row,new QWidget);   //place holder until new module is selected
    ui->stackedWidget->setCurrentIndex(row);
    for (int ind=row+1; ind < ui->stackedWidget->count(); ind++)
    {
        QLabel *headerLab = ui->stackedWidget->widget(ind)->findChild<QLabel*>("headerLabel");
        headerLab->setText("<b>Modul " + QString::number(ind+1) +"  " +
                                   ui->stackedWidget->widget(ind)->objectName() +"</b>");
    }
    ui->stackedWidget->show();
}

//Menue load instrument
void MainWindow::on_actionLoad_triggered()
{

   instrumentName = QFileDialog::getOpenFileName(this,"Open Instrument",instrumentDir,
                                                 tr("YAML (*.yaml *.yml)"));
   QFileInfo fileinfo(instrumentName);
   ui->InstName->setText(fileinfo.baseName());
   QFile file(instrumentName);
   if (!file.open(QFile::ReadOnly | QFile::Text))
   {
       QMessageBox::information(this,"Load Instrumnet","Warning cannot open: ",instrumentName);
       return;
   }
   QMessageBox::StandardButton reply = QMessageBox::question(this,
                                 "Load Instrument",
                                 "Set default directory to\n"+ fileinfo.path(),
                                 QMessageBox::Yes|QMessageBox::No);
   if (reply == QMessageBox::Yes) instrumentDir=fileinfo.path();
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
               QWidget *modulWidget = ui->stackedWidget->widget(ui->stackedWidget->count()-1);
               if (modulWidget->findChild<QLineEdit *>(childName))
                   modulWidget->findChild<QLineEdit *>(childName)
                              ->setText(QString::fromStdString(it->second.as<string>()));
               else if (modulWidget->findChild<QComboBox *>(childName))
                   modulWidget->findChild<QComboBox *>(childName)
                              ->setCurrentText(QString::fromStdString(it->second.as<string>()));
               else if (modulWidget->findChild<QCheckBox *>(childName))
                   modulWidget->findChild<QCheckBox *>(childName)
                              ->setChecked(it->second.as<bool>());
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
        instrumentName = QFileDialog::getSaveFileName(this,"Save Instrument",instrumentDir,
                                                     "Files (*.yaml *.yml)");
        //filename without extension
        if (!instrumentName.endsWith(".yaml") && !instrumentName.endsWith(".yml"))
           instrumentName += ".yml";
    }
    saveFile(instrumentName);
}

void MainWindow::on_actionSave_as_triggered()
{

    instrumentName = QFileDialog::getSaveFileName(this,"Save Instrument as",instrumentDir,
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
    QDesktopServices::openUrl(QUrl(VitessDir+"/WWW/tutorial.pdf"));     // tutorial.pdf

}

void MainWindow::on_pushFresh_clicked()
{

    modultab->cleanModules();
    while ( ui->stackedWidget->count() > 0 )
         ui->stackedWidget->removeWidget( ui->stackedWidget->widget(0) );
    instrumentName = "";
    ui->InstName->setText(instrumentName);
}

void MainWindow::on_pushClear_clicked()
{
    ui->textBrowser->clear();
}

void MainWindow::on_pushSave_clicked()
{

    QString logFile = QFileDialog::getSaveFileName(this,"Save logfile as",instrumentDir,
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

    if (pipeActive)
    {
        ui->textBrowser->setTextColor(Qt::red);
        ui->textBrowser->append("Pipe is active");
        ui->textBrowser->setTextColor(Qt::black);
        return;
    }
    // first module should be a source module
    if ((cmdList[0].indexOf("source_") < 0) & (cmdList[0].indexOf("read_in_") < 0))
    {
        ui->textBrowser->setTextColor(Qt::red);
        ui->textBrowser->append("Please specify an input file, if the first module\ndoes not generate simulated neutrons");
        ui->textBrowser->setTextColor(Qt::black);
        return;
    }

    // generate 100 trajectories only, change parameter -n to 100000
    if (cmdList[0].contains(QRegExp("-n[0-9]+e\\+?[0-9]+")))
        cmdList[0].replace(QRegExp("-n[0-9]+e\\+?[0-9]+"), "-n100000");
    else if (cmdList[0].contains(QRegExp("-n[0-9]+")))
        cmdList[0].replace(QRegExp("-n[0-9]+"), "-n100000") ;
    else cout << "Error in -n" << endl;

    procList.clear();
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
        if (!modultab->disableFlag[i])
        {
            QProcess *p = new QProcess();
            procList.append(p);
        }
    }
    connect(procList.last(),SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(finishedLast()));
    pipeActive = true;
    int enableIndex = 0;
    for (int i=0; i< ui->stackedWidget->count(); i++)
    {
       if (!modultab->disableFlag[i])
       {
           if (enableIndex < procList.count()-1 )
           {
             //Output of process is input of next process
             procList[enableIndex]->setStandardOutputProcess(procList[enableIndex+1]);     //pipe commands
             //toDo error handling
           }
           procList[enableIndex]->start(cmdList[enableIndex]);
           if (!procList[enableIndex]->waitForStarted())
           {
              cout << "Error with start proc" << enableIndex << endl;
              pipeActive = false;
              return;
           }
           enableIndex++;
       }
    }
}

void MainWindow::finishedLast()
{

    pipeActive = false;
    for (int i=0; i < procList.count(); i++)
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

        else if((childName == "MinWght") | (childName == "nBuffer"))
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
    int enableIndex = 0;
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
        if (!modultab->disableFlag[i])
        {
            allLineEdits.clear();
            allComboBoxes.clear();
            cmd = VitessDir + "/MODULES/";
            QString modulName = ui->stackedWidget->widget(i)->objectName();

            //Module  key:modulename  value:filename,c-module
            cmd += Module[modulName][1].toLower() + syspar;

            cmd += " --N" + QString::number(i+1);    //Modnum
            cmd += headerStr;
            cmd += " --L/home/jcns/source/testlog" + QString::number(enableIndex+1);
           foreach(QString param, mapVitess[modulName].keys())
           {
            if (mapVitess[modulName][param]["prefix"] != "")       //prefix
            {
              if ((ui->stackedWidget->widget(i)->findChild< QLineEdit *>(param)) &&
                 (ui->stackedWidget->widget(i)->findChild< QLineEdit *>(param)->text() != ""))
              {
                 cmd += " " + mapVitess[modulName][param]["prefix"];
                 cmd += ui->stackedWidget->widget(i)->findChild< QLineEdit *>(param)->text();
              }
              else if (ui->stackedWidget->widget(i)->findChild< QComboBox *>(param))
              {
                 cmd += " " + mapVitess[modulName][param]["prefix"];
                 int curInd = ui->stackedWidget->widget(i)->findChild< QComboBox *>(param)->currentIndex();
                 if (mapVitess[modulName][param]["index"] != "")         //index
                      cmd += mapVitess[modulName][param]["index"].split(",")[curInd];
                 else cmd += QString::number(curInd);
              }
              else if (ui->stackedWidget->widget(i)->findChild< QCheckBox *>(param))
              {
                 cmd += " " + mapVitess[modulName][param]["prefix"];
                 cmd += QString::number(ui->stackedWidget->widget(i)->findChild< QCheckBox *>(param)->isChecked());
              }
            }
           }
           cmdList.append(cmd);
           if (i < ui->stackedWidget->count()-1) cmd += " | ";
           ui->textBrowser->append(cmd);
           ui->textBrowser->verticalScrollBar()->
                   setValue(ui->textBrowser->verticalScrollBar()->maximum());

           enableIndex++;
        }
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
      allCheckBoxes.clear();
      string key = ui->stackedWidget->widget(i)->objectName().toStdString();       //std::string
      allLineEdits <<  ui->stackedWidget->widget(i)->findChildren< QLineEdit *>();
      allComboBoxes <<  ui->stackedWidget->widget(i)->findChildren< QComboBox *>();
      allCheckBoxes <<  ui->stackedWidget->widget(i)->findChildren< QCheckBox *>();

      for(int ii=0 ; ii < allLineEdits.size(); ii++)
        config[key][allLineEdits[ii]->objectName().toStdString()] = allLineEdits[ii]->text().toStdString();
      for(int ii=0 ; ii<allComboBoxes.size(); ii++)
        config[key][allComboBoxes[ii]->objectName().toStdString()] = allComboBoxes[ii]->currentText().toStdString();
      for(int ii=0 ; ii<allCheckBoxes.size(); ii++)
        config[key][allCheckBoxes[ii]->objectName().toStdString()] = allCheckBoxes[ii]->isChecked();

      fout << config;
      fout << "\n";
      config.reset();
    }
    file.close();
}

void MainWindow::designModul(QString modulName)
{
    scrollArea = new QScrollArea;
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    QWidget *modulWindow = new QWidget;
    gridLayout = new QGridLayout;
    modulWindow->setLayout(gridLayout);
    scrollArea->setWidget(modulWindow);
    scrollArea->setObjectName(modulName);
    for(int col=0; col<3; col++)  gridLayout->setColumnMinimumWidth(col,230);
    scrollArea->setWidgetResizable(true);
    //YAML::Node config = YAML::LoadFile(Module[modulName].toStdString());
    YAML::Node config = YAML::LoadFile(Module[modulName][0].toStdString());
    YAML::Node configParam = config.begin()->second;
    getModulParameter(configParam, modulName);

}

void MainWindow::getModulParameter(YAML::Node& configParam,QString modulName)
{
    int iGritRow = 0;
    int index = 0;
    QLabel *headerLabel = new QLabel("<b>" +  modulName + "</b>\n");
    headerLabel->setObjectName("headerLabel");
    gridLayout->addWidget(headerLabel,iGritRow+1,0,1,3,Qt::AlignHCenter);
    iGritRow+=2;
    //loop all modul parameters
    for(YAML::const_iterator it=configParam.begin(); it!=configParam.end(); ++it)
    {

        QString parName = QString::fromStdString(it->first.as<string>());
        //list of the single parameter definitions: type,descr,default,min,max,column,prefix
        //definitions of one parameter

        YAML::Node configParamDef = it->second;
        foreach(QString key,mapParam.keys())
        {
            if (configParamDef[key.toStdString()])
                if ( configParamDef[key.toStdString()].size() > 1)
                {
                   strList.clear();
                   for(int i=0; i<static_cast<int>(configParamDef[key.toStdString()].size()); i++)
                      strList << QString::fromStdString(configParamDef[key.toStdString()][i].as<string>());
                   mapParam[key] = strList.join(",");
                }else  mapParam[key] = QString::fromStdString(configParamDef[key.toStdString()].as<string>());
            else mapParam[key] = "";
        }
        mapModule[parName] = mapParam;
        getWidgetDesign(parName,mapParam, gridLayout,iGritRow,index);

        if( scrollArea->widget()->findChild<QPushButton*>("browse_" + parName))
            connect(scrollArea->widget()->findChild<QPushButton*>("browse_" + parName),
                    SIGNAL(clicked()),this,SLOT(browseBut_clicked()));
        else if( scrollArea->widget()->findChild<QPushButton*>(parName))
        {
            paramWindow.append( new Parameter);
            connect(scrollArea->widget()->findChild<QPushButton*>(parName),
                    SIGNAL(clicked()),this,SLOT(paramBut_clicked()));
        }
        else if( scrollArea->widget()->findChild<QLineEdit*>(parName))
                connect(scrollArea->widget()->findChild<QLineEdit*>(parName),
                    SIGNAL(textChanged(const QString &)),this,SLOT(checkIsValide()));
    }
    if (iGritRow <= 10)
    {
        iGritRow++;
        gridLayout->addItem( new QSpacerItem(20,40,QSizePolicy::Minimum,QSizePolicy::Expanding),iGritRow,0);
    }
}

void MainWindow::paramBut_clicked()
{
    QString yamlPath = QApplication::applicationDirPath().
                       left(QApplication::applicationDirPath().lastIndexOf("/"))+"/yaml/parameter/";
    QString param = qobject_cast<QPushButton *>(sender())->text();
    QString filename = yamlPath + param.toLower() + ".yaml";
    paramWindow[ui->stackedWidget->currentIndex()]->designParameterWin(filename);
    paramWindow[ui->stackedWidget->currentIndex()]->show();
}


void MainWindow::browseBut_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,"Open Instrument",instrumentDir);
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::information(this,"Warning cannot open: ",fileName);
        return;
    }
    // cut browse_ from sender
    ui->stackedWidget->currentWidget()->findChild<QLineEdit *>(
                qobject_cast<QPushButton *>(sender())->objectName().mid(7))
                ->setText(fileName);
}

void MainWindow::editBut_clicked()
{
}

void MainWindow::checkIsValide()
{
    QLineEdit *testEdit = qobject_cast<QLineEdit *>(sender());
    palette.setColor(QPalette::Base,Qt::white);
    if (!testEdit->hasAcceptableInput() && testEdit->text() != "" )
        palette.setColor(QPalette::Base,Qt::red);
    testEdit->setPalette(palette);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    if(ev->type()== QEvent::Wheel)
    {
        QComboBox* combo = qobject_cast<QComboBox*>(obj);
        if (combo && !combo->hasFocus())
        return true;
    }
    return false;
}

void MainWindow::on_actionPlot_File_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,"Open Instrument",instrumentDir);
    string cmd_filename = "testgnu.txt";
    ofstream f(cmd_filename.c_str());        //std::ofstream
    f <<
           "set terminal qt\n"
           // "unset pm3d\n"
           "set title \"Example\"\n"
           "set xlabel \"X coordinate\"\n"
           "set ylabel \"Y coordinate\"\n"
           "plot \"" << fileName.toStdString() << "\"\n";


     QProcess *gnuProc = new QProcess;
     //  connect(gnuProc,SIGNAL(error(QProcess::ProcessError)),this,SLOT(handleError(QProcess::ProcessError)));
     //  gnuProc->setArguments(QStringList() << "testgnu.txt");
     gnuProc->start("/bin/sh",QStringList() << "-c" << "gnuplot -p testgnu.txt");
}

void MainWindow::on_action2D_Plot_File_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,"Open Instrument",instrumentDir);
    string cmd_filename = "testgnu.txt";
    ofstream f(cmd_filename.c_str());
    f <<
         "set terminal qt\n"
         "set pm3d map\n"
         "set title \"" << fileName.toStdString() << "\"\n"
         "set xlabel \"X coordinate\"\n"
         "set ylabel \"Y coordinate\"\n"
         "splot \"" << fileName.toStdString() << "\"\n";

     QProcess *gnuProc = new QProcess;
     //  connect(gnuProc,SIGNAL(error(QProcess::ProcessError)),this,SLOT(handleError(QProcess::ProcessError)));
     //  gnuProc->setArguments(QStringList() << "testgnu.txt");
     gnuProc->start("/bin/sh",QStringList() << "-c" << "gnuplot -p testgnu.txt");

}
