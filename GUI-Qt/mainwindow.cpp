//=============================================================================
// File:    mainwindow.cpp
// Author:  Lydia Fleischhauer-Fuß <l.fleischhauer-fuss@fz-juelich.de>
// Date:    2021
// Purpose: General vitess application gui window to set all needed parameters,
//          save,load and start simulation
//=============================================================================

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTextStream>
#include <QScrollBar>
#include <QDesktopServices>
#include <QUrl>
#include <unistd.h>
#include <QDate>
#include <QPlainTextEdit>
#include <QThread>

using namespace YAML;
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    modultab(nullptr)
{
    ui->setupUi(this);

    //set logfile name
    #ifdef Q_OS_WIN
       syspar = ".exe";
       logFname = "C:/tmp/logfile";
    #elif defined (Q_OS_DARWIN)
       syspar = "_Darwin_x86_64";
       logFname = "/tmp/logfile";
    #elif defined (Q_OS_UNIX)
       syspar = "_Linux_x86_64";
       logFname = "/tmp/logfile";
    #else
    {
       QMessageBox::information(this,"Platform not get supported ",QSysInfo::kernelType());
    }
    #endif
    //react to event filter
    qApp->installEventFilter(this);

    while ( ui->stackedWidget->count() > 0 )
        ui->stackedWidget->removeWidget( ui->stackedWidget->widget(0) );
    ui->stackedWidget->hide();

    //get vitess directory
    VitessDir = QApplication::applicationDirPath().
               left(QApplication::applicationDirPath().lastIndexOf("/"));
    //If Application is under debug or release
    if (!QDir(  VitessDir+"/YAML/").exists())
        VitessDir = VitessDir.left(VitessDir.lastIndexOf("/"));
    instrumentInDir = VitessDir;
    instrumentOutDir = instrumentInDir;

    //get modul list from existing yaml definition files
    QDir directory (VitessDir+"/YAML/");
    QStringList modulList;
    fList = directory.entryList({"*.yaml"});
    for(int i=0; i<fList.count();i++ )
    {
       QString modulFile = directory.path()+"/"+fList[i];
       QFile file(modulFile);
       if (!file.open(QFile::ReadOnly | QFile::Text))
       {
        QMessageBox::warning(this,"Error in modul definition file",
                     "Cannot open file: ",modulFile);
       }
       else{

        //list of modulNames for comboBox in tableWidget
        //module name:  filename without extension
        modulList << fList[i].left(fList[i].lastIndexOf(".yaml"));

        //map key:modulname
        //    val:modul init filename , name of used c-module in pipe
        ModulFiles[modulList[i]] << modulFile;

        //map entry: modulparameter
        //    val: parameter definitions (type,description...)
        ModulParam.clear();
        //design gui for modul
        designModul(modulList[i]);
        ModulFiles[modulList[i]] << cModul;

        // map entry: modulname
        //       val: moduldesign
        ModulGui[modulList[i]] = scrollArea;

        //map entry: modulname
        //      val: map  entry: modulparameter
        //                val:   parameter definitions
        Module[modulList[i]] = ModulParam;

        //for later count of same modules
        modindex[modulList[i]] = 0;
      }
    }
    //table to select moduls
    modultab = new ModulTable(modulList, ui->modWidget);
    //set layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(modultab);
    layout->setContentsMargins(0,0,0,0);
    ui->modWidget->setLayout(layout);


    //Connect signals to slots
    //An arrow was pressed
    connect(modultab,SIGNAL(arrowPressed(int)),this,SLOT(showSelectedModul(int)));

    //Modul table value changed
    connect(modultab,SIGNAL(changedComboVal(QString,int)),this,
                     SLOT(changeModulWidget(QString,int)));

    //Remove module from modultable
    connect(modultab,SIGNAL(removeCombo(int)),this,SLOT(removeModule(int)));

    //Insert module in modultable
    connect(modultab,SIGNAL(insertCombo(int)),this,SLOT(insertModule(int)));

    //connect signals for menu action buffersize
    foreach(QAction * act, ui->menunBuffer->actions())
        connect(act,SIGNAL(triggered()),this,SLOT(BufferSize_triggered()));
    nBuffer = "50000";          //default setting

    //connect signals for min. neutron weight
    foreach(QAction * act, ui->menuMinWght->actions())
        connect(act,SIGNAL(triggered()),this,SLOT(minNeutWeight_triggered()));
    MinWght = "0.0";

    ui->RndSeed->setValidator(new QDoubleValidator);

    //signals help menu
    connect(ui->actionHelpGenerate_Series,SIGNAL(triggered()),this,SLOT(helpTools_triggered()));
    foreach(QAction * act, ui->menuHelpTools->actions())
        connect(act,SIGNAL(triggered()),this,SLOT(helpTools_triggered()));
    foreach(QString str, helpModul)
    {
        QAction *act = new QAction(str);
        act->setObjectName(str);
        ui->menuHelp->addAction(act);
        connect(act,SIGNAL(triggered()),this,SLOT(helpModules_triggered()));
    }

    //timer used for progress in visualization run
    tVisual = new QTimer(this);
    connect(tVisual, SIGNAL(timeout()),this,SLOT(visualActive()));

    //Dialog for big output window
    bigOutput = new Big();
    connect(bigOutput,SIGNAL(small()),
                         this,SLOT(showTextBrowser()));
    connect(bigOutput,SIGNAL(clear()),
                         this,SLOT(on_pushClear_clicked()));
    connect(this,SIGNAL(big(QString)),
                         bigOutput,SLOT(setBrowserText(QString)));

    //Dialog for generating series
    series = new Series();
    connect(this,SIGNAL(getSeriesVariable(QString,QString)),
                         series,SLOT(setSeriesVariable(QString,QString)));
    //start serie
    connect(series,SIGNAL(startSeries(QStringList, QVector<QVector<QString>>,
                                      QStringList, QStringList, QString )),
            this,SLOT(startSerie(QStringList, QVector<QVector<QString>>,
                                      QStringList, QStringList, QString )));
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
    //change modul in stackWidget
    if (row != ui->stackedWidget->count())
    {
        modindex[ui->stackedWidget->widget(row)->objectName()]--;
        ui->stackedWidget->removeWidget(ui->stackedWidget->widget(row));
        //modul first time in stackWidget
        if (modindex[modul] == 0)
            ui->stackedWidget->insertWidget(row,ModulGui[modul]);
        else    //modul repeated in stackWidget
        {
            designModul(modul);
            ui->stackedWidget->insertWidget(row,scrollArea);
        }
        ui->stackedWidget->setCurrentIndex(row);
    }
    //add modul in stackWidget
    else {
        if (modindex[modul] == 0)
            ui->stackedWidget->addWidget(ModulGui[modul]);
        else
        {
            designModul(modul);
            ui->stackedWidget->addWidget(scrollArea);
        }
        ui->stackedWidget->setCurrentIndex(ui->stackedWidget->count()-1);
    }
    modindex[modul]++;
    int curInd = ui->stackedWidget->currentIndex();

    //set header label to modul number and name
    QLabel *headerLab = ui->stackedWidget->widget(curInd)->findChild<QLabel*>("headerLabel");
    headerLab->setText("<b>Modul " + QString::number(row+1) +"  " +
                                   ui->stackedWidget->widget(curInd)->objectName() +"</b>");
    ui->stackedWidget->show();
}


//Remove module from module table
void MainWindow::removeModule(int row)
{
    ui->stackedWidget->hide();
    modindex[ui->stackedWidget->widget(row)->objectName()]--;
    //remove widget
    ui->stackedWidget->removeWidget(ui->stackedWidget->widget(row));
    ui->stackedWidget->setCurrentIndex(row);
    //change number in headerLabel
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

//Menue file:  load instrument
void MainWindow::on_actionLoad_triggered()
{

   QString fName = QFileDialog::getOpenFileName(this,"Open Instrument",instrumentInDir,
                                                 tr("*.yml (*.yml)"));
   if (fName != "") loadInstrument(fName);
}

//Menu file:  save instrument
void MainWindow::on_actionSave_triggered()
{
    if (instrumentFile == "")                          //no filename set
    {
        QString instName = QFileDialog::getSaveFileName(this,"Save Instrument",instrumentInDir,
                                                       "YML(*.yml) (*.yml)");
   //change if Gtk-Message: mapped without a transient parent
   //     QString instName = QFileDialog::getSaveFileName(this,"Save Instrument",instrumentInDir,
   //                                                    "YML(*.yml) (*.yml)", nullptr,
   //                                                    QFileDialog::DontUseNativeDialog);
        if (instName.isEmpty())return;
        else instrumentFile = instName;
        //filename without extension
        if (!instrumentFile.endsWith(".yml")) instrumentFile += ".yml";
    }
    saveFile(instrumentFile);
}

//Menu save instrument as
void MainWindow::on_actionSave_as_triggered()
{

    QString instName = QFileDialog::getSaveFileName(this,"Save Instrument as",instrumentInDir,
                                                  tr("YML(*.yml) (*.yml)"));
    if (instName.isEmpty())return;
    else instrumentFile = instName;
    if (!instrumentFile.endsWith(".yml")) instrumentFile += ".yml";
    saveFile(instrumentFile);
    ui->textBrowser->append("Instrument is saved as: " +instrumentFile);
    if (bigOutput->isVisible()) emit big(ui->textBrowser->toPlainText());
}


//Menu new instrument
void MainWindow::on_actionNewInst_triggered()
{

    ui->pushFresh->clicked();
    ui->textBrowser->clear();
    if (bigOutput->isVisible()) emit big(ui->textBrowser->toPlainText());
}

//Menu export as
void MainWindow::on_actionPy_Python_script_triggered()
{
    ui->pushCheck->clicked();
    pythonScript(instrumentOutDir, cmdList, logFname);
}

void MainWindow::on_actionBat_shell_triggered()
{
    ui->pushCheck->clicked();
    shellScript(instrumentOutDir, cmdList, logFname);
}

//Menu show *.inf file
void MainWindow::on_actionShow_inf_File_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,"Open inf file",
                                                      instrumentOutDir,tr("INF (*.inf)"));
    if(fileName.isEmpty()) return;
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this,"Show inf file","Warning cannot open file: ",fileName);
        return;
    }
    QPlainTextEdit* textEdit = new QPlainTextEdit();
    textEdit->resize(700,350);
    textEdit->setPlainText(file.readAll());
    textEdit->show();
}

//Menu exit
void MainWindow::on_actionExit_triggered()
{
    QApplication::closeAllWindows();
}

//Variations
//Menu copy modul parameter
void MainWindow::on_actionCopy_Module_Parameters_triggered()
{
    curModul.reset();
    readCurModul(curModul,ui->stackedWidget->currentIndex());
}

//Menu paste modul parameter
void MainWindow::on_actionPaste_Module_Parameters_triggered()
{
    int index = ui->stackedWidget->currentIndex();
    string key = ui->stackedWidget->widget(index)->objectName().toStdString();       //std::string
    if (curModul.begin()->first.as<string>() == key)
        pasteCurModul(curModul[key],index);
}

//Menu set instrument name
void MainWindow::on_actionSet_Instrument_Name_triggered()
{
    instrumentFile = QFileDialog::getSaveFileName(this,"Set Instrumentname",instrumentInDir,
                                                  tr("Files (*.yml)"));
    if (instrumentFile.isEmpty()) return;
    if (!instrumentFile.endsWith(".yml")) instrumentFile += ".yml";
    QFile file(instrumentFile);
    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox::warning(this,"set Instrumentname",
                             "Warning cannot open file: ",instrumentFile);
        return;
    }
    QFileInfo fileinfo(instrumentFile);
    ui->InstName->setText(fileinfo.baseName());
    //set new working dir
    instrumentInDir=fileinfo.path();
    ui->InDir->setText(instrumentInDir);

}

//Menu Tools
void MainWindow::on_actionConvert_Ascii_to_Binary_triggered()
{
    toolCommand("ascii2bin");
}

void MainWindow::on_actionDefine_Direction_triggered()
{
    toolCommand("define_direction");
}

void MainWindow::on_actionGenerate_Mirror_Files_triggered()
{
    toolCommand("mirror_coating");
}
void MainWindow::on_actionGenerate_Surface_Files_triggered()
{
    toolCommand("surface_file");
}

void MainWindow::on_actionGenerate_Extraction_System_triggered()
{
    toolCommand("gener_bispectral");
}

void MainWindow::on_actionGuide_Shape_triggered()
{
    toolCommand("guide_shape");
}

void MainWindow::on_actionCryst_Analayzer_Spectrom_triggered()
{
    QStringList modulSpec;
    modulSpec << VitessDir << syspar << instrumentOutDir;
    Chrystanalyzer *analyzerWin = new Chrystanalyzer(modulSpec);
    analyzerWin->show();
}

void MainWindow::on_actionCompute_Chopper_Phases_triggered()
{
    QString cmd = VitessDir + "/MODULES/chop_phases" + syspar;
    cmd += " -o" +logFname.left(logFname.lastIndexOf("/")) + "/chop_phases";
    ChopperPhases *chopperPhases = new ChopperPhases(cmd);
    chopperPhases->show();
}

//Options
//Set buffer size
void MainWindow::BufferSize_triggered()
{
      foreach (QAction* act,ui->menunBuffer->actions())
        act->setChecked(false);
      this->findChild<QAction *>(sender()->objectName())->setChecked(true);
      nBuffer = this->findChild<QAction *>(sender()->objectName())->text();
}

//Set min. neutron weight
void MainWindow::minNeutWeight_triggered()
{
    foreach (QAction* act,ui->menuMinWght->actions())
        act->setChecked(false);
    this->findChild<QAction *>(sender()->objectName())->setChecked(true);
    MinWght = this->findChild<QAction *>(sender()->objectName())->text();
}

//Help menu
//Action general information
void MainWindow::on_actionGeneral_Information_triggered()
{
    //open seperat help dialog
    Help *help_general = new Help(this);
    help_general->defaultHelp();
    help_general->show();
}

//Action tutorial
void MainWindow::on_actionTutorial_triggered()
{
    //open pdf in webbrowser
    QDesktopServices::openUrl(QUrl(VitessDir+"/WWW/tutorial.pdf"));     // tutorial.pdf
}

//Action user interface
void MainWindow::on_actionUser_Interface_triggered()
{
    Help *help_general = new Help(this);
    help_general->guiHelp();
    help_general->show();
}

//Action Visualization
void MainWindow::on_actionVisualization_triggered()
{
    QDesktopServices::openUrl(QUrl(VitessDir + "/WWW/visual.html"));
}

//Action optimization
void MainWindow::on_actionOptimization_triggered()
{
    QDesktopServices::openUrl(QUrl(VitessDir+"/WWW/Optimization.pdf"));
}

//Action generate series, tools
void MainWindow::helpTools_triggered()
{
     QString text = this->findChild<QAction *>(sender()->objectName())->text();
     //map helptools defined in tools.h
     QDesktopServices::openUrl(QUrl(VitessDir + "/WWW/" + helpTools[text] + ".html"));
}

//Action different moduls
void MainWindow::helpModules_triggered()
{
     QString text = sender()->objectName();
     if (!QDesktopServices::openUrl(QUrl("file:///"+ VitessDir + "/WWW/" + text + ".html")))
         QMessageBox::warning(this,"Help file",
                              "Warning cannot open helpfile for modul: ",text);
}


//Button check       prepare pipe
void MainWindow::on_pushCheck_clicked()
{
    //generate modul pipe string for enabled modules
    QString headerStr, cmd;
    QTextStream header(&headerStr);
    cmdList.clear();
    //get global Parameter
    getHeader(header);
    ui->textBrowser->append("Pipe will be:");
    int enableIndex = 0;
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
        if (!modultab->disableFlag[i])
        {
            allLineEdits.clear();
            allComboBoxes.clear();

            //create first part of pipe string with c-module and global parameters
            cmd = VitessDir + "/MODULES/";
            QString modulName = ui->stackedWidget->widget(i)->objectName();

            //ModuleFiles  key:modulename  value: definition filename,c-module
            cmd += ModulFiles[modulName][1].toLower() + syspar;
            cmd += " --N" + QString::number(i+1);           //Modnum   number of modul
            cmd += headerStr;                               //global parameter
            cmd += " --L"+logFname + QString::number(enableIndex+1);  //logfile name


            //second part in pipe string with all parameters for enabled moduls that have prefix
            //and are not empty
            foreach(QString param, Module[modulName].keys())
            {
                //prefix for parameter ist set
                if (Module[modulName][param]["prefix"] != "")             //prefix
                {
                    //read lineedit text
                    if ((ui->stackedWidget->widget(i)->findChild< QLineEdit *>(param)) &&
                       (ui->stackedWidget->widget(i)->findChild< QLineEdit *>(param)->text() != ""))
                    {
                       cmd += " " + Module[modulName][param]["prefix"];
                       cmd += ui->stackedWidget->widget(i)->findChild< QLineEdit *>(param)->text();
                    }
                    //get combobox text and convert
                    else if (ui->stackedWidget->widget(i)->findChild< QComboBox *>(param))
                    {
                       cmd += " " + Module[modulName][param]["prefix"];
                       QString curText = ui->stackedWidget->widget(i)
                               ->findChild< QComboBox *>(param)->currentText();
                       functionMap[param](curText.toStdString().c_str());
                       cmd += QString::number(functionMap[param](curText.toStdString().c_str()));
                    }
                    //get checkbox value
                    else if (ui->stackedWidget->widget(i)->findChild< QCheckBox *>(param))
                    {
                       cmd += " " + Module[modulName][param]["prefix"];
                       cmd += QString::number(ui->stackedWidget->widget(i)
                                       ->findChild< QCheckBox *>(param)->isChecked());
                    }
                }
                //no prefix set, get values from subparameter file if set
                else if ((ui->stackedWidget->widget(i)->findChild< QPushButton *>(param)) &&
                     (ui->stackedWidget->widget(i)
                        ->findChild< QLineEdit *>(param.toLower()+"_file")->text() != ""))
                {
                    QString fileName = ui->stackedWidget->widget(i)
                          ->findChild< QLineEdit *>(param.toLower()+"_file")->text();
                    fileName = instrumentOutDir+"/"+fileName;
                    QFile file(fileName);
                    if (!file.open(QFile::ReadOnly | QFile::Text))
                    {
                      QMessageBox::warning(this,"Parameter file",
                                           "Warning cannot open file: ",fileName);
                      return;
                    }
                    //read subparameter file, contains values from separate parameterwindow
                    config = YAML::LoadFile(fileName.toStdString());
                    YAML::Node config_paramWin = config[config.begin()->first.as<string>()];
                    for (unsigned i=0; i < config_paramWin.size(); i++)
                    {
                      for(YAML::const_iterator it=config_paramWin[i].begin(); it!=config_paramWin[i].end(); ++it)
                      {
                        QString paramKey = QString::fromStdString(it->first.as<std::string>());      //key
                        QString paramVal = QString::fromStdString(it->second.as<std::string>());      //value
                        QString prefix = paramWindow[param]->mapModule[paramKey]["prefix"];
                        if ( paramWindow[param]->findChild<QComboBox *>(paramKey) &&
                             paramWindow[param]->mapModule[paramKey]["index"]!="")
                            paramVal = paramWindow[param]->mapModule[paramKey]["index"][paramVal.toInt()];
                        if (prefix != "")
                            cmd += " " + prefix[0] + QString::number(i) + prefix[1] + paramVal;
                      }
                    }
                }
            }
            //append command string to stringlist with entry per enabled modul
            cmdList.append(cmd);
            if (i < ui->stackedWidget->count()-1) cmd += " | ";
            //write pipe string to textbrowser
            ui->textBrowser->append(cmd);
            ui->textBrowser->verticalScrollBar()->
                   setValue(ui->textBrowser->verticalScrollBar()->maximum());
            if (bigOutput->isVisible()) emit big(ui->textBrowser->toPlainText());
            enableIndex++;
        } //not disabled
    } //loop stackedWidget count
}


//Button Dryrun
void MainWindow::on_pushDryrun_clicked()
{
// Start a dry run.
// A dry run is a pipe execution with few neutron trajectories.
    ui->pushCheck->clicked();

    // first module should be a source module
    if (cmdList[0].indexOf("source") < 0)
    {
        ui->textBrowser->setTextColor(Qt::red);
        ui->textBrowser->append("First module should be a source module");
        ui->textBrowser->setTextColor(Qt::black);
        if (bigOutput->isVisible()) emit big(ui->textBrowser->toPlainText());
        return;
    }
    // change number of neutron trajectories  -n to 100000
    if (cmdList[0].contains(QRegExp("-n[0-9]+e\\+?[0-9]+")))
        cmdList[0].replace(QRegExp("-n[0-9]+e\\+?[0-9]+"), "-n100000");
    else if (cmdList[0].contains(QRegExp("-n[0-9]+")))
        cmdList[0].replace(QRegExp("-n[0-9]+"), "-n100000") ;

    startPipe();
}


//Button Start
void MainWindow::on_pushStart_clicked()
{
    // get pipe string
    ui->pushCheck->clicked();

    // first module should be a source or read_in module
    if ((cmdList[0].indexOf("source") < 0) & (cmdList[0].indexOf("read_in_") < 0))
    {
        ui->textBrowser->setTextColor(Qt::red);
        ui->textBrowser->append("Please specify an input file, if the first module\ndoes not generate simulated neutrons");
        ui->textBrowser->setTextColor(Qt::black);
        if (bigOutput->isVisible()) emit big(ui->textBrowser->toPlainText());
        return;
    }
    startPipe();
}


//Button Visualization
void MainWindow::on_pushVisual_clicked()
{
    ui->pushCheck->clicked();
    // first module should be a source module
    if (cmdList[0].indexOf("source") < 0)
    {
        ui->textBrowser->setTextColor(Qt::red);
        ui->textBrowser->append("First module should be a source module");
        ui->textBrowser->setTextColor(Qt::black);
        if (bigOutput->isVisible()) emit big(ui->textBrowser->toPlainText());
        return;
    }
    if (cmdList[0].contains(QRegExp("-n[0-9]+e\\+?[0-9]+")))
        cmdList[0].replace(QRegExp("-n[0-9]+e\\+?[0-9]+"), "-n10000");
    else if (cmdList[0].contains(QRegExp("-n[0-9]+")))
        cmdList[0].replace(QRegExp("-n[0-9]+"), "-n10000") ;
    ui->textBrowser->setTextColor(Qt::blue);
    ui->textBrowser->append("Set Number of trajections to 10000\n");
    ui->textBrowser->setTextColor(Qt::black);
    visualRepete = 0;
    //start timer to repete test of visual simulation runs to be ready
    //without blocking program
    tVisual->start(100);
}

//Stop/terminate running processes
void MainWindow::on_pushStop_clicked()
{
    //end processes without new trajections
    ui->textBrowser->setTextColor(Qt::red);
    for (int i=0; i<procList.count(); i++)
        if (procList[i]->state() > 0)
        {
            procList[i]->terminate();
            sleep(1);
            ui->textBrowser->append( "Module: " + QString::number(i) + " stopped;");
        }
    ui->textBrowser->setTextColor(Qt::black);
    if (bigOutput->isVisible()) emit big(ui->textBrowser->toPlainText());
}


//Kill running processes
void MainWindow::on_pushKill_clicked()
{
    //kill processes immediately
    ui->textBrowser->setTextColor(Qt::red);
    for (int i=0; i<procList.count(); i++)
       if (procList[i]->state() > 0)
       {
           procList[i]->kill();
           ui->textBrowser->append( "Module: " + QString::number(i) + " killed;");
       }
    ui->textBrowser->setTextColor(Qt::black);
    if (bigOutput->isVisible()) emit big(ui->textBrowser->toPlainText());
    killBit = true;
}


//Get input directory
void MainWindow::on_pushIndir_clicked()
{
    QString InDir = QFileDialog::getExistingDirectory(this,"Set input directory",
                                 VitessDir,QFileDialog::ShowDirsOnly);
    //change if Gtk-Message: mapped without a transient parent
    //                             VitessDir,QFileDialog::ShowDirsOnly |
    //                             QFileDialog::DontUseNativeDialog);
    if (InDir != "")ui->InDir->setText(InDir);

}

//Get output directory
void MainWindow::on_pushOutdir_clicked()
{
    QString OutDir = QFileDialog::getExistingDirectory(this,"Set output directory",
                                  VitessDir,QFileDialog::ShowDirsOnly);
    if (OutDir != "") ui->OutDir->setText(OutDir);

}

void MainWindow::on_InDir_editingFinished()
{
    QDir path(ui->InDir->text());
    if (!path.exists())
    {
        QMessageBox::warning(this,"Set input directory",
                             "Directory does not exist: " + ui->InDir->text());
        ui->InDir->setText(VitessDir);
    }
    instrumentInDir = ui->InDir->text();
}

void MainWindow::on_OutDir_editingFinished()
{
    QDir path(ui->OutDir->text());
    if (!path.exists())
    {
        QMessageBox::warning(this,"Set output directory",
                             "Output Directory does not exist: " + ui->OutDir->text());
        ui->OutDir->setText(ui->InDir->text());
    }
    instrumentOutDir = ui->OutDir->text();
}


//Button New  Fresh modul table
void MainWindow::on_pushFresh_clicked()
{
    modultab->cleanModules();
    while ( ui->stackedWidget->count() > 0 )
         ui->stackedWidget->removeWidget( ui->stackedWidget->widget(0) );
    instrumentFile = "";
    ui->InstName->setText(instrumentFile);
}

//Button Big   TextBrowser
void MainWindow::on_pushBig_clicked()
{
    ui->outputWidget->hide();
    emit big(ui->textBrowser->toPlainText());
    bigOutput->show();
}


//Button Clear  TextBrowser
void MainWindow::on_pushClear_clicked()
{
    ui->textBrowser->clear();
}


//Button Save   TextBrowser
void MainWindow::on_pushSave_clicked()
{
    //save textbrowser content to file
    QString logFile = QFileDialog::getSaveFileName(this,"Save logfile as",instrumentOutDir,
                                                   tr("Files (*.*)"));
    if (logFile.isEmpty()) return;
    QFile file(logFile);
    if (!file.open(QFile::WriteOnly | QFile::Text))        //open file
    {
        QMessageBox::warning(this,"Save Output","Warning cannot open logfile: ",logFile);
        return;
    }
    ofstream fout(logFile.toStdString());          // std::ofstream
    fout << ui->textBrowser->toPlainText().toStdString();
    file.close();
}


//Subparameter new window
void MainWindow::paramBut_clicked()
{
    //show parameter subwindow
    QString param = qobject_cast<QPushButton *>(sender())->text();
    QString fileName = ui->stackedWidget->currentWidget()
                         ->findChild< QLineEdit *>(param.toLower()+"_file")->text();
    if (fileName != "")
    {
       fileName = instrumentOutDir+"/"+fileName;
       QFile file(fileName);
       if (!file.open(QFile::ReadOnly | QFile::Text))
       {
           QMessageBox::warning(this,"Parameter file","Warning cannot open file: ",fileName);
           return;
       }
       paramWindow[param]->loadFile(fileName);
    }
    paramWindow[param]->instInDir=instrumentInDir;
    paramWindow[param]->instOutDir=instrumentOutDir;
    paramWindow[param]->show();
}

//Parameter subwindow values saved, put in filename
void MainWindow::changeParamWidget(QString filename,QString initName)
{
    //set filename from subparameter into matching lineEdit
    QFileInfo fileinfo(filename);
    int curInd = ui->stackedWidget->currentIndex();
    ui->stackedWidget->widget(curInd)->findChild<QLineEdit*>(initName + "_file")->setText(fileinfo.fileName());
}


//Browse files
void MainWindow::browseBut_clicked()
{
    //get filename
    QString fileName = QFileDialog::getOpenFileName(this,"Open Instrument",instrumentInDir);
    //change if Gtk-Message: mapped without a transient parent
    //QString fileName = QFileDialog::getOpenFileName(this,"Open Instrument",instrumentDir,
    //                                        QString(),nullptr,QFileDialog::DontUseNativeDialog);
    if (fileName == "") return;
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this,"Browse file","Warning cannot open file: ",fileName);
        return;
    }
    QFileInfo fileinfo(fileName);
    // cut browse_ from sender
    QString str = qobject_cast<QPushButton *>(sender())->objectName().mid(7);
    if (ui->stackedWidget->currentWidget()->findChild<QLineEdit *>(str))
          ui->stackedWidget->currentWidget()->findChild<QLineEdit *>(str)
                         ->setText(fileinfo.fileName());
    else ui->stackedWidget->currentWidget()->findChild<QLineEdit *>(str.toLower()+"_file")
                         ->setText(fileName);
}


//Show files
void MainWindow::editBut_clicked()
{
     QString str = qobject_cast<QPushButton *>(sender())->objectName().mid(5);
     QString fileName = ui->stackedWidget->currentWidget()->findChild<QLineEdit *>(str)->text();
     if (fileName == "") return;
     QFile file(instrumentInDir+"/"+fileName);
     if (!file.open(QFile::ReadOnly | QFile::Text))
     {
         QMessageBox::warning(this,"Show file","Warning cannot open file: ",instrumentInDir+"/"+fileName);
         return;
     }
     QPlainTextEdit* textEdit = new QPlainTextEdit();
     textEdit->resize(700,350);
     textEdit->setPlainText(file.readAll());
     textEdit->show();
}

//Design gui from definition files
void MainWindow::designModul(QString modulName)
{
    //create page of stackedWidget for modul
    scrollArea = new QScrollArea;
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    QWidget *modulWindow = new QWidget;
    gridLayout = new QGridLayout;
    modulWindow->setLayout(gridLayout);
    scrollArea->setWidget(modulWindow);
    scrollArea->setObjectName(modulName);
    for(int col=0; col<3; col++)  gridLayout->setColumnMinimumWidth(col,230);
    scrollArea->setWidgetResizable(true);
    //load modul yaml configuration file
    YAML::Node config = YAML::LoadFile(ModulFiles[modulName][0].toStdString());
    // set used c-modul
    cModul = QString::fromStdString(config.begin()->first.as<string>());
    getModulParameter(config, modulName);

}


void MainWindow::getModulParameter(YAML::Node& configParam,QString modulName)
{
    //create page of stackedWidget for given modul

    int iGritRow = 0;
    int index = 0;
    QLabel *headerLabel = new QLabel("<b>" +  modulName + "</b>\n");
    headerLabel->setObjectName("headerLabel");
    gridLayout->addWidget(headerLabel,iGritRow+1,0,1,3,Qt::AlignHCenter);
    iGritRow+=2;

    //get parameter
    YAML::Node configParameter = configParam[configParam.begin()->first.as<string>()];
    //loop all modul parameters
    for(unsigned int ipipe = 0; ipipe < configParameter.size(); ipipe++)
    {
        for(YAML::const_iterator it=configParameter[ipipe].begin(); it!=configParameter[ipipe].end(); ++it)
        {
            //get parameter name
            QString parName = QString::fromStdString(it->first.as<string>());

            //fill mapParam (defined in tools.h),
            //map of single parameter definition keys: type,descr,default,min,max,column,prefix
            //with values from modul definition file
            //definitions of one parameter
            YAML::Node configParamDef = it->second;
            foreach(QString key,mapParam.keys())
            {
              if (configParamDef[key.toStdString()])
                  if ( configParamDef[key.toStdString()].size() > 1)    //list
                  {
                     strList.clear();
                     for(int i=0; i<static_cast<int>(configParamDef[key.toStdString()].size()); i++)
                        strList << QString::fromStdString(configParamDef[key.toStdString()][i].as<string>());
                     mapParam[key] = strList.join(",");
                  }else
                     mapParam[key] = QString::fromStdString(configParamDef[key.toStdString()].as<string>());
              else mapParam[key] = "";
            }
            ModulParam[parName] = mapParam;     //map for one parameter

            //put matching parameter entry in gridLayout
            getWidgetDesign(parName,mapParam, gridLayout,iGritRow,index);

            //for file parameter connect belonging buttons
            if( scrollArea->widget()->findChild<QPushButton*>("browse_" + parName))
                connect(scrollArea->widget()->findChild<QPushButton*>("browse_" + parName),
                        SIGNAL(clicked()),this,SLOT(browseBut_clicked()));
            if( scrollArea->widget()->findChild<QPushButton*>("edit_" + parName))
                connect(scrollArea->widget()->findChild<QPushButton*>("edit_" + parName),
                        SIGNAL(clicked()),this,SLOT(editBut_clicked()));

            //for sub parameter create separate widget Parameter class, get values from matching file
            //under /YAML/parameter/<parameter>.yaml. Design parameter window.
            if( scrollArea->widget()->findChild<QPushButton*>(parName))
            {
                //create widget for subparameter
                paramWindow[parName] = new Parameter();
                paramWindow[parName]->setWindowModality(Qt::ApplicationModal);
                //get configuration yaml file for subparameter in subdirectory
                QString yamlPath = VitessDir + "/YAML/parameter/";
                QString filename = yamlPath + parName.toLower() + ".yaml";
                //design subwidget
                paramWindow[parName]->designParameterWin(filename);
                //connect parameter window save button
                connect(paramWindow[parName],SIGNAL(changedParamWidget(QString,QString)),
                                     this,SLOT(changeParamWidget(QString,QString)));
                //connect parameter pushButton
                connect(scrollArea->widget()->findChild<QPushButton*>(parName),
                        SIGNAL(clicked()),this,SLOT(paramBut_clicked()));
             }
             //check valide input for lineEdits
             else if( scrollArea->widget()->findChild<QLineEdit*>(parName))
                connect(scrollArea->widget()->findChild<QLineEdit*>(parName),
                        SIGNAL(textChanged(const QString &)),this,SLOT(checkIsValide()));
        }
    }
    //insert spacer if rowcount in grid less than 15
    if (iGritRow <= 15)
    {
        iGritRow++;
        gridLayout->addItem( new QSpacerItem(20,40,QSizePolicy::Minimum,QSizePolicy::Expanding),iGritRow,0);
    }
}


void MainWindow::showTextBrowser()
{
    ui->outputWidget->show();
}



void MainWindow::readCurModul(YAML::Node& curModule,int index)
{
    allLineEdits.clear();
    allComboBoxes.clear();
    allCheckBoxes.clear();
    allPushButtons.clear();

    //get module name and gui entries from stackedWidget
    string key = ui->stackedWidget->widget(index)->objectName().toStdString();       //std::string
    allLineEdits <<  ui->stackedWidget->widget(index)->findChildren< QLineEdit *>();
    allComboBoxes <<  ui->stackedWidget->widget(index)->findChildren< QComboBox *>();
    allCheckBoxes <<  ui->stackedWidget->widget(index)->findChildren< QCheckBox *>();
    allPushButtons << ui->stackedWidget->widget(index)->findChildren< QPushButton *>();

    //get values and store in yaml node curModule
    for(int ii=0 ; ii < allLineEdits.size(); ii++)
        if (allLineEdits[ii]->text() != "" && !allLineEdits[ii]->objectName().endsWith("_file"))
           curModule[key][allLineEdits[ii]->objectName().toStdString()] =
                                      allLineEdits[ii]->text().toStdString();
    for(int ii=0 ; ii<allComboBoxes.size(); ii++)
        curModule[key][allComboBoxes[ii]->objectName().toStdString()] =
                                      allComboBoxes[ii]->currentText().toStdString();
    for(int ii=0 ; ii<allCheckBoxes.size(); ii++)
        curModule[key][allCheckBoxes[ii]->objectName().toStdString()] =
                                      allCheckBoxes[ii]->isChecked();
    for(int ii=0 ; ii<allPushButtons.size(); ii++)
    {
        QString butName = allPushButtons[ii]->objectName();
        //get subparameter values
        if(paramWindow.keys().indexOf(butName) != -1)
        {
           QString parFile = ui->stackedWidget->widget(index)
                  ->findChild<QLineEdit *>(butName.toLower()+"_file")->text();
           if( parFile != "")
           {
              parFile = instrumentOutDir + "/" +parFile;
              paramWindow[butName]->saveData(config,key,butName,parFile);
           }
        }
    }
}


void MainWindow::pasteCurModul(YAML::Node curModule,int index)
{
    //get entries and values from yaml node and sort into current stackedWidget
    for(YAML::const_iterator it=curModule.begin(); it!=curModule.end(); ++it)
    {
        QString childName = QString::fromStdString(it->first.as<string>());      //key
        QWidget *modulWidget = ui->stackedWidget->widget(index);
        if (modulWidget->findChild<QLineEdit *>(childName))
            modulWidget->findChild<QLineEdit *>(childName)
                       ->setText(QString::fromStdString(it->second.as<string>()));
        else if (modulWidget->findChild<QComboBox *>(childName))
                 modulWidget->findChild<QComboBox *>(childName)
                            ->setCurrentText(QString::fromStdString(it->second.as<string>()));
        else if (modulWidget->findChild<QCheckBox *>(childName))
                 modulWidget->findChild<QCheckBox *>(childName)
                            ->setChecked(it->second.as<bool>());
        else if (modulWidget->findChild<QPushButton *>(childName))
        {
            //write subparameter yaml data to seperat file,that will be opened when
            //button is pressed
            QString fileName = instrumentOutDir+"/"+childName.toLower()+".yml";
            QFile file(fileName);
            if (!file.open(QFile::ReadWrite | QFile::Text))
            {
                QMessageBox::warning(this,"Warning cannot open parameter file: ",fileName);
                return;
            }
            //stream to write to file
            ofstream fout(fileName.toStdString());       // using namespace std
            YAML::Node paramWin;
            paramWin[childName.toStdString()] =
                       configChildren[childName.toStdString()];
            fout << paramWin;
            file.close();
            //set filename to lineEdit
            QFileInfo fileinfo(fileName);
            modulWidget->findChild< QLineEdit *>(childName.toLower()+"_file")
                       ->setText(fileinfo.fileName());
        }
    }
}

//load and save instrumentfile
void MainWindow::loadInstrument(QString fName)
{
    QFile file(fName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this,"Load Instrumnet","Warning cannot open file: ",fName);
        return;
    }
    instrumentFile = fName;
    QFileInfo fileinfo(instrumentFile);

    ui->InstName->setText(fileinfo.baseName());
    //set new working dir
    instrumentInDir = fileinfo.path();
    instrumentOutDir = instrumentInDir;
    ui->InDir->setText(instrumentInDir);
    ui->OutDir->setText(instrumentInDir);

    //clean modul table and stackedWidget
    modultab->cleanModules();
    while ( ui->stackedWidget->count() > 0 )
         ui->stackedWidget->removeWidget( ui->stackedWidget->widget(0) );

    //get yaml instrument configuration
    YAML::Node pipe = YAML::LoadFile(instrumentFile.toStdString());

    config = pipe[pipe.begin()->first.as<string>()];

    for(unsigned int ipipe = 0; ipipe < config.size();ipipe++)
    {
      for(YAML::const_iterator it=config[ipipe].begin(); it!=config[ipipe].end(); ++it)
      {
        configChildren.reset();
        configChildren = it->second;
        //get module name
        QString module = QString::fromStdString(it->first.as<string>());
        if (module == "GlobalParameters")
           loadHeader(configChildren);
        else
        {
            //put module in table, this sends signal changedComboVal and
            //slot changeModulWidget is executed
            if (configChildren["disabled"])
                modultab->disableFlag[int(ipipe-1)]=true;
            modultab->loadModule(module);
            //put values in gui
            pasteCurModul(configChildren, ui->stackedWidget->count()-1);
        }
      }
    }
    if (pipe["Serie"])
    {
        config = pipe["Serie"];
        series->loadSerieVariablen(config);
    }
    file.close();
    //check disableFlag
    modultab->setDisabled();
    //to do: error case
    ui->textBrowser->setText("Successfully loaded intrument:  "+fileinfo.baseName());
    if (bigOutput->isVisible()) emit big(ui->textBrowser->toPlainText());

}


void MainWindow::loadHeader(YAML::Node& nodeGlobal)
{
    QString childName;
    //set global values in gui
    for(YAML::const_iterator iter=nodeGlobal.begin(); iter!=nodeGlobal.end(); ++iter)
    {
        childName = QString::fromStdString(iter->first.as<string>());      //key
        if (this->findChild<QLineEdit *>(childName))
            this->findChild<QLineEdit *>(childName)
                ->setText( QString::fromStdString(iter->second.as<string>()));
        else if(this->findChild<QComboBox *>(childName))
            this->findChild<QComboBox *>(childName)
                ->setCurrentText(QString::fromStdString(iter->second.as<string>()));
        //set menu entries for buffersize and minimal neutron weight
        else if((childName == "MinWght") | (childName == "nBuffer"))
        {
            if (childName == "MinWght")
            {
                MinWght = QString::fromStdString(iter->second.as<string>());
                str = MinWght;
                str.replace(QRegularExpression("[.|-]+"),"_");
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
    if (ui->InDir->text()!="") instrumentInDir = ui->InDir->text();
    else ui->InDir->setText(instrumentInDir);
    if (ui->OutDir->text()!="") instrumentOutDir = ui->OutDir->text();
    else
    {
        ui->OutDir->setText(instrumentInDir);
        instrumentOutDir = instrumentInDir;
    }
}


//Read global header
void MainWindow::getHeader(QTextStream& out)
{

    //get global values and the matching prefixes from map
    foreach(QString entry, mapHeader.keys())
    {
        out << " " << mapHeader[entry];
        if (this->findChild<QLineEdit *>(entry))
            out << this->findChild<QLineEdit *>(entry)->text();
        else if (this->findChild<QComboBox *>(entry))
            out << this->findChild<QComboBox *>(entry)->currentIndex();
        else if(entry == "MinWght") out << MinWght;
        else if(entry == "nBuffer") out << nBuffer;
    }
    out << " ";
}


void MainWindow::writeHeader(YAML::Node& config)
{
    string gPara = "GlobalParameters";
    config[gPara]["RndSeed"] = ui->RndSeed->text().toStdString();
    config[gPara]["bGravity"] = ui->bGravity->currentText().toStdString();
    config[gPara]["nBuffer"] = nBuffer.toStdString();
    config[gPara]["MinWght"] = MinWght.toStdString();
    config[gPara]["Modnum"] = ui->stackedWidget->count();
}


// save instrumentfile
void MainWindow::saveFile(QString instrumentFile)
{
    //save instrument to file
    QFileInfo fileinfo(instrumentFile);
    ui->InstName->setText(fileinfo.baseName());
    QFile file(instrumentFile);
    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox::warning(this,"Save Instrument",
                             "Warning cannot open file: ",instrumentFile);
    }
    //stream to write to file
    ofstream fout(instrumentFile.toStdString());       // using namespace std
    //fill yaml node pipe with hole instrument data and write it to yml file
    YAML::Node pipe = YAML::LoadFile(instrumentFile.toStdString());
    //reset and read global variables into yaml node config
    config.reset();
    writeHeader(config);
    pipe[fileinfo.baseName().toStdString()][0]=config;
    config.reset();
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
        string key = ui->stackedWidget->widget(i)->objectName().toStdString();       //std::string
        if ( modultab->disableFlag[i]) config[key]["disabled"]=true;

        readCurModul(config,i);

        pipe[fileinfo.baseName().toStdString()][i+1]=config;
        config.reset();
    }

    series->getSerieVariablen(config);
    pipe["Serie"] = config;
    //write yaml node to file
    fout << pipe;
    file.close();
}

//start the compound pipe
void MainWindow::startPipe()
{
    if (pipeActive)
    {
        ui->textBrowser->setTextColor(Qt::red);
        ui->textBrowser->append("Pipe is active");
        ui->textBrowser->setTextColor(Qt::black);
        if (bigOutput->isVisible()) emit big(ui->textBrowser->toPlainText());
        return;
    }
    //create process list
    procList.clear();
    for (int i=0; i<ui->stackedWidget->count(); i++)
        //do not create process if modul is disabled
        if (!modultab->disableFlag[i])
        {
            procList.append(new QProcess());
            //remove old logfiles
            QFile::remove(logFname+QString::number(procList.count()));
        }
    connect(procList.last(),SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(finishedLast()));
    pipeActive = true;
    int enableIndex = 0;
    //create progressDialog and elapsed timer to get measurment time
    progress();
    timer.start();

    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
        if (!modultab->disableFlag[i])
        {
            //start processes of enabled modules in chain
            if (enableIndex < procList.count()-1 )
            {
                //Output of process is input of next process
                procList[enableIndex]->setStandardOutputProcess(procList[enableIndex+1]);     //pipe commands
                //toDo error handling
            }

            procList[enableIndex]->start(cmdList[enableIndex]);
            if (!procList[enableIndex]->waitForStarted())
            {
               QMessageBox::critical(this,"Critical Error!",
                            " Could not start process number:",QString::number(enableIndex));
               ui->textBrowser->setTextColor(Qt::red);
               ui->textBrowser->append( "Error with start module: " + QString::number(i));
               ui->textBrowser->setTextColor(Qt::black);
               if (bigOutput->isVisible()) emit big(ui->textBrowser->toPlainText());
               pipeActive = false;
               return;
            }
            enableIndex++;
        }
    }
}


//Last process in pipe finished
void MainWindow::finishedLast()
{
    //get measurment time in sec minimum 1
    QString str = QString::number(
                static_cast<int>(timer.elapsed()/1000 >0) ? static_cast<int>(timer.elapsed()/1000) : 1);

    QDate curDate = QDate::currentDate();
    //generate protocol filename from
    //instrument output dir + "/XC" + year + day in year + ".log"
    QString fileName = instrumentOutDir+"/XC"+QString::number(curDate.year())+
            QString::number(curDate.dayOfYear())+".log";
    //open protocol file
    QFile protFile(fileName);
    if (!protFile.open(QFile::ReadWrite | QIODevice::Append | QFile::Text))
    {
       QMessageBox::warning(this,"Save daily protocol file",
                            "Warning cannot open file: ",fileName);
       return;
    }
    //write cmdList to protocolfile
    foreach(QString pipe,cmdList)
        protFile.write((pipe+"\n").toStdString().c_str());
    protFile.write("\n\n");

    //write contents of each modul logfile to output area textbrowser and protocol file
    for (int i=0; i < procList.count(); i++)
    {
       QString logName = logFname + QString::number(i+1);
       QFile file(logName);
       if (!file.open(QFile::ReadOnly | QFile::Text))
       {
           QMessageBox::information(this,"Warning cannot open modul logfile: ", logName);
           return;
       }
       if (file.size() > 0)
       {
           //get logfile creation date and time
           QString createTime = "Date: "+ QFileInfo(logName).lastModified().toString("yyyyMMdd-hh:mm:ss")+ "\n\n";
           QByteArray arr = file.readAll();
           ui->textBrowser->append(arr);

           //write to daily protocol file
           protFile.write(createTime.toStdString().c_str());
           protFile.write(arr);
           protFile.write("\n\n");
       }
       else {
           protFile.write(("Pipe was interrupted at modul number: " + QString::number(i+1)).toStdString().c_str());
       }
       procList[i]->close();
   }
    //close process dialog
    progDial->pd->close();
    //write measurement time into textbrowser
    ui->textBrowser->append("Measurement took: " + str + " sec");
    if (bigOutput->isVisible()) emit big(ui->textBrowser->toPlainText());
    protFile.close();
    pipeActive = false;
}


//Validator slot for lineEdits
void MainWindow::checkIsValide()
{
    //check if input in lineEdit is valide
    QLineEdit *testEdit = qobject_cast<QLineEdit *>(sender());
    //palette.setColor(QPalette::Base,Qt::white);
    testEdit->setStyleSheet(" QLineEdit {background-color:lightyellow;}");
    if (!testEdit->hasAcceptableInput() && testEdit->text() != "" )
       //palette.setColor(QPalette::Base,Qt::red);
        testEdit->setStyleSheet(" QLineEdit {background-color:red;}");
    //testEdit->setPalette(palette);
}


//create progressDialog
void MainWindow::progress()
{
    progDial = new Progress(procList.count(),modultab->disableFlag,logFname,this);
    progDial->pd->show();
}

void MainWindow::toolCommand(QString prog)
{
    QProcess *toolProcess = new QProcess();
    #ifdef Q_OS_WIN
       toolProcess->start(VitessDir + "/MODULES/shelexec.exe /EXE " +VitessDir + "/MODULES/" + prog);
    #else
       toolProcess->start("xterm",QStringList()<< VitessDir + "/MODULES/" + prog + syspar);
    #endif

}

//Test gnuplot
void MainWindow::on_actionPlot_File_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,"Open Instrument",instrumentOutDir);
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
     gnuProc->start("/bin/sh",QStringList() << "-c" << "gnuplot -p testgnu.txt");
}

void MainWindow::on_action2D_Plot_File_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,"Open Instrument",instrumentOutDir);
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
     gnuProc->start("/bin/sh",QStringList() << "-c" << "gnuplot -p testgnu.txt");

}



void MainWindow::visualActive()
{
    if (!pipeActive)
    {
        tVisual->stop();
        Visualization();
    }
}

void MainWindow::Visualization()
{
    if (visualRepete < 2)
    {
        //create progressDialog and start elapsed timer to get measurment time
        progress();
        timer.start();
        QString simParam;
        //create process list
        procList.clear();
        for (int i=0; i<ui->stackedWidget->count(); i++)
            //do not create process if modul if disabled
            if (!modultab->disableFlag[i]) procList.append(new QProcess());
        connect(procList.last(),SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(finishedLast()));
        pipeActive = true;
        int enableIndex = 0;
        for (int i=0; i<ui->stackedWidget->count(); i++)
        {
            if (!modultab->disableFlag[i])
            {
                //start processes of enabled modules in chain
                if (enableIndex < procList.count()-1 )
                {
                    //Output of process is input of next process
                    procList[enableIndex]->setStandardOutputProcess(procList[enableIndex+1]);     //pipe commands
                }
                //two simulation runs
                //first  with  --vgeometry.inf
                //second with  --V<logFname><nr>v

                if (visualRepete > 0) simParam = " --V" + logFname + QString::number(i+1) + "v";
                else simParam =  " --vgeometry.inf";
                procList[enableIndex]->start(cmdList[enableIndex]+ simParam);
                if (!procList[enableIndex]->waitForStarted())
                {
                   ui->textBrowser->setTextColor(Qt::red);
                   ui->textBrowser->append( "Error with start module: " + QString::number(i));
                   ui->textBrowser->setTextColor(Qt::black);
                   if (bigOutput->isVisible()) emit big(ui->textBrowser->toPlainText());
                   pipeActive = false;
                   return;
                }
                enableIndex++;
            }
        }
        ui->textBrowser->setTextColor(Qt::blue);
        ui->textBrowser->append("Started visualization cycle: " + QString::number(visualRepete+1) +"\n");
        ui->textBrowser->setTextColor(Qt::black);
        visualRepete++;
        //check pipe to be ready
        tVisual->start(100);
    }
    else
    {
        //run sorting programm (sortiap)
        QProcess *sortProc = new QProcess;
        connect(sortProc,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(finishedSort()));
        QString cmd = VitessDir + "/MODULES/sortiap" + syspar + " -x ";
        //search for next new possible number of geom_ file
        for (int nr=1; nr<1000; nr++)
        {
            fGeom = instrumentOutDir + "/geom_" + QString::number(nr) + ".x3d";
            if ( !QFile::exists(fGeom)) break;
        }
        QString geoLogFiles = "";
        //
        for (int nr=1; nr<= procList.count(); nr++)
            if ( QFile::exists(logFname + QString::number(nr) + "v") )
                 geoLogFiles.append(" " + logFname + QString::number(nr) + "v");
            //else
            // ausgabe fehler und break
        cmd += "-o " + fGeom + geoLogFiles;
        sortProc->start(cmd);
    }
}


void MainWindow::finishedSort()
{
    ui->textBrowser->setTextColor(Qt::blue);
    ui->textBrowser->append("Starting InstantPlayer");
    ui->textBrowser->setTextColor(Qt::black);
    QProcess *playerProc = new QProcess;
    #ifdef Q_OS_WIN
       playerProc->start("cmd /c where.exe /R C:\\ InstantPlayer.exe");
    #else
       playerProc->start("which", QStringList() << "InstantPlayer");
    #endif
    playerProc->waitForReadyRead();
    QString output(playerProc->readAllStandardOutput());
    playerProc->close();
    if (output != "")
    {
        QFileInfo fi(output);
        QString exePath=fi.absolutePath();
        //show visualization x3d file
        #ifdef Q_OS_WIN
            //could not use output string directly,because of blanks in directory
            playerProc->start("\"" + exePath +"\"" +  "/InstantPlayer.exe " + fGeom);
        #else
            playerProc->start( exePath +  "/InstantPlayer " + fGeom);
        #endif
    }
    else {
        QMessageBox::information(this,"Programm is not get installed: ", "InstantPlayer");
        return;
    }
}


bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    if(ev->type()== QEvent::MouseButtonPress)
    {
      //get data (modul nr: parameter prefix: parameter name and parameter value) to generate series
      if ( series->isVisible() && qobject_cast<QLineEdit*>(obj) &&
                   QApplication::activeWindow()->objectName() == "MainWindow")
      {
              QString modul = ui->stackedWidget->currentWidget()->objectName();
              QString text =  " " + QString::number(ui->stackedWidget->currentIndex()+1) + ":" +
                   Module[modul][obj->objectName()]["prefix"]+ ":" + obj->objectName();
              QString val = qobject_cast<QLineEdit*>(obj)->text();

              emit getSeriesVariable(text,val);
       }
    }
    //combobox should not react on mouse wheel
    if(ev->type()== QEvent::Wheel)
    {
        QComboBox* combo = qobject_cast<QComboBox*>(obj);
        if (combo && !combo->hasFocus())
        return true;
    }
    return false;
}


void MainWindow::closeEvent( QCloseEvent *ev)
{
    QApplication::closeAllWindows();
    QWidget::closeEvent(ev);
}


void MainWindow::on_actionGenerate_Series_triggered()
{
    series->show();
    series->raise();
}

//start serie with arguments from separate series window
//serieVar list of strings <modulnr>:<prefix>:<parameter name>
//table    vector of series table rows
//selected rows, names of files to copy, name of target firectory
void MainWindow::startSerie(QStringList serieVar, QVector<QVector <QString>> table ,
                            QStringList selSelect, QStringList copyFiles, QString copyDir)
{
    //get pipe string
    ui->pushCheck->clicked();
    QStringList cmdSerie = cmdList;
    int row = 0;
    int selectIndex = 0;

    int colNr,beginEntry,endEntry;

    //loop over serie parameters. Find entries of serie parameters in cmdList and
    //replace the values with #<number of serie parameter> (#1,#2,...)
    if(!selSelect.contains("all"))
          row=selSelect[selectIndex].toInt()-1;

    //loop over all selected iteration table rows
    while(row < table.size())
    {
        //check if kill button has been pressed
        if (killBit){
            killBit = false;
            break;
        }
        colNr=0;
        cmdList = cmdSerie;
        //loop over enabled modules
        for (int modNr=0; modNr< cmdList.size(); modNr++)
        {
            //replaced all variable parameter
            if (colNr == serieVar.length()) break;

            beginEntry = cmdList[modNr].indexOf("--N")+3;
            endEntry = cmdList[modNr].indexOf(" ",beginEntry);
            //get modulnumber
            int instModNr = cmdList[modNr].mid(beginEntry, endEntry-beginEntry).toInt();

            for(int col=colNr; col<serieVar.length(); col++)
            {
                //modulnr in serieVar string
                int modNrSerie = serieVar[col].left(serieVar[col].indexOf(":")).toInt();
                //parameter prefix of serie parameter
                QString paramPrefix = serieVar[col].mid(serieVar[col].indexOf(":")+1,2);

                if (instModNr == modNrSerie)
                {
                   //look for prefix
                   if (cmdList[modNr].contains(paramPrefix))
                   {
                     //change cmdlist parameter value to serie parameter value
                     beginEntry = cmdList[modNr].indexOf(paramPrefix);
                     endEntry = cmdList[modNr].indexOf(" ",beginEntry);
                     QString entry = cmdList[modNr].mid(beginEntry, endEntry-beginEntry);
                     if (table[row][col].isEmpty())
                        cmdList[modNr].replace(entry, "");
                     else
                        cmdList[modNr].replace(entry,paramPrefix+table[row][col]);
                   }
                   else
                   {
                     //prefix not in list,because value was empty,therefore append
                     cmdList[modNr] = cmdList[modNr]+" " + paramPrefix + table[row][col];
                   }
                colNr=col;
                }
            }
        }  //loop modNr

         //start maesurement for one serie iteration
         if (!pipeActive)
         {
           ui->textBrowser->append("\n\nStart serie step: " + QString::number(row+1));
           startPipe();
         }
         else
         {
           //wait while pipe is active
           while(true)
           {
             if (pipeActive) delay();
             else break;
           }

           //copy selected files
           ui->textBrowser->setTextColor(Qt::red);
           foreach(QString file, copyFiles)
           {
              QFile from(instrumentOutDir+"/"+file);
              QFile target(copyDir+"/S"+QString::number(row+1)+"_"+file);
              if (target.exists()) target.remove();
              if (!from.exists())
                ui->textBrowser->append("File does not exist: " + from.fileName());
              else{
                from.copy(target.fileName());
                if (from.error() != QFile::NoError)
                   ui->textBrowser->append("Error in copy: " + from.fileName());
              }
           }
           ui->textBrowser->setTextColor(Qt::black);

           //series step selection
           if(selSelect.contains("all"))
               //all rows
               row++;
           else
           {
               //get next selected row
               if (selectIndex++ < selSelect.size())
                  row=selSelect[selectIndex].toInt()-1;
               else break;
           }
         }  //end pipe active
    }   //loop row
    ui->textBrowser->append("\nSerie ready");
}

void MainWindow::delay()
{
    //wait 1sec but events will be executed
    QTime dieTime= QTime::currentTime().addSecs(1);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}
