#include "capture_flux.h"
#include "ui_capture_flux.h"

Capture_flux::Capture_flux(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Capture_flux)
{
    ui->setupUi(this);
    allLineEdits =this->findChildren<QLineEdit*>();
    setValidatorFloat();
    QList<QComboBox *> allComboBoxes = this->findChildren<QComboBox*>();
    std::string modName = this->objectName().toStdString();
}

Capture_flux::~Capture_flux()
{
    delete ui;
}

void Capture_flux::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Capture_flux::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

/*
void Capture_flux::writePipe(QTextStream& out)
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


void Capture_flux::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Capture_flux::readValues(YAML::Node& config)
{
    readModule(config);
}
