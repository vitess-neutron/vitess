#include "detector.h"
#include "ui_detector.h"

Detector::Detector(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Detector)
{
    ui->setupUi(this);
    allLineEdits =this->findChildren<QLineEdit*>();
    setValidatorFloat();
    allComboBoxes = this->findChildren<QComboBox*>();
    modName = this->objectName().toStdString();
}

Detector::~Detector()
{
    delete ui;
}

void Detector::writeValues(YAML::Node& config)
{
    writeModule(config);
}
void Detector::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Detector::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}


void Detector::readValues(YAML::Node& config)
{
    readModule(config);
}
