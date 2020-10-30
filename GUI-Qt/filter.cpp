#include "filter.h"
#include "ui_filter.h"

Filter::Filter(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Filter)
{
    ui->setupUi(this);
    allLineEdits =this->findChildren<QLineEdit*>();
    setValidatorFloat();
    allComboBoxes = this->findChildren<QComboBox*>();
    modName = this->objectName().toStdString();
}

Filter::~Filter()
{
    delete ui;
}
void Filter::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Filter::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Filter::writeValues(YAML::Node& config)
{
    writeModule(config);
}
void Filter::readValues(YAML::Node& config)
{
    readModule(config);
}
