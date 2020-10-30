#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <QStringList>
#include "yaml-cpp/yaml.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <fstream>
#include "string.h"
#include <QDateTime>
#include <QDesktopServices>
#include <QProcess>
#include <unistd.h>    // for sleep command test
using namespace std;
using namespace YAML;

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

    //Widget list with all existing modul widget classes
//    allModulWidgets.append(new Beamstop());
//    allModulWidgets.append(new Detector());
//    allModulWidgets.append(new Filter());
    allModulWidgets.append(new Flipper_coil());
    allModulWidgets.append(new Flipper_gradient());
    allModulWidgets.append(new Frame());
//    allModulWidgets.append(new Chopper_fermi_str());
//    allModulWidgets.append(new Chopper_fermi_cur());
    allModulWidgets.append(new Chopper_disc());
//    allModulWidgets.append(new Capture_flux());
//    allModulWidgets.append(new Collimator_radial());
//    allModulWidgets.append(new Collimator());
    allModulWidgets.append(new Guide());
    allModulWidgets.append(new Monitor1D());
    allModulWidgets.append(new Monitor2D());
    allModulWidgets.append(new Monochr_analyser());
    allModulWidgets.append(new Monochromator());
    allModulWidgets.append(new Polariser_he3());
    allModulWidgets.append(new Polariser_sm());
    allModulWidgets.append(new Precessionfield());
    allModulWidgets.append(new Rotating_field());
    allModulWidgets.append(new Resonator_drabkin());
    allModulWidgets.append(new Source());
    allModulWidgets.append(new Spacewindow());
    allModulWidgets.append(new Space());
    allModulWidgets.append(new Slit());
    allModulWidgets.append(new Sample_environment());
    allModulWidgets.append(new Sample_elasticisotr());
    allModulWidgets.append(new Sample_inelast());
    allModulWidgets.append(new Sample_nxs());
    allModulWidgets.append(new Sample_powder());
    allModulWidgets.append(new Sample_reflectom());
    allModulWidgets.append(new Sample_sans());
    allModulWidgets.append(new Sample_singcryst());
    allModulWidgets.append(new Sample_s_q());
    allModulWidgets.append(new Sm_ensemble());
    allModulWidgets.append(new Velselect());

    //modindex used for counting multiple class instances per module
    for (int i=0; i<allModulWidgets.size();i++)
        modindex << 0;

    // List of modules
    // Module names have to be the objectnames of the corresponding widgets
    // Subentries begin with the spezial character QChar(0x2514)
    QStringList modulNames;
    modulNames
    //           <<   "Beamstop"
               << "Chopper:"
               << QString("%1 Chopper_disc").arg(QChar(0x2514))
    //           << QString("%1 Chopper_fermi_str").arg(QChar(0x2514))
    //           << QString("%1 Chopper_fermi_cur").arg(QChar(0x2514))
    //           << "Collimators:"                                           // entry has subentries
    //           << QString("%1 Collimator").arg(QChar(0x2514))              // subentry
    //           << QString("%1 Collimator_radial").arg(QChar(0x2514))
    //           << "Detector"
    //           << "Evaluation:"
    //           << QString("%1 Capture_flux").arg(QChar(0x2514))
    //           << "Filter"
               << "Flipper:"
               << QString("%1 Flipper_coil").arg(QChar(0x2514))
               << QString("%1 Flipper_gradient").arg(QChar(0x2514))
               << "Frame"
               << "Guide"
               << "Magnetic_field:"
               << QString("%1 Precessionfield").arg(QChar(0x2514))
               << QString("%1 Rotating_field").arg(QChar(0x2514))
               << "Monochromator:"
               << QString("%1 Monochr_analyser").arg(QChar(0x2514))
               << QString("%1 Monochromator").arg(QChar(0x2514))
               << "Polariser:"
               << QString("%1 Polariser_he3").arg(QChar(0x2514))
               << QString("%1 Polariser_sm").arg(QChar(0x2514))
               << "Resonator_drabkin"
               << "Sample:"                                             // entry has subentries
               << QString("%1 Sample_elasticisotr").arg(QChar(0x2514))  // subentry
               << QString("%1 Sample_inelast").arg(QChar(0x2514))
               << QString("%1 Sample_nxs").arg(QChar(0x2514))
               << QString("%1 Sample_reflectom").arg(QChar(0x2514))
               << QString("%1 Sample_powder").arg(QChar(0x2514))
               << QString("%1 Sample_sans").arg(QChar(0x2514))
               << QString("%1 Sample_singcryst").arg(QChar(0x2514))
               << QString("%1 Sample_s_q").arg(QChar(0x2514))
               << "Sample_environment"
               << "Sm_ensemble"
               << "Slit"
               << "Source"
               << "Spacewindow"
               << "Space"
               << "Vizualisation:"
               << QString("%1 Monitor1D").arg(QChar(0x2514))
               << QString("%1 Monitor2D").arg(QChar(0x2514))
               << "Velselect";

    modultab = new ModulTable(modulNames, ui->widget_modul);

    instrumentName ="";
    //get user name and current path for later file prefix      //to do
    userName = getenv("USER");
    pwd = QDir::currentPath()+"/";
    QString timeInSec = QString::number(QDateTime::currentSecsSinceEpoch(),16);
    //cout << "user: " << userName.toStdString() << "    time: " << timeInSec.toStdString() << endl;


    //Connect signals to slots
    //An arrow was pressed
    connect(modultab,SIGNAL(arrowPressed(int)),this,SLOT(comboModulItemChanged(int)));

    //Modul comboBox Value changed
    connect(modultab,SIGNAL(changedComboVal(QString,int)),this,SLOT(comboModulItemChangedVal(QString,int)));

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
void MainWindow::comboModulItemChanged(int row)
{
    ui->stackedWidget->setCurrentIndex(row);
    ui->stackedWidget->show();
}

//Module table value changed or module added
void MainWindow::comboModulItemChangedVal(QString text,int row)
{
    for (int i=0; i<allModulWidgets.size(); i++)
    {
        //look for matching objectName
        switch ( text.indexOf(allModulWidgets[i]->objectName()) )
        {
        case 0: // main entry
        case 2: // subentry
            if (modindex[i] == 0)                //first entry of this module in stackedWidget
                newModule = allModulWidgets[i];
            else                                 //multiple entry of this module in stackedWidget
                newModule =qobject_cast<QWidget*>(allModulWidgets[i]->metaObject()->newInstance());
            //replace or add module
            if (row != ui->stackedWidget->count())
            {
                ui->stackedWidget->removeWidget(ui->stackedWidget->widget(row));
                ui->stackedWidget->insertWidget(row,newModule);
            }
            else
                ui->stackedWidget->addWidget(newModule);

            modindex[i]++;
            ui->stackedWidget->setCurrentIndex(row);
            ui->stackedWidget->show();
            return;
        }
    }
    ui->stackedWidget->hide();   //no corresponding widget
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
    ui->stackedWidget->insertWidget(row,new Dummy());   //place holder until new module is selected
    ui->stackedWidget->setCurrentIndex(row);
    ui->stackedWidget->show();
}

//Menue load instrument
void MainWindow::on_actionLoad_triggered()
{
   instrumentName = QFileDialog::getOpenFileName(this,"Open Instrument","/home/jcns/Downloads/vitess3.4",
                                                 tr("YAML (*.yaml *.yml)"));
   //ui->InstName->setText(instrumentName.section(QDir::separator(),-1));
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
   for (int i=0; i<allModulWidgets.size();i++)
          modindex[i] = 0;

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
           for (int i=0; i<allModulWidgets.size(); i++)
           {
              //searching for matching module
              if (module == allModulWidgets[i]->objectName())
              {
                  //put module in tabelle, this sends signal changedComboVal
                  modultab->loadModule(module);
                  //read yaml values in correct instance of module in stackedWidget
                  //readValues is virtual function in BaseModule,so readValues in the fitting
                  //module is called
                  if (modindex[i] == 0)   //1
                    qobject_cast<BaseModule *>(allModulWidgets[i])->readValues(configChildren);
                  else
                    qobject_cast<BaseModule *>(newModule)->readValues(configChildren);
                  //show module page
                  ui->stackedWidget->setCurrentIndex(ui->stackedWidget->count()-1);
                  ui->stackedWidget->show();
                  break;
              }
           }
   }
   file.close();
   //to do: error case
   ui->textBrowser->setText("Successfully loaded intrument:  "+fileinfo.baseName());
   //ui->textBrowser->setText("Successfully loaded intrument:"+instrumentName);
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
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
        //cast stacked widget to type BaseModule and call virtual function writeValues
        //this starts the function in the correct module
        qobject_cast<BaseModule *>(ui->stackedWidget->widget(i))->writeValues(config);
        fout << config;
        fout << "\n";
        config.reset();
    }
    file.close();
}

void MainWindow::on_actionSave_as_triggered()
{
    instrumentName = QFileDialog::getSaveFileName(this,"Save Instrument as","/home/jcns/Downloads/vitess3.4",
                                                  tr("Files (*.yaml *.yml)"));
    if (!instrumentName.endsWith(".yaml") && !instrumentName.endsWith(".yml"))
        instrumentName += ".yml";
    QFileInfo fileinfo(instrumentName);
    ui->InstName->setText(fileinfo.baseName());
    QFile file(instrumentName);
    if (!file.open(QFile::WriteOnly | QFile::Text))        //open file
    {
        QMessageBox::warning(this,"cannot open file:",instrumentName);
        return;
    }
    ofstream fout(instrumentName.toStdString());          // std::ofstream

    config = YAML::LoadFile(instrumentName.toStdString());
    writeHeader(config);
    fout << config;
    fout << "\n";
    config.reset();
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
       qobject_cast<BaseModule *>(ui->stackedWidget->widget(i))->writeValues(config);
       fout << config;
       fout << "\n";
       config.reset();
    }
    file.close();
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
    for (int i=0; i<allModulWidgets.size();i++)
           modindex[i] = 0;

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
    foreach(QString entry, map.keys())
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
        foreach(QString entry, map.keys())
            if ( entry.indexOf( childName) == 0 )                   //if key is lineEdit objectname
            {
                if (this->findChild<QLineEdit *>(entry))
//                if (map[entry][1] == "lEdit")
                    this->findChild<QLineEdit *>(childName)->setText(
                                QString::fromStdString(iter->second.as<string>()));
//                else if(map[entry][1] == "qCombo")
                else if (this->findChild<QComboBox *>(entry))
                    this->findChild<QComboBox *>(childName)->setCurrentText(
                                QString::fromStdString(iter->second.as<string>()));
//                else if(map[entry][1] == "menu")
                else if(childName == "MinWght" | childName == "nBuffer")
                   {
                    QString str;
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
}


void MainWindow::getHeader(QTextStream& out)
{
    //get global values and the matching entries from map
    foreach(QString entry, map.keys())
    {
        out << " " << map[entry][0];
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
        cmd = pwd + "MODULES/";
        cmd += ui->stackedWidget->widget(i)->objectName().toLower() + syspar;
        cmd += " --N" + QString::number(i+1);    //Modnum
        cmd += headerStr;
        cmd += " --L/home/jcns/source/testlog" + QString::number(i+1);
        qobject_cast<BaseModule *>(ui->stackedWidget->widget(i))->writeCmd(cmd);
        cmdList.append(cmd);
        if (i < ui->stackedWidget->count()-1) cmd += " | ";
        ui->textBrowser->append(cmd);
    }
}

void MainWindow::on_pushIndir_clicked()
{
    InDir = QFileDialog::getExistingDirectory(this,"Set input directory",
                                 "/home/"+userName,QFileDialog::ShowDirsOnly);
    ui->InDir->setText(InDir);
}

void MainWindow::on_pushOutdir_clicked()
{
    OutDir = QFileDialog::getExistingDirectory(this,"Set output directory",
                                  "/home/"+userName,QFileDialog::ShowDirsOnly);
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
    //toDo has to be tested sleep raus
    for (int i=0; i<ui->stackedWidget->count(); i++)
        if (procList[i]->state() > 0)
        {
            procList[i]->terminate();
            ui->textBrowser->append( "Module: " + QString::number(i) + " stopped;");
            sleep(2);
        }
    ui->textBrowser->setTextColor(Qt::black);
}
