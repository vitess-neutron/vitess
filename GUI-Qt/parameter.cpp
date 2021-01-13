#include "parameter.h"
#include "ui_parameter.h"

using namespace YAML;
using namespace std;

Parameter::Parameter(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Parameter)
{
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
    designParameterWin(fname);

}

void Parameter::designParameterWin(QString filename)
{
    fname = filename;
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
    YAML::Node configParam = config.begin()->second;
    getModulSubParameter(configParam, "");

    ui->stackedWidget->addWidget(winScrollArea);
    ui->labelNum->setText( QString::number( ui->stackedWidget->count()));
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->count()-1);
    ui->numberEdit->setText( QString::number( ui->stackedWidget->count()));
}


void Parameter::getModulSubParameter(YAML::Node& configParam,QString modulName)
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

        if( winScrollArea->widget()->findChild<QPushButton*>("browse_" + parName))
            connect(winScrollArea->widget()->findChild<QPushButton*>("browse_" + parName),
                    SIGNAL(clicked()),this,SLOT(browseBut_clicked()));
        else if( winScrollArea->widget()->findChild<QLineEdit*>(parName))
                connect(winScrollArea->widget()->findChild<QLineEdit*>(parName),
                    SIGNAL(textChanged(const QString &)),this,SLOT(checkIsValide()));
    }
    if (iGritRow <= 10)
    {
        iGritRow++;
        gridLayout->addItem( new QSpacerItem(20,40,QSizePolicy::Minimum,QSizePolicy::Expanding),iGritRow,0);
    }

}

void Parameter::browseBut_clicked()
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
    this->close();
}

