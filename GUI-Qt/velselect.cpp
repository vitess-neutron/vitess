#include "velselect.h"
#include "ui_velselect.h"

Velselect::Velselect(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Velselect)
{
    ui->setupUi(this);
    foreach(QString entry, map.keys())
        if (this->findChild<QLineEdit *>(entry))
        {
           QLineEdit *lEdit = this->findChild<QLineEdit *>(entry);
           setValidator( lEdit,map[entry]);
           connect(this->findChild<QLineEdit *>(entry), SIGNAL(textChanged(const QString &)),this,
                                                        SLOT(checkIsValide()));
           allLineEdits += this->findChild<QLineEdit *>(entry);
        }
    allComboBoxes = this->findChildren<QComboBox*>();
    modName = this->objectName().toStdString();
}

Velselect::~Velselect()
{
    delete ui;
}
void Velselect::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Velselect::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Velselect::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Velselect::readValues(YAML::Node& config)
{
    readModule(config);
}
