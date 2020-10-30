#include "moderator.h"
#include "ui_moderator.h"
#include <iostream>
#include <QWidget>
#include "basemodule.h"

using namespace std;

Moderator::Moderator(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Moderator)
{
    ui->setupUi(this);
    this->setWindowTitle("Moderator parameter");
    QDoubleValidator *validatorFloat = new QDoubleValidator(this);
    QList<QLineEdit *> allLineEdits =this->findChildren<QLineEdit*>();
    for(int i=0 ; i<allLineEdits.size(); i++)
    {
        int pos = allLineEdits[i]->objectName().lastIndexOf("_");
        allLineEdits[i]->setValidator(validatorFloat);
        foreach(QString IEdit,StrEdit)
          if( allLineEdits[i]->objectName().left(pos) == IEdit)
          {
              allLineEdits[i]->setValidator(nullptr);
              break;
          }
    }
    ModWindows << ui->ModWin_1
                  << ui->ModWin_2
                  << ui->ModWin_3
                  << ui->ModWin_4;
    for(int i=1; i<4; i++)
      ModWindows[i]->hide();
}

Moderator::~Moderator()
{
    delete ui;
}

void Moderator::writePara(YAML::Node& config,std::string moduleName)
{
    YAML::Node config2;
    int nr = ui->nrMod->value();
    for (int i=1; i<=nr; i++) {
        config2.reset();
        QWidget *ModeratorWin = this->findChild<QWidget*>("ModWin_"+ QString::number(i));
        lEdits = ModeratorWin->findChildren<QLineEdit*>();
        cBoxes = ModeratorWin->findChildren<QComboBox*>();
        for(int i=0 ; i < lEdits.size(); i++)
        {
           int pos = lEdits[i]->objectName().lastIndexOf("_");
           config2[lEdits[i]->objectName().left(pos).toStdString()] = lEdits[i]->text().toStdString();
        }
        for(int i=0 ; i < cBoxes.size(); i++)
            config2[cBoxes[i]->objectName().toStdString()] = cBoxes[i]->currentText().toStdString();
        config[moduleName]["Moderator"][i-1]= config2;
    }
}


void Moderator::readPara(YAML::Node& Moderator)
{
    QString text;
    ui->nrMod->setValue(static_cast<int>(Moderator.size()));
    for (unsigned i=0; i < Moderator.size(); i++)
    {
        QWidget *ModeratorWin = this->findChild<QWidget*>("ModWin_"+ QString::number(i+1));
        lEdits = ModeratorWin->findChildren<QLineEdit*>();
        cBoxes = ModeratorWin->findChildren<QComboBox*>();

        for(YAML::const_iterator it=Moderator[i].begin(); it!=Moderator[i].end(); ++it)
        {
           QString childName = QString::fromStdString(it->first.as<std::string>());      //key
           flag = false;
           foreach (QLineEdit* child, lEdits)
           {
              int pos = child->objectName().lastIndexOf("_");
              if ( childName.indexOf(child->objectName().left(pos)) == 0 )                   //if key is lineEdit objectname
              {
                flag = true;
                 child->setText(QString::fromStdString(it->second.as<std::string>()));  //setText of linEdit
                 break;
              }
           }
           if (!flag)                                                               //if key is no lineEdit
           foreach (QComboBox* child, cBoxes)
           {
             int pos = child->objectName().lastIndexOf("_");
             if ( childName.indexOf(child->objectName().left(pos)) == 0 )                 //if key is comboBox objectname
             {
                child->setCurrentText(QString::fromStdString(it->second.as<std::string>()));
                break;
             }
           }
        }
     }
}

void Moderator::on_nrMod_valueChanged(int nr)
{
    std::cout << "moderator nr:" << nr << std::endl;
    foreach (QWidget* ModWin,ModWindows)
      ModWin->hide();
    for (int var = 1; var <= nr; ++var)
        ModWindows[var-1]->show();


}
