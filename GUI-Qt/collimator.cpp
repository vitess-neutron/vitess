#include "collimator.h"
#include "ui_collimator.h"

Collimator::Collimator(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Collimator)
{
    ui->setupUi(this);
    allLineEdits =this->findChildren<QLineEdit*>();
    setValidatorFloat();
    allComboBoxes = this->findChildren<QComboBox*>();
    modName = this->objectName().toStdString();
}

Collimator::~Collimator()
{
    delete ui;
}
void Collimator::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Collimator::readValues(YAML::Node& config)
{
    readModule(config);
}

void Collimator::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Collimator::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}
