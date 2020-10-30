#include "sans_para.h"
#include "ui_sans_para.h"

Sans_para::Sans_para(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Sans_para)
{
    ui->setupUi(this);
}

Sans_para::~Sans_para()
{
    delete ui;
}
void Sans_para::writePara(YAML::Node& config,std::string moduleName)
{

    QList<QLineEdit *> allLineEdits = this->findChildren<QLineEdit*>();
    QList<QComboBox *> allComboBoxes = this->findChildren<QComboBox*>();

    for(int i=0 ; i < allLineEdits.size(); i++)
        config[moduleName][allLineEdits[i]->objectName().toStdString()] = allLineEdits[i]->text().toStdString();
    for(int i=0 ; i<allComboBoxes.size(); i++)
        config[moduleName][allComboBoxes[i]->objectName().toStdString()] = allComboBoxes[i]->currentText().toStdString();
}
