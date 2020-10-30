#include "chopper_para.h"
#include "ui_chopper_para.h"
#include <iostream>
#include <QDoubleValidator>

Chopper_para::Chopper_para(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Chopper_para)
{
    ui->setupUi(this);
    this->setWindowTitle("Chopper windows parameter");
    QDoubleValidator *validatorFloat = new QDoubleValidator(this);
    QList<QLineEdit *> allLineEdits =this->findChildren<QLineEdit*>();
    for(int i=0 ; i<allLineEdits.size(); i++)
       allLineEdits[i]->setValidator(validatorFloat);
}

Chopper_para::~Chopper_para()
{
    delete ui;
}

void Chopper_para::writePara(YAML::Node& config,std::string moduleName)
{
    YAML::Node config2;

    for (int i=0; i<4 ;i++)
    {
        config2.reset();
        QLineEdit *Win = this->findChild<QLineEdit*>("WndPos_"+ QString::number(i));
        if ( Win->text() != "")
        {
            std::cout << Win->text().toStdString() << std::endl;
            config2.reset();
            config2["WndPos"] =  this->findChild<QLineEdit*>("WndPos_"+ QString::number(i))->text().toStdString();
            config2["WndWidth"] = this->findChild<QLineEdit*>("WndWidth_"+ QString::number(i))->text().toStdString();
            config2["WndHite"] = this->findChild<QLineEdit*>("WndHite_"+ QString::number(i))->text().toStdString();
            config2["DevL"] = this->findChild<QLineEdit*>("DevL_"+ QString::number(i))->text().toStdString();
            config2["DevR"] = this->findChild<QLineEdit*>("DevR_"+ QString::number(i))->text().toStdString();
            config[moduleName]["window"][i]= config2;
        }
        else break;
    }
}

void Chopper_para::readPara(YAML::Node& Windows)
{
    QList<QLineEdit*>lEdits = this->findChildren<QLineEdit*>();
    foreach (QLineEdit *lineEdit, lEdits) {
        lineEdit->clear();
    }
    QString text;
    for (unsigned i=0; i < Windows.size(); i++)
    {
        text = QString::fromStdString(Windows[i]["WndPos"].as<std::string>());
        this->findChild<QLineEdit*>("WndPos_"+ QString::number(i))->setText(text);

        text = QString::fromStdString(Windows[i]["WndWidth"].as<std::string>());
        this->findChild<QLineEdit*>("WndWidth_"+ QString::number(i))->setText(text);

        text = QString::fromStdString(Windows[i]["WndHite"].as<std::string>());
        this->findChild<QLineEdit*>("WndHite_"+ QString::number(i))->setText(text);

        text = QString::fromStdString(Windows[i]["DevL"].as<std::string>());
        this->findChild<QLineEdit*>("DevL_"+ QString::number(i))->setText(text);

        text = QString::fromStdString(Windows[i]["DevR"].as<std::string>());
        this->findChild<QLineEdit*>("DevR_"+ QString::number(i))->setText(text);
    }
}
