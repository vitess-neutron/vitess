#include "basedialog.h"
#include <iostream>

BaseDialog::BaseDialog(QWidget *parent) :
    QDialog(parent)
{
}

BaseDialog::~BaseDialog()
{
}

void BaseDialog::writeParameter(YAML::Node& config,std::string moduleName)
{

    for(int i=0 ; i < allLineEdits.size(); i++)
        config[moduleName][allLineEdits[i]->objectName().toStdString()] = allLineEdits[i]->text().toStdString();
    for(int i=0 ; i<allComboBoxes.size(); i++)
        config[moduleName][allComboBoxes[i]->objectName().toStdString()] = allComboBoxes[i]->currentText().toStdString();
}

void BaseDialog::checkIsValide()
{
    testEdit = qobject_cast<QLineEdit *>(sender());
    palette.setColor(QPalette::Base,Qt::white);
    if (!testEdit->hasAcceptableInput() && testEdit->text() != "" )
        palette.setColor(QPalette::Base,Qt::red);
    testEdit->setPalette(palette);
}

void BaseDialog::setValidatorFloat()
{
    QDoubleValidator *validatorFloat = new QDoubleValidator(this);
    validatorFloat->setNotation(QDoubleValidator::StandardNotation);    //keine e-100  Darstellung
    for(int i=0 ; i<allLineEdits.size(); i++)
        if (allLineEdits[i]->objectName().at(0) == "n")
            allLineEdits[i]->setValidator(new QRegExpValidator(QRegExp("[0-9]*"),this));
        else
            allLineEdits[i]->setValidator(validatorFloat);
}

void BaseDialog::setValidator(QLineEdit *lEdit, QMap <QString,QStringList> &map)
{
    std::cout << "setValidator" << std::endl;

    bool isNumeric=false;
    QDoubleValidator *validatorFloat = new QDoubleValidator(this);
    QIntValidator *validatorInt = new QIntValidator(this);
    validatorFloat->setNotation(QDoubleValidator::StandardNotation);    //keine e-100  Darstellung
    validatorFloat->setLocale(QLocale::C);
    validatorInt->setLocale(QLocale::C);
    if (map[lEdit->objectName()][0] == "float")
    {
        QString str =map[lEdit->objectName()][1];
        str.toDouble(&isNumeric);
        if ( isNumeric )
            validatorFloat->setBottom(str.toDouble());
        str = map[lEdit->objectName()][2];
        str.toDouble(&isNumeric);
        if ( isNumeric )
            validatorFloat->setTop(str.toDouble());
        validatorFloat->setDecimals(3);
        lEdit->setValidator(validatorFloat);
    }
    else if (map[lEdit->objectName()][0] == "int")
    {
        if ( map[lEdit->objectName()][1].toInt())
           validatorInt->setBottom(map[lEdit->objectName()][1].toInt());
        if ( map[lEdit->objectName()][2].toInt())
            validatorInt->setTop(map[lEdit->objectName()][2].toInt());
        lEdit->setValidator(validatorInt);
    }
    lEdit->setToolTip(map[lEdit->objectName()][1] + " - " +
                      map[lEdit->objectName()][2]);
}
