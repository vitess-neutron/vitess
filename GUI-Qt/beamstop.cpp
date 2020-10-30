#include "beamstop.h"
#include "ui_beamstop.h"

Beamstop::Beamstop(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Beamstop)
{
    ui->setupUi(this);
    allLineEdits =this->findChildren<QLineEdit*>();
    setValidatorFloat();                  //in basemodule setzt DoubleValidator für alle lineEdits
    allComboBoxes = this->findChildren<QComboBox*>();
    modName = this->objectName().toStdString();
}

Beamstop::~Beamstop()
{
    delete ui;
}

void Beamstop::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Beamstop::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

/*
void Beamstop::writePipe(QTextStream& out)
{
    map.size();
    foreach(QString entry, map.keys())
    {
        out << " " << map[entry];
        if (this->findChild<QLineEdit *>(entry))
            out << this->findChild<QLineEdit *>(entry)->text();
        else
            out << this->findChild<QComboBox *>(entry)->currentText();
    }
}
*/
void Beamstop::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Beamstop::readValues(YAML::Node& config)
{
    readModule(config);
}

