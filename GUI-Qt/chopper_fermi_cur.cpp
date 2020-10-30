#include "chopper_fermi_cur.h"
#include "ui_chopper_fermi_cur.h"

Chopper_fermi_cur::Chopper_fermi_cur(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Chopper_fermi_cur)
{
    ui->setupUi(this);
    allLineEdits =this->findChildren<QLineEdit*>();
    setValidatorFloat();
    allComboBoxes = this->findChildren<QComboBox*>();
    modName = this->objectName().toStdString();
}

Chopper_fermi_cur::~Chopper_fermi_cur()
{
    delete ui;
}
void Chopper_fermi_cur::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Chopper_fermi_cur::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}
/*
void Chopper_fermi_cur::writePipe(QTextStream& out)
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

void Chopper_fermi_cur::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Chopper_fermi_cur::readValues(YAML::Node& config)
{
    readModule(config);
}
