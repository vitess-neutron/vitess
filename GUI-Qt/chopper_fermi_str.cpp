#include "chopper_fermi_str.h"
#include "ui_chopper_fermi_str.h"

Chopper_fermi_str::Chopper_fermi_str(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Chopper_fermi_str)
{
    ui->setupUi(this);
    allLineEdits =this->findChildren<QLineEdit*>();
    setValidatorFloat();
    allComboBoxes = this->findChildren<QComboBox*>();
    modName = this->objectName().toStdString();
}

Chopper_fermi_str::~Chopper_fermi_str()
{
    delete ui;
}
void Chopper_fermi_str::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Chopper_fermi_str::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}
/*
void Chopper_fermi_str::writePipe(QTextStream& out)
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
void Chopper_fermi_str::writeValues(YAML::Node& config)
{
    writeModule(config);
}
void Chopper_fermi_str::readValues(YAML::Node& config)
{
    readModule(config);
}
