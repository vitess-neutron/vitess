#include <iostream>
#include "basemodule.h"
#include <QWidget>
#include <QFileDialog>
#include <QMessageBox>

BaseModule::BaseModule(QWidget *parent) :
    QWidget(parent)
{

}
BaseModule::~BaseModule()
{

}
void BaseModule::checkIsValide()
{
    testEdit = qobject_cast<QLineEdit *>(sender());
    palette.setColor(QPalette::Base,Qt::white);
    if (!testEdit->hasAcceptableInput() && testEdit->text() != "" )
        palette.setColor(QPalette::Base,Qt::red);
    testEdit->setPalette(palette);
}
void BaseModule::setValidator(QLineEdit *lEdit, QStringList &Limits)
{
    if (Limits[1] == "float")
    {
        QDoubleValidator *validatorFloat = new QDoubleValidator(this);
        validatorFloat->setNotation(QDoubleValidator::StandardNotation);    //keine e-100  Darstellung
        validatorFloat->setLocale(QLocale::C);
        val = Limits[2].toDouble(&flag);
        if ( flag ) validatorFloat->setBottom(val);
        if (Limits[3] != "") validatorFloat->setTop(Limits[3].toDouble());
        val = Limits[3].toDouble(&flag);
        if ( flag ) validatorFloat->setTop(val);
        validatorFloat->setDecimals(3);
        lEdit->setValidator(validatorFloat);
    }
    else if (Limits[1] == "int")
    {
        QIntValidator *validatorInt = new QIntValidator(this);
        if ( Limits[2].toInt())
           validatorInt->setBottom( Limits[2].toInt());
        if ( Limits[3].toInt())
            validatorInt->setTop(Limits[3].toInt());
        lEdit->setValidator(validatorInt);
    }
    lEdit->setToolTip(Limits[2] + " - " + Limits[3]);
}


QString BaseModule::openFileName()
{
    QString fileName = QFileDialog::getOpenFileName(this,"Open Instrument","/home/jcns/Downloads/vitess3.4",
                                                  tr("YAML (*.yaml *.yml)"));
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
//    if (!file.open(QFile::ReadWrite | QFile::Text))
    {
        QMessageBox::information(this,"Warning cannot open: ",fileName);
        return fileName="";
    }
    return fileName;
}

QString BaseModule::saveFileName()
{
    QString fileName = QFileDialog::getSaveFileName(this,"Save Instrument as","/home/jcns/Downloads/vitess3.4",
                                                  tr("Files (*.yaml *.yml)"));
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))        //open file
    {
        QMessageBox::information(this,"Warning cannot open: ",fileName);
        return fileName="";
    }
    return fileName;
}



void BaseModule::writeModule(YAML::Node& config)
{
    for(int i=0 ; i < allLineEdits.size(); i++)
    {
        config[modName][allLineEdits[i]->objectName().toStdString()] = allLineEdits[i]->text().toStdString();
    }
    for(int i=0 ; i<allComboBoxes.size(); i++)
        config[modName][allComboBoxes[i]->objectName().toStdString()] = allComboBoxes[i]->currentText().toStdString();

}

void BaseModule::readModule(YAML::Node& config)
{
    for(YAML::const_iterator it=config.begin(); it!=config.end(); ++it)
    {
       flag = false;
//       cout << "first:" << it->first.as<string>() << endl;
//       cout << "  second:" << it->second.as<string>() << endl;
       QString childName = QString::fromStdString(it->first.as<string>());      //key
       cout << "childName: " << childName.toStdString() << endl;
       foreach (QLineEdit* child, allLineEdits)
       {
           if ( childName.indexOf(child->objectName()) == 0 )                   //if key is lineEdit objectname
           {
              flag = true;
              child->setText(QString::fromStdString(it->second.as<string>()));  //setText of linEdit
              break;
           }
       }
       if (!flag)                                                               //if key is no lineEdit
          foreach (QComboBox* child, allComboBoxes)
          {
             if ( childName.indexOf(child->objectName()) == 0 )                 //if key is comboBox objectname
             {
                child->setCurrentText(QString::fromStdString(it->second.as<string>()));
                break;
             }
          }
     }
}

void BaseModule::modulePipe(QTextStream& out, QMap<QString,QStringList>& map)
{
    foreach(QString entry, map.keys())
    {
        out << " " << map[entry][0];
        flag = false;
        foreach (QLineEdit* child, allLineEdits)
        {
           if ( entry.indexOf(child->objectName()) == 0 )                   //if key is lineEdit objectname
           {
              flag = true;
              out << child->text();
              break;
           }
        }
        if (!flag)                                                               //if key is no lineEdit
           foreach (QComboBox* child, allComboBoxes)
              if ( entry.indexOf(child->objectName()) == 0 )                   //if key is lineEdit objectname
              {
 //               out << child->currentText();
                out << child->currentIndex();
                break;
             }
     }
}

void BaseModule::moduleCmd(QString& cmd, QMap<QString,QStringList>& map)
{
    foreach(QString entry, map.keys())
    {
        cmd += " " + map[entry][0];
        flag = false;
        foreach (QLineEdit* child, allLineEdits)
        {
           if ( entry.indexOf(child->objectName()) == 0 )                   //if key is lineEdit objectname
           {
              flag = true;
              cmd += child->text();
              break;
           }
        }
        if (!flag)                                                               //if key is no lineEdit
           foreach (QComboBox* child, allComboBoxes)
              if ( entry.indexOf(child->objectName()) == 0 )                   //if key is lineEdit objectname
              {
//               out << child->currentText();
                int index = child->currentIndex();
                if (child->itemData(index).toString() == "")
                    cmd += QString::number(child->currentIndex());
                else
                    cmd += child->itemData(index).toString();
                break;
             }
     }
}

// Beim speichern aller widgets in QList<Qwidgets *> funktionierte das casten nur bei lineEdits, comboBoxes fanden nicht den richtigen type, bzw. parent
// deshalb jetzt zwei foreach loops
//                if (qobject_cast<QLineEdit*>(child))
//                   qobject_cast<QLineEdit*>(child)->setText(QString::fromStdString(it2->second.as<string>()));
//                else if (qobject_cast<QComboBox*>(child))
//                   qobject_cast<QComboBox*>(child)->setCurrentText(QString::fromStdString(it2->second.as<string>()));
