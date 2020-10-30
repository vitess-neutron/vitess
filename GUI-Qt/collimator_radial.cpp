#include "collimator_radial.h"
#include "ui_collimator_radial.h"

Collimator_radial::Collimator_radial(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Collimator_radial)
{
    ui->setupUi(this);
    allLineEdits =this->findChildren<QLineEdit*>();
    setValidatorFloat();
    allComboBoxes = this->findChildren<QComboBox*>();
    modName = this->objectName().toStdString();
}

Collimator_radial::~Collimator_radial()
{
    delete ui;
}

void Collimator_radial::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Collimator_radial::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Collimator_radial::readValues(YAML::Node& config)
{
    readModule(config);
}

void Collimator_radial::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}
