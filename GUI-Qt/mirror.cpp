#include "mirror.h"
#include "ui_mirror.h"
#include <iostream>
#include <QWidget>

Mirror::Mirror(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Mirror)
{
    ui->setupUi(this);
    MirrorWindows << ui->MirrorWin_0
                  << ui->MirrorWin_1
                  << ui->MirrorWin_2
                  << ui->MirrorWin_3
                  << ui->MirrorWin_4
                  << ui->MirrorWin_5
                  << ui->MirrorWin_6
                  << ui->MirrorWin_7
                  << ui->MirrorWin_8
                  << ui->MirrorWin_9
                  << ui->MirrorWin_10;
    foreach (QWidget* MirrorWin,MirrorWindows)
      MirrorWin->hide();
}

Mirror::~Mirror()
{
    delete ui;
}


void Mirror::on_nrWin_valueChanged(int nr)
{
    cout << "nr mirror changed" << nr << endl;
    foreach (QWidget* MirrorWin,MirrorWindows)
      MirrorWin->hide();
    for (int var = 1; var <= nr; ++var)
        MirrorWindows[var-1]->show();
}

void Mirror::writePara(YAML::Node& config,std::string moduleName)
{
    YAML::Node config2;
    int nr = ui->nrWin->value();
    for (int i=0; i<nr; i++) {
        config2.reset();
        QWidget *MirrorWin = this->findChild<QWidget*>("MirrorWin_"+ QString::number(i));
        lEdits = MirrorWin->findChildren<QLineEdit*>();
        cBoxes = MirrorWin->findChildren<QComboBox*>();
        for(int i=0 ; i < lEdits.size(); i++)
        {
            int pos = lEdits[i]->objectName().lastIndexOf("_");
            config2[lEdits[i]->objectName().left(pos).toStdString()] = lEdits[i]->text().toStdString();
        }
        for(int i=0 ; i < cBoxes.size(); i++)
            config2[cBoxes[i]->objectName().toStdString()] = cBoxes[i]->currentText().toStdString();
        config[moduleName]["Mirror"][i]= config2;
    }
}

void Mirror::readPara(YAML::Node& Mirror)
{
    QString text;
    ui->nrWin->setValue(static_cast<int>(Mirror.size()));
    for (unsigned i=0; i < Mirror.size(); i++)
    {
        QWidget *MirrorWin = this->findChild<QWidget*>("MirrorWin_"+ QString::number(i));
        lEdits = MirrorWin->findChildren<QLineEdit*>();
        cBoxes = MirrorWin->findChildren<QComboBox*>();
        for(YAML::const_iterator it=Mirror[i].begin(); it!=Mirror[i].end(); ++it)
        {
           QString childName = QString::fromStdString(it->first.as<std::string>());      //key
           flag = false;
           cout << "Mirror first:" << it->first.as<string>() << "  second:" << it->second.as<string>() << endl;
           foreach (QLineEdit* child, lEdits)
           {
              int pos = child->objectName().lastIndexOf("_");
              if ( childName.indexOf(child->objectName().left(pos)) == 0 )                   //if key is lineEdit objectname
              {
                 flag = true;
                 child->setText(QString::fromStdString(it->second.as<string>()));  //setText of linEdit
                 break;
              }
           }
           if (!flag)                                                               //if key is no lineEdit
           foreach (QComboBox* child, cBoxes)
           {
             int pos = child->objectName().lastIndexOf("_");
             if ( childName.indexOf(child->objectName().left(pos)) == 0 )                 //if key is comboBox objectname
             {
                child->setCurrentText(QString::fromStdString(it->second.as<string>()));
                break;
             }
           }
        }
     }
}
