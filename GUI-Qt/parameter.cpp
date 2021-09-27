//=============================================================================
// File:    parameter.cpp
// Author:  Lydia Fleischhauer-Fuß <l.fleischhauer-fuss@fz-juelich.de>
// Date:    2021
// Purpose: Generate and handle subparameter in seperate window 
//=============================================================================

#include "parameter.h"
#include "ui_parameter.h"
#include <fstream>
#include <QTextStream>

using namespace YAML;
using namespace std;

Parameter::Parameter(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Parameter)
{
    //Window without close cross
    //CustomizeWindowHint flag turns off the default window title hints
    //WindowTitleHint gives the window only a title bar without icons
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    ui->setupUi(this);
    ui->numberEdit->setValidator(new QIntValidator(1,20,this));
}

Parameter::~Parameter()
{
    delete ui;
}

void Parameter::on_numberEdit_returnPressed()
{
    if ( ui->numberEdit->text().toInt() <= ui->stackedWidget->count())
         ui->stackedWidget->setCurrentIndex( ui->numberEdit->text().toInt()-1);
    else ui->numberEdit->setText( QString::number( ui->stackedWidget->currentIndex()+1));

}

void Parameter::on_butMinus_clicked()
{
    ui->stackedWidget->removeWidget(ui->stackedWidget->currentWidget());
    ui->labelNum->setText( QString::number( ui->stackedWidget->count()));
    ui->numberEdit->setText( QString::number( ui->stackedWidget->count()));
}

void Parameter::on_butPlus_clicked()
{
    designParameterWin(initFile);

}

void Parameter::designParameterWin(QString filename)
{
    initFile = filename;
    winScrollArea = new QScrollArea;
    winScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    QWidget *paramWindow = new QWidget;
    gridLayout = new QGridLayout;
    paramWindow->setLayout(gridLayout);
    winScrollArea->setWidget(paramWindow);
    for(int col=0; col<3; col++)  gridLayout->setColumnMinimumWidth(col,230);
    winScrollArea->setWidgetResizable(true);

    QFileInfo fileinfo(filename);
    this->setWindowTitle(fileinfo.baseName());
    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::information(this,"Load Instrumnet","Warning cannot open: ",filename);
        return;
    }
    ui->labelShow->setText("show "+fileinfo.baseName());
    ui->labelNumText->setText("currently set "+fileinfo.baseName()+"s:");
    YAML::Node config = YAML::LoadFile(filename.toStdString());
    ui->numWidget->hide();
    foreach (QString str, multipleWin)
        if(filename.contains(str,Qt::CaseInsensitive))
       {
           ui->numWidget->show();
           break;
       }

    //configure parameter window
    getModulSubParameter(config, "");

    ui->stackedWidget->addWidget(winScrollArea);
    ui->labelNum->setText( QString::number( ui->stackedWidget->count()));
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->count()-1);
    ui->numberEdit->setText( QString::number( ui->stackedWidget->count()));
}

void Parameter::getModulSubParameter(YAML::Node& configParam,QString modulName)
{
    //configure parameter window
    int iGritRow = 0;
    int index = 0;
    QLabel *headerLabel = new QLabel("<b>" +  modulName + "</b>\n");
    headerLabel->setObjectName("headerLabel");
    gridLayout->addWidget(headerLabel,iGritRow+1,0,1,3,Qt::AlignHCenter);
    iGritRow+=2;
    //loop all modul parameters
    YAML::Node configParameter = configParam[configParam.begin()->first.as<string>()];
    for(unsigned int ipipe = 0; ipipe < configParameter.size(); ipipe++)
    {
        for(YAML::const_iterator it=configParameter[ipipe].begin(); it!=configParameter[ipipe].end(); ++it)
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

           if( winScrollArea->widget()->findChild<QPushButton*>("browse_" + parName))
               connect(winScrollArea->widget()->findChild<QPushButton*>("browse_" + parName),
                       SIGNAL(clicked()),this,SLOT(browseBut_clicked()));
           if( winScrollArea->widget()->findChild<QPushButton*>("edit_" + parName))
               connect(winScrollArea->widget()->findChild<QPushButton*>("edit_" + parName),
                       SIGNAL(clicked()),this,SLOT(editBut_clicked()));
           if( winScrollArea->widget()->findChild<QLineEdit*>(parName))
                    connect(winScrollArea->widget()->findChild<QLineEdit*>(parName),
                           SIGNAL(textChanged(const QString &)),this,SLOT(checkIsValide()));

        }
    }
    if (iGritRow <= 10)
    {
        iGritRow++;
        gridLayout->addItem( new QSpacerItem(20,40,QSizePolicy::Minimum,QSizePolicy::Expanding),iGritRow,0);
    }
}

void Parameter::browseBut_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,"Open Instrument",instInDir);
    if (fileName == "") return;
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::information(this,"Warning cannot open: ",fileName);
        return;
    }
    QFileInfo fileinfo(fileName);
    // cut browse_ from sender
    ui->stackedWidget->currentWidget()->findChild<QLineEdit *>(
                qobject_cast<QPushButton *>(sender())->objectName().mid(7))
                ->setText(fileinfo.fileName());
}
void Parameter::editBut_clicked()
{
     QString str = qobject_cast<QPushButton *>(sender())->objectName().mid(5);
     QString fileName = ui->stackedWidget->currentWidget()->findChild<QLineEdit *>(str)->text();
     if (fileName == "") return;
     else fileName = instInDir+"/"+fileName;
     QFile file(fileName);
     if (!file.open(QFile::ReadOnly | QFile::Text))
     {
         QMessageBox::warning(this,"Show file","Warning cannot open file: ",fileName);
         return;
     }
     QPlainTextEdit* textEdit = new QPlainTextEdit();
     textEdit->setWindowModality(Qt::ApplicationModal);
     textEdit->resize(700,350);
     textEdit->setPlainText(file.readAll());
     textEdit->show();
}


void Parameter::checkIsValide()
{
    QLineEdit *testEdit = qobject_cast<QLineEdit *>(sender());
    palette.setColor(QPalette::Base,Qt::white);
    if (!testEdit->hasAcceptableInput() && testEdit->text() != "" )
        palette.setColor(QPalette::Base,Qt::red);
    testEdit->setPalette(palette);
}

void Parameter::on_pushClose_clicked()
{
    while ( ui->stackedWidget->count() > 1 )
        ui->stackedWidget->removeWidget( ui->stackedWidget->widget(1) );
    ui->labelNum->setText( QString::number( ui->stackedWidget->count()));
    ui->numberEdit->setText( QString::number( ui->stackedWidget->count()));
    this->close();
}

void Parameter::on_pushSave_clicked()
{
    //save parameter values in file
    QFileInfo fileinfo(initFile);
    QString fileName = QFileDialog::getSaveFileName(this,"Open Instrument",instInDir,
                                                    tr("YML(*.yml) (*.yml)"));
    if (fileName.isEmpty()) return;
    if (!fileName.endsWith(".yml"))
        fileName += ".yml";
    QFile file(fileName);
    if (!file.open(QFile::ReadWrite | QFile::Text))
    {
        QMessageBox::information(this,"Warning cannot open: ",fileName);
        fileName = "";
        return;
    }
    //stream to write to file
    ofstream fout(fileName.toStdString());       // using namespace std

    //write yaml file
    YAML::Node config;
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
        YAML::Node configWin;
        getData(configWin,i);
        config[this->windowTitle().toStdString()][i] = configWin;
    }
    fout << config;
    file.close();
    emit changedParamWidget(fileName,fileinfo.baseName());
    while ( ui->stackedWidget->count() > 1 )
        ui->stackedWidget->removeWidget( ui->stackedWidget->widget(1) );
    ui->labelNum->setText( QString::number( ui->stackedWidget->count()));
    ui->numberEdit->setText( QString::number( ui->stackedWidget->count()));
    this->close();
}

void Parameter::loadFile(QString fileName)
{
    QFileInfo fileinfo(fileName);
    instInDir = fileinfo.path();
    YAML::Node config = YAML::LoadFile(fileName.toStdString());
    YAML::Node config_paramWin = config[config.begin()->first.as<string>()];
    for (unsigned i=1; i < config_paramWin.size(); i++)
        designParameterWin(initFile);
    for (unsigned i=0; i < config_paramWin.size(); i++)
    {
       int ind = static_cast <int> (i);
       for(YAML::const_iterator it=config_paramWin[ind].begin(); it!=config_paramWin[i].end(); ++it)
       {
          QString paramKey = QString::fromStdString(it->first.as<std::string>());      //key
          QString paramVal = QString::fromStdString(it->second.as<std::string>());      //value
          if (ui->stackedWidget->widget(ind)->findChild<QLineEdit *>(paramKey))
             ui->stackedWidget->widget(ind)->findChild<QLineEdit *>(paramKey)->setText(paramVal);
          if (ui->stackedWidget->widget(ind)->findChild<QComboBox *>(paramKey))
             ui->stackedWidget->widget(ind)->findChild<QComboBox *>(paramKey)
                      ->setCurrentIndex(paramVal.toInt());
       }
    }
}

void Parameter::saveData(YAML::Node& config,std::string key,QString param,QString parFile)
{
    //called if instrument is saved
    loadFile(parFile);
    YAML::Node configWin;
    for (int i=0; i<ui->stackedWidget->count(); i++)
    {
        getData(configWin,i);
        config[key][param.toStdString()][i] = configWin;
        configWin.reset();
    }
}

void Parameter::getData(YAML::Node& configWin, int i)
{
    allLineEdits  =  ui->stackedWidget->widget(i)->findChildren< QLineEdit *>();
    allComboBoxes =  ui->stackedWidget->widget(i)->findChildren< QComboBox *>();
    allCheckBoxes =  ui->stackedWidget->widget(i)->findChildren< QCheckBox *>();

    for(int ii=0 ; ii < allLineEdits.size(); ii++)
       if (allLineEdits[ii]->text() != "")
          configWin[allLineEdits[ii]->objectName().toStdString()] =
                            allLineEdits[ii]->text().toStdString();
    for(int ii=0 ; ii<allComboBoxes.size(); ii++)
       configWin[allComboBoxes[ii]->objectName().toStdString()] =
                            allComboBoxes[ii]->currentIndex();
    for(int ii=0 ; ii<allCheckBoxes.size(); ii++)
       configWin[allCheckBoxes[ii]->objectName().toStdString()] =
                            allCheckBoxes[ii]->isChecked();
}
