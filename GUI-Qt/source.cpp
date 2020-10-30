#include "source.h"
#include "ui_source.h"

#include <iostream>
#include <QFileDialog>

using namespace std;

Source::Source(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Source)
{
    ui->setupUi(this);
    foreach(QString entry, map.keys())
        if (this->findChild<QLineEdit *>(entry))
        {
           QLineEdit *lEdit = this->findChild<QLineEdit *>(entry);
           setValidator( lEdit,map[entry]);
           connect(this->findChild<QLineEdit *>(entry), SIGNAL(textChanged(const QString &)),this,
                                                        SLOT(checkIsValide()));
           allLineEdits += this->findChild<QLineEdit *>(entry);
        }
    allComboBoxes = this->findChildren<QComboBox*>();
    modName = this->objectName().toStdString();
    ui->frame->hide();
    ui->eType->addItem("CWS",QVariant(VteType::CWS));
    ui->eType->addItem("SPSS",QVariant(VteType::SPSS));
    ui->eType->addItem("LPSS",QVariant(VteType::LPSS));

}

Source::~Source()
{
    delete ui;
}

void Source::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Source::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Source::writeValues(YAML::Node& config)
{
    sourceList = ui->frame->findChildren<QLineEdit*>();
    foreach (QLineEdit* lEdit, ui->frame->findChildren<QLineEdit*>())
      sourceEntry.append(lEdit->objectName());
    for(int i=0 ; i < allLineEdits.size(); i++)
    {
      if(sourceEntry.indexOf(allLineEdits[i]->objectName()) != -1)
          config[modName]["Source"][allLineEdits[i]->objectName().toStdString()] = allLineEdits[i]->text().toStdString();
      else
        config[modName]["Simulation"][allLineEdits[i]->objectName().toStdString()] = allLineEdits[i]->text().toStdString();
    }

    for(int i=0 ; i<allComboBoxes.size(); i++)
    {
      if(sourceCombo.indexOf(allComboBoxes[i]->objectName()) != -1)
        config[modName]["Source"][allComboBoxes[i]->objectName().toStdString()] = allComboBoxes[i]->currentText().toStdString();
      else
        config[modName]["Simulation"][allComboBoxes[i]->objectName().toStdString()] = allComboBoxes[i]->currentText().toStdString();
    }

    moderator.writePara(config,modName);
}

void Source::readValues(YAML::Node& config)
{
    YAML::Node NewNode = config["Simulation"];
    readModule(NewNode);
    NewNode = config["Source"];
    readModule(NewNode);
    NewNode = config["Moderator"];
    moderator.readPara(NewNode);
}

void Source::on_eType_currentIndexChanged(int index)
{
    if (index > 1)
         ui->frame->show();
    else ui->frame->hide();
}

void Source::on_ModPara_clicked()
{
    moderator.exec();
}

void Source::on_pushRayTracing_clicked()
{
    QString fileName = openFileName();
    ui->TrcFile->setText(fileName);

}

void Source::on_pushModFile_clicked()
{
    QString modFile = QFileDialog::getOpenFileName(this,"Moderator file","/home/jcns/Downloads/vitess3.4",
                                                 "Files (*.mod)");
    ui->ModFile->setText(modFile);
}
