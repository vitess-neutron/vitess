#include "inelast_para.h"
#include "ui_inelast_para.h"
//#include <iostream>

//Inelast_para::Inelast_para(QWidget *parent) :
//    QDialog(parent),

Inelast_para::Inelast_para(BaseDialog *parent) :
    BaseDialog(parent),
    ui(new Ui::Inelast_para)
{
    ui->setupUi(this);
    allLineEdits =this->findChildren<QLineEdit*>();
    setValidatorFloat();
    foreach(QString entry, mapLimit.keys())
    {
        QLineEdit *lEdit = this->findChild<QLineEdit *>(entry);
        setValidator( lEdit,mapLimit);
        std::cout << entry.toStdString() << std::endl;
        connect(this->findChild<QLineEdit *>(entry), SIGNAL(textChanged(const QString &)),this,
                                                     SLOT(checkIsValide()));
    }
}

Inelast_para::~Inelast_para()
{
    delete ui;
}
/*
void Inelast_para::writePara(YAML::Node& config,std::string moduleName)
{

    allLineEdits = this->findChildren<QLineEdit*>();
    allComboBoxes = this->findChildren<QComboBox*>();
    writeParameter(config, moduleName);

}
*/
